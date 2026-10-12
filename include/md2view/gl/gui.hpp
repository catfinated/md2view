#pragma once

#include "md2view/gl/gl.hpp" // IWYU pragma: keep
#include "md2view/window.hpp"

#include <GLFW/glfw3.h>
#include <gsl-lite/gsl-lite.hpp>

namespace GL {

/// ImGui integration for the OpenGL renderer
class Gui : public InputListener {
public:
    /// Create context and initialize backends.
    /// Requires a current OpenGL context.
    ///
    /// @throws std::runtime_error on failure to initialize
    explicit Gui(gsl_lite::not_null<GLFWwindow*> window);
    ~Gui() override;

    Gui(Gui const&) = delete;
    Gui& operator=(Gui const&) = delete;

    Gui(Gui&&) noexcept = delete;
    Gui& operator=(Gui&&) noexcept = delete;

    void update();
    void render();

    /// Call when the GUI gains or loses input
    void onInputFocus(bool focused);

    /// True while a text field is being edited
    [[nodiscard]] bool wantsTextInput() const;

    void onKey(int key, int scancode, int action, int mods) override;
    void onMouseButton(int button, int action, int mods) override;
    void onCursorPos(double xpos, double ypos) override;
    void onScroll(double xoffset, double yoffset) override;
    void onChar(unsigned int codepoint) override;

private:
    gsl_lite::not_null<GLFWwindow*> window_;
};

} // namespace GL
