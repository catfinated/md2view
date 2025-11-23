#pragma once

#include "md2view/gl/gl.hpp"

#include <GLFW/glfw3.h>
#include <gsl-lite/gsl-lite.hpp>
#include <imgui.h>

#include <array>
#include <cstdint>

class Engine;

class Gui {
public:
    Gui(Engine& engine, gsl_lite::not_null<GLFWwindow*> window);
    ~Gui() = default;

    Gui(Gui const&) = delete;
    Gui& operator=(Gui const&) = delete;

    Gui(Gui&&) noexcept = delete;
    Gui& operator=(Gui&&) noexcept = delete;

    void update(double current_time, bool apply_inputs = true);
    void render();
    void shutdown();

private:
    enum class Buffer : std::int8_t { vertex = 0, element, num_buffers };

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
    std::array<unsigned int, static_cast<size_t>(Buffer::num_buffers)>
        glbuffers_{};
};
