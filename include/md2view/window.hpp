/**
 * @brief Types and functions related to windowing
 *
 * Shared by the OpenGL and Vulkan backends. GLFW is deliberately not included
 * here since glfw3.h pulls in GL/gl.h, which must not precede GL/glew.h.
 */
#pragma once

#include <span>
#include <utility>

struct GLFWwindow;

/**
 * @brief RAII guard for glfwInit/glfwTerminate
 *
 * Declare before any object that uses GLFW so that it is destroyed last.
 */
struct GlfwContext {
    /// @throws std::runtime_error if GLFW could not be initialized
    GlfwContext();
    ~GlfwContext();
    GlfwContext(GlfwContext const&) = delete;
    GlfwContext& operator=(GlfwContext const&) = delete;
};

/**
 * @brief A GLFW window creation hint, see glfwWindowHint
 */
struct WindowHint {
    int hint;
    int value;
};

/**
 * @brief RAII wrapper for a GLFW window
 *
 * For an OpenGL window this also owns the GL context, so it must outlive
 * every object that makes GL calls in its destructor.
 */
class Window {
public:
    explicit Window(GLFWwindow* window = nullptr) noexcept;
    ~Window() noexcept;

    Window(Window const&) = delete;
    Window& operator=(Window const&) = delete;

    Window(Window&& rhs) noexcept
        : window_(std::exchange(rhs.window_, nullptr)) {}

    Window& operator=(Window&& rhs) noexcept;

    [[nodiscard]] bool shouldClose() const noexcept;

    [[nodiscard]] GLFWwindow* get() const noexcept { return window_; }

    /// Create a window. Hints are applied on top of the GLFW defaults.
    ///
    /// @throws std::runtime_error if the window could not be created
    static Window create(int width,
                         int height,
                         char const* title,
                         std::span<WindowHint const> hints = {});

private:
    GLFWwindow* window_;
};
