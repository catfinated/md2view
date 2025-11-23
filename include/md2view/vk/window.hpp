/**
 * @brief Types and functions related to windowing
 */
#pragma once

#include <GLFW/glfw3.h>

#include <expected>
#include <stdexcept>
#include <utility>

namespace md2v {

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

    static std::expected<Window, std::runtime_error>
    create(int width, int height) noexcept;

private:
    GLFWwindow* window_;
};

} // namespace md2v
