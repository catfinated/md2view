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

Window& fromGlfw(GLFWwindow* window) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    gsl_Assert(self != nullptr);
    return *self;
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
    : window_(window) {
    installCallbacks();
}

Window::Window(Window&& rhs) noexcept
    : window_(std::exchange(rhs.window_, nullptr))
    , input_(std::exchange(rhs.input_, nullptr))
    , onFramebufferResize_(std::move(rhs.onFramebufferResize_))
    , onWindowResize_(std::move(rhs.onWindowResize_)) {
    if (window_ != nullptr) {
        glfwSetWindowUserPointer(window_, this);
    }
}

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
        input_ = std::exchange(rhs.input_, nullptr);
        onFramebufferResize_ = std::move(rhs.onFramebufferResize_);
        onWindowResize_ = std::move(rhs.onWindowResize_);
        if (window_ != nullptr) {
            glfwSetWindowUserPointer(window_, this);
        }
    }
    return *this;
}

void Window::installCallbacks() noexcept {
    if (window_ == nullptr) {
        return;
    }

    glfwSetWindowUserPointer(window_, this);

    // NB: these must not throw since they are called from C. A missing
    // listener or handler simply drops the event.
    glfwSetKeyCallback(window_, [](GLFWwindow* window, int key, int scancode,
                                   int action, int mods) {
        if (auto* input = fromGlfw(window).input_; input != nullptr) {
            input->onKey(key, scancode, action, mods);
        }
    });
    glfwSetMouseButtonCallback(
        window_, [](GLFWwindow* window, int button, int action, int mods) {
            if (auto* input = fromGlfw(window).input_; input != nullptr) {
                input->onMouseButton(button, action, mods);
            }
        });
    glfwSetCursorPosCallback(
        window_, [](GLFWwindow* window, double xpos, double ypos) {
            if (auto* input = fromGlfw(window).input_; input != nullptr) {
                input->onCursorPos(xpos, ypos);
            }
        });
    glfwSetScrollCallback(
        window_, [](GLFWwindow* window, double xoffset, double yoffset) {
            if (auto* input = fromGlfw(window).input_; input != nullptr) {
                input->onScroll(xoffset, yoffset);
            }
        });
    glfwSetFramebufferSizeCallback(
        window_, [](GLFWwindow* window, int width, int height) {
            if (auto& handler = fromGlfw(window).onFramebufferResize_) {
                handler(width, height);
            }
        });
    glfwSetWindowSizeCallback(
        window_, [](GLFWwindow* window, int width, int height) {
            if (auto& handler = fromGlfw(window).onWindowResize_) {
                handler(width, height);
            }
        });
}

bool Window::shouldClose() const noexcept {
    return glfwWindowShouldClose(window_) == GLFW_TRUE;
}

void Window::requestClose() noexcept {
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void Window::setInputListener(InputListener* listener) noexcept {
    input_ = listener;
}

void Window::setFramebufferResizeHandler(ResizeHandler handler) {
    onFramebufferResize_ = std::move(handler);
}

void Window::setWindowResizeHandler(ResizeHandler handler) {
    onWindowResize_ = std::move(handler);
}

Window::Extent Window::size() const noexcept {
    Extent e{};
    glfwGetWindowSize(window_, &e.width, &e.height);
    return e;
}

Window::Extent Window::framebufferSize() const noexcept {
    Extent e{};
    glfwGetFramebufferSize(window_, &e.width, &e.height);
    return e;
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
