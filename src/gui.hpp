#pragma once

#include "gl.hpp"
#include "imgui/imgui.h"

#include <GLFW/glfw3.h>

class EngineBase;

class Gui
{
public:
    Gui(EngineBase&);

    Gui(Gui const&) = delete;
    Gui& operator=(Gui const&) = delete;

    Gui(Gui&&) = delete;
    Gui& operator=(Gui&&) = delete;

    void init(GLFWwindow *);
    void update(double current_time, bool apply_inputs = true);
    void render();
    void shutdown();

private:
    enum
    {
        vertex = 0,
        element = 1,
        num_buffers
    };

    EngineBase&   engine_;
    GLFWwindow *  window_ = nullptr;
    double        time_ = 0.0;
    bool          mouse_pressed_[3] = { false, false, false };
    float         mouse_wheel_ = 0.0f;
    GLuint        font_texture_ = 0;
    int           attrib_location_tex_ = 0;
    int           attrib_location_projection_ = 0;
    int           attrib_location_position_ = 0;
    int           attrib_location_uv_ = 0;
    int           attrib_location_color_ = 0;
    unsigned int  vao_ = 0;
    unsigned int  glbuffers_[num_buffers];
};
