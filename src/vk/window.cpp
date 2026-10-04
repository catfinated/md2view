#include "md2view/vk/window.hpp"

#include <fmt/core.h>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <memory>
#include <stdexcept>
#include <utility>

namespace VK {

GlfwContext::GlfwContext() {
    if (glfwInit() != GLFW_TRUE) {
        char const* description = nullptr;
        auto const code = glfwGetError(&description);
        throw std::runtime_error(fmt::format(
            "failed to initialize GLFW: {} ({})",
            description != nullptr ? description : "unknown error", code));
    }
}

GlfwContext::~GlfwContext() { glfwTerminate(); }

Window::Window(GLFWwindow* window) noexcept
    : window_(window) {}

Window::~Window() noexcept {
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
    }
}

Window& Window::operator=(Window&& rhs) noexcept {
    if (this != std::addressof(rhs)) {
        if (window_ != nullptr) {
            glfwDestroyWindow(window_);
        }
        window_ = std::exchange(rhs.window_, nullptr);
    }
    return *this;
}

Window Window::create(int width, int height) {
    spdlog::info("create window");
    gsl_Expects(width > 0);
    gsl_Expects(height > 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto* window = glfwCreateWindow(width, height, "vkmd2v", nullptr, nullptr);

    if (window == nullptr) {
        // GLFW keeps the reason for the last failure; it is far more useful
        // than a generic message (no Vulkan loader, no display, ...)
        char const* description = nullptr;
        auto const code = glfwGetError(&description);
        throw std::runtime_error(fmt::format(
            "failed to create window: {} ({})",
            description != nullptr ? description : "unknown error", code));
    }

    return Window{window};
}

} // namespace VK
