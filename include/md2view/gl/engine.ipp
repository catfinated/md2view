#include "md2view/gl/engine.hpp"

#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <array>
#include <utility>

template <typename Game> void GL::Engine<Game>::doInit() {
    int width = width_;
    int height = height_;
    screen_width_ = width;
    screen_height_ = height;

    std::optional<std::filesystem::path> pak;
    if (!pak_path_.empty()) {
        pak = pak_path_;
    }
    resource_manager_ = std::make_unique<ResourceManager>("data", pak);

    constexpr std::array hints{
        WindowHint{GLFW_CONTEXT_VERSION_MAJOR, 4},
        WindowHint{GLFW_CONTEXT_VERSION_MINOR, 1},
        WindowHint{GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE},
        WindowHint{GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE}};

    window_ = Window::create(width, height, game_.title(), hints);
    glfwMakeContextCurrent(window_.get());

    window_.setWindowResizeHandler([this](int width, int height) {
        spdlog::debug("window resize x={} y={}", width, height);
        screen_width_ = width;
        screen_height_ = height;
    });
    window_.setFramebufferResizeHandler([this](int width, int height) {
        spdlog::debug("framebuffer resize x={} y={}", width, height);
        width_ = width;
        height_ = height;
        game_.on_framebuffer_resized(width, height);
    });

    glfwSetInputMode(window_.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwMakeContextCurrent(window_.get());

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

    glfwGetFramebufferSize(window_.get(), &width_, &height_);
    glViewport(0, 0, width_, height_);

    spdlog::info("Default frame buffer size {}x{}", width_, height_);

    GLint nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    spdlog::info("Maximum # of vertex attributes supported: {}", nrAttributes);

    if (!game_.on_engine_initialized(*this)) {
        throw std::runtime_error("failed to initialize game");
    }
    glCheckError();

    mouse_.xpos = width / 2.0;
    mouse_.ypos = height / 2.0;

    gui_ = std::make_unique<GL::Gui>(*this, *resource_manager_,
                                     gsl_lite::not_null{window_.get()});
    glCheckError();
}

template <typename Game> void GL::Engine<Game>::run_game() {
    last_frame_ = gsl_lite::narrow_cast<GLfloat>(glfwGetTime());
    // glfwSwapInterval(1);

    while (!window_.shouldClose()) {
        auto const current_frame =
            gsl_lite::narrow_cast<GLfloat>(glfwGetTime());
        delta_time_ = current_frame - last_frame_;
        last_frame_ = current_frame;

        beginFrame();
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

        glfwSwapBuffers(window_.get());
    }

    glCheckError();
}

template <typename Game>
void GL::Engine<Game>::onCursorPos(double xpos, double ypos) {
    ::Engine::onCursorPos(xpos, ypos);
    if (input_goes_to_game_) {
        game_.on_mouse_movement(gsl_lite::narrow_cast<float>(mouse_.xoffset),
                                gsl_lite::narrow_cast<float>(mouse_.yoffset));
    }
}

template <typename Game>
void GL::Engine<Game>::onScroll(double xoffset, double yoffset) {
    ::Engine::onScroll(xoffset, yoffset);
    if (input_goes_to_game_) {
        game_.on_mouse_scroll(xoffset, yoffset);
    }
}

template <typename Game>
void GL::Engine<Game>::onMouseButton(int button, int action, int /*mods*/) {
    if (gui_) {
        gui_->onMouseButton(button, action);
    }
}
