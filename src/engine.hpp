#pragma once

#include "resource_manager.hpp"
#include "gui.hpp"

#include <boost/program_options.hpp>
#include <GLFW/glfw3.h>

#include <bitset>
#include <random>

class EngineBase
{
public:
    static size_t const max_keys = 1024;

    EngineBase()
        : resource_manager_("data") // up from build dir
    {}

    int width() const { return width_; }
    int height() const { return height_; }

    int screen_width() const { return screen_width_; }
    int screen_height() const { return screen_height_; }

    ResourceManager& resource_manager() { return resource_manager_; }
    std::bitset<max_keys> const& keys() const { return keys_; }

    bool check_key_pressed(unsigned int key);

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

    boost::program_options::options_description opt_desc_;
    boost::program_options::variables_map variables_map_;
};

template <typename Game>
class Engine : public EngineBase
{
public:
    Engine()
        : EngineBase()
        , gui_(*this)
    {}

    bool init(int argc, char const * argv[]);
    void run_game();

protected:
    GLfloat delta_time() const { return delta_time_; }

    // consider using attorney so these callbacks don't have to be public
    void key_callback(int key, int action);
    void mouse_callback(double xpos, double ypos);
    void scroll_callback(double xoffset, double yoffset);
    bool parse_args(int argc, char const * argv[]);

    void window_resize_callback(int x, int y);
    void framebuffer_resize_callback(int x, int y);

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

