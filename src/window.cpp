#include "md2view/window.hpp"

#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

std::string lastGlfwError() {
    char const* description = nullptr;
    auto const code = glfwGetError(&description);
    return fmt::format("{} ({})",
                       description != nullptr ? description : "unknown error",
                       code);
}

} // namespace

GlfwContext::GlfwContext() {
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error(
            fmt::format("failed to initialize GLFW: {}", lastGlfwError()));
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

bool Window::shouldClose() const noexcept {
    return glfwWindowShouldClose(window_) == GLFW_TRUE;
}

Window Window::create(int width,
                      int height,
                      char const* title,
                      std::span<WindowHint const> hints) {
    spdlog::info("create window");
    gsl_Expects(width > 0);
    gsl_Expects(height > 0);

    glfwDefaultWindowHints();
    for (auto const& [hint, value] : hints) {
        glfwWindowHint(hint, value);
    }

    auto* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window == nullptr) {
        throw std::runtime_error(
            fmt::format("failed to create window: {}", lastGlfwError()));
    }

    return Window{window};
}
