#pragma once

#include "md2view/engine.hpp"
#include "md2view/gl/gui.hpp"
#include "md2view/resource_manager.hpp"

#include <GLFW/glfw3.h>

namespace GL {

template <typename Game> class Engine : public ::Engine {
public:
    Engine() = default;

    bool init(std::span<char const*> args);
    void run_game();

    ResourceManager& resource_manager() {
        gsl_Expects(resource_manager_);
        return *resource_manager_;
    }

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
    std::unique_ptr<ResourceManager> resource_manager_;
    std::unique_ptr<GL::Gui> gui_;
    GLfloat delta_time_ = 0.0f;
    GLfloat last_frame_ = 0.0f;
    bool input_goes_to_game_ = false;
};

} // namespace GL
