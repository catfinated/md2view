#include "md2view/gl/engine.hpp"

#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <utility>

template <typename Game>
bool GLEngine<Game>::init(std::span<char const*> args) {
    if (!parse_args(args)) {
        return false;
    }

    int width = width_;
    int height = height_;

    screen_width_ = width;
    screen_height_ = height;

    std::optional<std::filesystem::path> pak;
    if (!pak_path_.empty()) {
        pak = pak_path_;
    }
    resource_manager_ = std::make_unique<ResourceManager>("data", pak);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window_ = glfwCreateWindow(width, height, game_.title(), nullptr, nullptr);
    gsl_Assert(window_);
    glfwMakeContextCurrent(window_);

    glfwSetWindowUserPointer(window_, this);

    // TODO: Keyboard/InputManager classes

    // define the callbacks here as lambdas so they are not accessible to
    // outside code
    auto key_callback = [](GLFWwindow* window, int key, int /* scancode */,
                           int action, int /* mode */) {
        using EngineType = GLEngine<Game>;
        auto* engine =
            static_cast<EngineType*>(glfwGetWindowUserPointer(window));
        gsl_Assert(engine);
        engine->key_callback(key, action);
    };

    glfwSetKeyCallback(window_, key_callback);

    auto mouse_callback = [](GLFWwindow* window, double xpos, double ypos) {
        using EngineType = GLEngine<Game>;
        auto* engine =
            static_cast<EngineType*>(glfwGetWindowUserPointer(window));
        gsl_Assert(engine);
        engine->mouse_callback(xpos, ypos);
    };

    glfwSetCursorPosCallback(window_, mouse_callback);

    auto scroll_callback = [](GLFWwindow* window, double xoffset,
                              double yoffset) {
        using EngineType = GLEngine<Game>;
        auto* engine =
            static_cast<EngineType*>(glfwGetWindowUserPointer(window));
        gsl_Assert(engine);
        engine->scroll_callback(xoffset, yoffset);
    };

    glfwSetScrollCallback(window_, scroll_callback);

    auto win_resize_callback = [](GLFWwindow* window, int width, int height) {
        using EngineType = GLEngine<Game>;
        auto* engine =
            static_cast<EngineType*>(glfwGetWindowUserPointer(window));
        gsl_Assert(engine);
        engine->window_resize_callback(width, height);
    };

    glfwSetWindowSizeCallback(window_, win_resize_callback);

    auto fb_resize_callback = [](GLFWwindow* window, int width, int height) {
        using EngineType = GLEngine<Game>;
        auto* engine =
            static_cast<EngineType*>(glfwGetWindowUserPointer(window));
        gsl_Assert(engine);
        engine->framebuffer_resize_callback(width, height);
    };

    glfwSetFramebufferSizeCallback(window_, fb_resize_callback);

    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwMakeContextCurrent(window_);

    spdlog::info("gl version: {}", glStrView(glGetString(GL_VERSION)));
    spdlog::info("gl renderer: {}", glStrView(glGetString(GL_RENDERER)));

    glewExperimental = GL_TRUE;
    auto const result = glewInit();

    // https://github.com/nigels-com/glew/issues/417
    if (result != GLEW_OK && result != GLEW_ERROR_NO_GLX_DISPLAY) {
        std::string_view err = glStrView(glewGetErrorString(result));
        throw std::runtime_error(fmt::format("failed to init glew: '{}'", err));
    }

    glGetError(); // glewInit is known to cause invalid enum error

    glfwGetFramebufferSize(window_, &width_, &height_);
    glViewport(0, 0, width_, height_);

    spdlog::info("Default frame buffer size {}x{}", width_, height_);

    GLint nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    spdlog::info("Maximum # of vertex attributes supported: {}", nrAttributes);

    if (!game_.on_engine_initialized(*this)) {
        spdlog::error("failed to initialize game");
        return false;
    }
    glCheckError();

    mouse_.xpos = width / 2.0;
    mouse_.ypos = height / 2.0;

    gui_ = std::make_unique<Gui>(*this, gsl_lite::not_null{window_});
    glCheckError();

    return true;
}

template <typename Game> void GLEngine<Game>::run_game() {
    last_frame_ = gsl_lite::narrow_cast<GLfloat>(glfwGetTime());
    // glfwSwapInterval(1);

    while (glfwWindowShouldClose(window_) == 0) {
        auto const current_frame =
            gsl_lite::narrow_cast<GLfloat>(glfwGetTime());
        delta_time_ = current_frame - last_frame_;
        last_frame_ = current_frame;

        glfwPollEvents();

        if (input_goes_to_game_) {
            game_.process_input(*this, delta_time_);
        }

        gui_->update(current_frame, !input_goes_to_game_);
        glCheckError();
        game_.update(*this, delta_time_);
        glCheckError();
        game_.render(*this);
        glCheckError();
        gui_->render();
        glCheckError();

        glfwSwapBuffers(window_);
    }

    glCheckError();
}

template <typename Game>
void GLEngine<Game>::key_callback(int key, int action) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window_, GL_TRUE);
        } else if (key == GLFW_KEY_F1) {
            input_goes_to_game_ = !input_goes_to_game_;
            spdlog::info("got F1. game input: {}", input_goes_to_game_);
        } else if (key >= 0 && std::cmp_less(key, max_keys)) {
            keys_[key] = true;
        }
    } else if (action == GLFW_RELEASE && key >= 0 &&
               std::cmp_less(key, max_keys)) {
        keys_[key] = false;
        keys_pressed_[key] = false;
    }
}

template <typename Game>
void GLEngine<Game>::mouse_callback(double xpos, double ypos) {
    GLfloat xoffset = xpos - mouse_.xpos.value_or(xpos);
    GLfloat yoffset = mouse_.ypos.value_or(ypos) -
                      ypos; // reversed since y-coords go from bottom to top
    mouse_.xpos = xpos;
    mouse_.ypos = ypos;

    if (input_goes_to_game_) {
        game_.on_mouse_movement(xoffset, yoffset);
    }
}

template <typename Game>
void GLEngine<Game>::scroll_callback(double xoffset, double yoffset) {
    mouse_.scroll_xoffset = xoffset;
    mouse_.scroll_yoffset = yoffset;

    if (input_goes_to_game_) {
        game_.on_mouse_scroll(xoffset, yoffset);
    }
}

template <typename Game>
void GLEngine<Game>::window_resize_callback(int x, int y) {
    spdlog::info("window resize x={} y={}", x, y);
    screen_width_ = x;
    screen_height_ = y;
}

template <typename Game>
void GLEngine<Game>::framebuffer_resize_callback(int x, int y) {
    spdlog::info("framebuffer resize x={} y={}", x, y);
    width_ = x;
    height_ = y;
    game_.on_framebuffer_resized(x, y);
}
