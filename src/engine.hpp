#pragma once

#include "common.hpp"
#include "resource_manager.hpp"
#include "gui.hpp"

#include <GLFW/glfw3.h>

#include <boost/program_options.hpp>

#include <bitset>
#include <cassert>
#include <random>

namespace blue {

class EngineBase
{
public:
    static size_t const max_keys = 1024;

    EngineBase()
        : resource_manager_("../src") // up from build dir
        , mt_(std::random_device{}()/*1729*/)
    {}

    int width() const { return width_; }
    int height() const { return height_; }

    int screen_width() const { return screen_width_; }
    int screen_height() const { return screen_height_; }

    ResourceManager& resource_manager() { return resource_manager_; }
    std::bitset<max_keys> const& keys() const { return keys_; }

    bool check_key_pressed(unsigned int key);

    std::mt19937& random_engine() { return mt_; }

    float aspect_ratio() const { return static_cast<float>(width_) / static_cast<float>(height_); }

    boost::program_options::options_description& options_desc() { return opt_desc_; }
    boost::program_options::variables_map const& variables_map() { return variables_map_; }

private:
    ResourceManager resource_manager_;

protected:
    int width_;
    int height_;
    int screen_width_;
    int screen_height_;

    std::bitset<max_keys> keys_;
    std::bitset<max_keys> keys_pressed_;

    std::mt19937 mt_;

    boost::program_options::options_description opt_desc_;
    boost::program_options::variables_map variables_map_;
};

inline
bool EngineBase::check_key_pressed(unsigned int key)
{
    assert(key < max_keys);

    if (keys_[key] && !keys_pressed_[key]) {
        keys_pressed_[key] = true;
        return true;
    }
    return false;
}

template <typename Game>
class GLFWEngine : public EngineBase
{
public:
    GLFWEngine()
        : EngineBase()
        , gui_(*this)
    {}

    //bool init(int width, int height);
    bool init(int argc, char const * argv[]);
    void run();

protected:
    GLfloat delta_time() const { return delta_time_; }

    // consider using attorney so these callbacks don't have to be public
    void key_callback(int key, int action);
    void mouse_callback(double xpos, double ypos);
    bool parse_args(int argc, char const * argv[]);

private:
    Game game_;
    Gui gui_;
    GLFWwindow * window_;
    GLfloat delta_time_ = 0.0f;
    GLfloat last_frame_ = 0.0f;
    GLfloat last_x_;
    GLfloat last_y_;
    bool first_mouse_ = true;
    bool input_goes_to_game_ = false;
};

template <typename Game>
bool GLFWEngine<Game>::parse_args(int argc, char const * argv[])
{
    boost::program_options::options_description engine("Engine options");
    engine.add_options()
        ("help", "Show this help message")
        ("width,w", boost::program_options::value<int>(&width_)->default_value(1280), "Screen width")
        ("height,h", boost::program_options::value<int>(&height_)->default_value(800), "Screen height");

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
//bool GLFWEngine<Game>::init(int width, int height)
bool GLFWEngine<Game>::init(int argc, char const * argv[])
{
    if (!parse_args(argc, argv)) {
        return false;
    }

    int width = width_;
    int height = height_;

    screen_width_ = width;
    screen_height_ = height;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window_ = glfwCreateWindow(width, height, game_.title(), nullptr, nullptr);
    BLUE_EXPECT(window_);
    glfwMakeContextCurrent(window_);

    glfwSetWindowUserPointer(window_, this);

    // TODO: Keyboard/InputManager classes

    // define the callbacks here as lambdas so they are not accessible to outside code
    auto key_callback = [](GLFWwindow * window, int key, int scancode, int action, int mode) {
        using EngineType = GLFWEngine<Game>;
        EngineType * engine = static_cast<EngineType *>(glfwGetWindowUserPointer(window));
        assert(engine);
        engine->key_callback(key, action);
    };

    auto mouse_callback = [](GLFWwindow * window, double xpos, double ypos) {
        using EngineType = GLFWEngine<Game>;
        EngineType * engine = static_cast<EngineType *>(glfwGetWindowUserPointer(window));
        assert(engine);
        engine->mouse_callback(xpos, ypos);
    };

    glfwSetKeyCallback(window_, key_callback);
    glfwSetCursorPosCallback(window_, mouse_callback);
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    BLUE_EXPECT(glewInit() == GLEW_OK);
    glGetError(); // glewInit is known to cause invalid enum error

    glfwGetFramebufferSize(window_, &width_, &height_);
    glViewport(0, 0, width_, height_);

    std::cout << "Default frame buffer size " << width_ << "x" << height_ << '\n';

    GLint nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    std::cout << "Maximum # of vertex attributes supported: " << nrAttributes << '\n';

    BLUE_EXPECT(game_.on_engine_initialized(*this));
    glCheckError();

    last_x_ = width / 2.0;
    last_y_ = height / 2.0;

    gui_.init(window_);
    glCheckError();

    return true;
}

template <typename Game>
void GLFWEngine<Game>::run()
{
    while (!glfwWindowShouldClose(window_)) {
        GLfloat current_frame = glfwGetTime();
        delta_time_ = current_frame - last_frame_;
        last_frame_ = current_frame;

        glfwPollEvents();

        if (input_goes_to_game_) {
            game_.process_input(*this, delta_time_);
        }

        gui_.update(current_frame, !input_goes_to_game_);
        glCheckError();
        game_.update(*this, delta_time_);
        glCheckError();
        game_.render(*this);
        glCheckError();
        gui_.render();
        glCheckError();

        glfwSwapBuffers(window_);
    }

    glCheckError();
}

template <typename Game>
void GLFWEngine<Game>::key_callback(int key, int action)
{
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window_, GL_TRUE);
        }
        else if (key == GLFW_KEY_F1) {
            input_goes_to_game_ = !input_goes_to_game_;
            std::cout << "got F1. game input: " << std::boolalpha << input_goes_to_game_ << '\n';
        }
        else if (key >= 0 && static_cast<size_t>(key) < max_keys) {
            keys_[key] = true;
            std::cout << "key pressed: " << key << '\n';
        }
    }
    else if (action == GLFW_RELEASE && key >= 0 && static_cast<size_t>(key) < max_keys)
    {
        keys_[key] = false;
        keys_pressed_[key] = false;
    }
}

template <typename Game>
void GLFWEngine<Game>::mouse_callback(double xpos, double ypos)
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

} // namespace blue
