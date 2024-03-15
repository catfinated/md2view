#pragma once

#include <GLFW/glfw3.h>
#include <gsl/gsl-lite.hpp>
#include <tl/expected.hpp>

#include <stdexcept>
#include <optional>
#include <utility>

namespace vk {

class Window 
{
public:
    Window(GLFWwindow * window = nullptr);
    ~Window();

    Window(Window const&) = delete;
    Window& operator=(Window const&) = delete;

    Window(Window&& rhs) noexcept
        : window_(std::exchange(rhs.window_, nullptr))
    {}

    Window& operator=(Window&& rhs) noexcept 
    {
        if (this != std::addressof(rhs)) {
            window_ = std::exchange(rhs.window_, nullptr);
        }
        return *this;
    }

    [[nodiscard]] bool shouldClose() const noexcept 
    {
        return glfwWindowShouldClose(window_) > 0;
    }

    static tl::expected<Window, std::runtime_error> create(int width, int height);

private:
    GLFWwindow * window_;
};

class Instance
{
public:
    Instance() = default;
    ~Instance();
    Instance(Instance const&) = delete;
    Instance& operator=(Instance const&) = delete;

    Instance(Instance&& rhs) noexcept
        : handle_(std::exchange(rhs.handle_, std::nullopt))
    {}

    Instance& operator=(Instance&& rhs) noexcept 
    {
        if (this != std::addressof(rhs)) {
            handle_ = std::exchange(rhs.handle_, std::nullopt);
        }
        return *this;
    }

    operator VkInstance() const 
    {
        gsl_Expects(handle_); 
        return *handle_; 
    }

    static tl::expected<Instance, std::runtime_error> create();

private:
    Instance(VkInstance handle);

    std::optional<VkInstance> handle_;
};

class DebugMessenger 
{
public:
    ~DebugMessenger();
    DebugMessenger(DebugMessenger const&) = delete;
    DebugMessenger& operator=(DebugMessenger const&) = delete;

    DebugMessenger(DebugMessenger&& rhs) noexcept 
        : instance_(rhs.instance_)
        , handle_(std::exchange(rhs.handle_, std::nullopt))
    {}

    DebugMessenger& operator=(DebugMessenger&& rhs) noexcept
    {
        if (this != std::addressof(rhs)) {
            instance_ = rhs.instance_;
            handle_ = std::exchange(rhs.handle_, std::nullopt);
        }
        return *this;
    }

    operator VkDebugUtilsMessengerEXT() const 
    {
        gsl_Expects(handle_); 
        return *handle_; 
    }

     static tl::expected<DebugMessenger, std::runtime_error> create(Instance&);

private:
    DebugMessenger(Instance&, VkDebugUtilsMessengerEXT handle);

    gsl::not_null<Instance*> instance_;
    std::optional<VkDebugUtilsMessengerEXT> handle_;
};

} // namespace v 
