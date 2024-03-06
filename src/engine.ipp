#include "engine.hpp"
#include "common.hpp"

#include <cassert>

#include <spdlog/spdlog.h>

inline bool EngineBase::check_key_pressed(unsigned int key)
{
    assert(key < max_keys);

    if (keys_[key] && !keys_pressed_[key]) {
        keys_pressed_[key] = true;
        return true;
    }
    return false;
}

template <typename Game>
bool Engine<Game>::parse_args(int argc, char const * argv[])
{
    boost::program_options::options_description engine("Engine options");
    engine.add_options()
        ("help,h", "Show this help message")
        ("width,W", boost::program_options::value<int>(&width_)->default_value(1280), "Screen width")
        ("height,H", boost::program_options::value<int>(&height_)->default_value(800), "Screen height");

    options_desc().add(engine).add(game_.options());

    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, options_desc()), variables_map_);
    boost::program_options::notify(variables_map_);

    if (variables_map_.count("help")) {
        std::cerr << options_desc() << '\n';
        return false;
    }

    return game_.parse_args(*this);
}

template <typename Game>
bool Engine<Game>::init(int argc, char const * argv[])
{
    if (!parse_args(argc, argv)) {
        return false;
    }

    int width = width_;
    int height = height_;

    screen_width_ = width;
    screen_height_ = height;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window_ = glfwCreateWindow(width, height, game_.title(), nullptr, nullptr);
    MD2V_EXPECT(window_);
    glfwMakeContextCurrent(window_);

    glfwSetWindowUserPointer(window_, this);

    // TODO: Keyboard/InputManager classes

    // define the callbacks here as lambdas so they are not accessible to outside code
    auto key_callback = [](GLFWwindow * window, int key, int scancode, int action, int mode) {
        using EngineType = Engine<Game>;
        EngineType * engine = static_cast<EngineType *>(glfwGetWindowUserPointer(window));
        assert(engine);
        engine->key_callback(key, action);
    };

    glfwSetKeyCallback(window_, key_callback);

    auto mouse_callback = [](GLFWwindow * window, double xpos, double ypos) {
        using EngineType = Engine<Game>;
        EngineType * engine = static_cast<EngineType *>(glfwGetWindowUserPointer(window));
        assert(engine);
        engine->mouse_callback(xpos, ypos);
    };

    glfwSetCursorPosCallback(window_, mouse_callback);

    auto scroll_callback = [](GLFWwindow * window, double xoffset, double yoffset) {
        using EngineType = Engine<Game>;
        EngineType * engine = static_cast<EngineType *>(glfwGetWindowUserPointer(window));
        assert(engine);
        engine->scroll_callback(xoffset, yoffset);
     };

    glfwSetScrollCallback(window_, scroll_callback);

    auto win_resize_callback = [](GLFWwindow * window, int width, int height) {
        using EngineType = Engine<Game>;
        EngineType * engine = static_cast<EngineType *>(glfwGetWindowUserPointer(window));
        assert(engine);
        engine->window_resize_callback(width, height);
    };

    glfwSetWindowSizeCallback(window_, win_resize_callback);

    auto fb_resize_callback = [](GLFWwindow * window, int width, int height) {
        using EngineType = Engine<Game>;
        EngineType * engine = static_cast<EngineType *>(glfwGetWindowUserPointer(window));
        assert(engine);
        engine->framebuffer_resize_callback(width, height);
    };

    glfwSetFramebufferSizeCallback(window_, fb_resize_callback);

    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    MD2V_EXPECT(glewInit() == GLEW_OK);
    glGetError(); // glewInit is known to cause invalid enum error

    glfwGetFramebufferSize(window_, &width_, &height_);
    glViewport(0, 0, width_, height_);

    spdlog::info("Default frame buffer size {}x{}", width_, height_);

    GLint nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    spdlog::info("Maximum # of vertex attributes supported: {}", nrAttributes);

    MD2V_EXPECT(game_.on_engine_initialized(*this));
    glCheckError();

    last_x_ = width / 2.0;
    last_y_ = height / 2.0;

    gui_ = std::make_unique<Gui>(*this, gsl::not_null{window_});
    glCheckError();

    return true;
}

template <typename Game>
void Engine<Game>::run_game()
{
    last_frame_ = glfwGetTime();
    //glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window_)) {
        GLfloat current_frame = glfwGetTime();
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
void Engine<Game>::key_callback(int key, int action)
{
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window_, GL_TRUE);
        }
        else if (key == GLFW_KEY_F1) {
            input_goes_to_game_ = !input_goes_to_game_;
            spdlog::info("got F1. game input: {}", input_goes_to_game_);
        }
        else if (key >= 0 && static_cast<size_t>(key) < max_keys) {
            keys_[key] = true;
        }
    }
    else if (action == GLFW_RELEASE && key >= 0 && static_cast<size_t>(key) < max_keys)
    {
        keys_[key] = false;
        keys_pressed_[key] = false;
    }
}

template <typename Game>
void Engine<Game>::mouse_callback(double xpos, double ypos)
{
    if (first_mouse_) {
        last_x_ = xpos;
        last_y_ = ypos;
        first_mouse_ = false;
    }

    GLfloat xoffset = xpos - last_x_;
    GLfloat yoffset = last_y_ - ypos; // reversed since y-coords go from bottom to top
    last_x_ = xpos;
    last_y_ = ypos;

    if (input_goes_to_game_) {
        game_.on_mouse_movement(xoffset, yoffset);
    }
}

template <typename Game>
void Engine<Game>::scroll_callback(double xoffset, double yoffset)
{
    if (input_goes_to_game_) {
        game_.on_mouse_scroll(xoffset, yoffset);
    }
}

template <typename Game>
void Engine<Game>::window_resize_callback(int x, int y)
{
    spdlog::info("window resize x={} y={}", x, y);
    screen_width_ = x;
    screen_height_ = y;
}

template <typename Game>
void Engine<Game>::framebuffer_resize_callback(int x, int y)
{
    spdlog::info("framebuffer resize x={} y={}", x, y);
    width_ = x;
    height_ = y;
    game_.on_framebuffer_resized(x, y);
}