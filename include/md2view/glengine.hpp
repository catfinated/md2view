#pragma once

#include "engine.hpp"

#include <GLFW/glfw3.h>

template <typename Game> class GLEngine : public Engine {
public:
    GLEngine() = default;

    bool init(std::span<char const*> args);
    void run_game();

protected:
    [[nodiscard]] GLfloat delta_time() const { return delta_time_; }

    // consider using attorney so these callbacks don't have to be public
    void key_callback(int key, int action);
    void mouse_callback(double xpos, double ypos);
    void scroll_callback(double xoffset, double yoffset);

    void window_resize_callback(int x, int y);
    void framebuffer_resize_callback(int x, int y);

private:
    Game game_;
    GLFWwindow* window_{nullptr};
    std::unique_ptr<Gui> gui_;
    GLfloat delta_time_ = 0.0f;
    GLfloat last_frame_ = 0.0f;
    bool input_goes_to_game_ = false;
};
