/**
 * @brief Types and functions related to windowing
 */
#pragma once

#include <GLFW/glfw3.h>

#include <utility>

namespace VK {

/**
 * @brief RAII guard for glfwInit/glfwTerminate
 */
struct GlfwContext {
    /// @throws std::runtime_error if GLFW could not be initialized
    GlfwContext();
    ~GlfwContext();
    GlfwContext(GlfwContext const&) = delete;
    GlfwContext& operator=(GlfwContext const&) = delete;
};

/**
 * @brief Window that supports Vulkan rendering
 */
class Window {
public:
    Window(GLFWwindow* window = nullptr) noexcept;
    ~Window() noexcept;

    Window(Window const&) = delete;
    Window& operator=(Window const&) = delete;

    Window(Window&& rhs) noexcept
        : window_(std::exchange(rhs.window_, nullptr)) {}

    Window& operator=(Window&& rhs) noexcept;

    [[nodiscard]] bool shouldClose() const noexcept {
        return glfwWindowShouldClose(window_) > 0;
    }

    [[nodiscard]] GLFWwindow* get() const noexcept { return window_; }

    /// Create a Vulkan-capable window.
    ///
    /// @throws std::runtime_error if the window could not be created
    static Window create(int width, int height);

private:
    GLFWwindow* window_;
};

} // namespace VK
