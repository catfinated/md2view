#pragma once

#include "gl.hpp"

#include <GLFW/glfw3.h>
#include <gsl-lite/gsl-lite.hpp>
#include <imgui.h>

#include <array>

class Engine;

class Gui {
public:
    Gui(Engine&, gsl_lite::not_null<GLFWwindow*> window);

    Gui(Gui const&) = delete;
    Gui& operator=(Gui const&) = delete;

    Gui(Gui&&) = delete;
    Gui& operator=(Gui&&) = delete;

    void update(double current_time, bool apply_inputs = true);
    void render();
    void shutdown();

private:
    enum { vertex = 0, element = 1, num_buffers };

    void init();

    Engine& engine_;
    gsl_lite::not_null<GLFWwindow*> window_;
    double time_ = 0.0;
    std::array<bool, 3> mouse_pressed_{false, false, false};
    GLuint font_texture_ = 0;
    int attrib_location_tex_ = 0;
    int attrib_location_projection_ = 0;
    int attrib_location_position_ = 0;
    int attrib_location_uv_ = 0;
    int attrib_location_color_ = 0;
    unsigned int vao_ = 0;
    std::array<unsigned int, num_buffers> glbuffers_{};
};
