#pragma once

#include "md2view/gl/gl.hpp" // IWYU pragma: keep

#include <GLFW/glfw3.h>
#include <gsl-lite/gsl-lite.hpp>

#include <array>

class Engine;

namespace GL {

/// ImGui integration for the OpenGL renderer
class Gui {
public:
    /// Create context and initialize backend.
    /// Requires a current OpenGL context.
    ///
    /// @throws std::runtime_error on failure to initialize
    Gui(::Engine& engine, gsl_lite::not_null<GLFWwindow*> window);
    ~Gui();

    Gui(Gui const&) = delete;
    Gui& operator=(Gui const&) = delete;

    Gui(Gui&&) noexcept = delete;
    Gui& operator=(Gui&&) noexcept = delete;

    void update(double current_time, bool apply_inputs = true);

    /// Record a press so a click shorter than a frame still registers
    void onMouseButton(int button, int action);
    void render();

private:
    ::Engine& engine_;
    gsl_lite::not_null<GLFWwindow*> window_;
    double time_ = 0.0;
    std::array<bool, 3> mouse_pressed_{false, false, false};
};

} // namespace GL
