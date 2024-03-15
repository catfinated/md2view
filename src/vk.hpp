#pragma once

#include <GLFW/glfw3.h>
#include <gsl/gsl-lite.hpp>
#include <tl/expected.hpp>

#include <cstdint>
#include <stdexcept>
#include <optional>
#include <utility>

namespace vk {

class Window 
{
public:
    Window(GLFWwindow * window = nullptr) noexcept;
    ~Window() noexcept;

    Window(Window const&) = delete;
    Window& operator=(Window const&) = delete;

    Window(Window&& rhs) noexcept
        : window_(std::exchange(rhs.window_, nullptr))
    {}

    Window& operator=(Window&& rhs) noexcept;

    [[nodiscard]] bool shouldClose() const noexcept 
    {
        return glfwWindowShouldClose(window_) > 0;
    }

    static tl::expected<Window, std::runtime_error> create(int width, int height) noexcept;

private:
    GLFWwindow * window_;
};

class Instance
{
public:
    Instance() = default;
    ~Instance() noexcept;
    Instance(Instance const&) = delete;
    Instance& operator=(Instance const&) = delete;

    Instance(Instance&& rhs) noexcept
        : handle_(std::exchange(rhs.handle_, std::nullopt))
    {}

    Instance& operator=(Instance&& rhs) noexcept;

    operator VkInstance() const 
    {
        gsl_Expects(handle_); 
        return *handle_; 
    }

    static tl::expected<Instance, std::runtime_error> create() noexcept;

private:
    Instance(VkInstance handle) noexcept;

    std::optional<VkInstance> handle_;
};

class DebugMessenger 
{
public:
    ~DebugMessenger() noexcept;
    DebugMessenger(DebugMessenger const&) = delete;
    DebugMessenger& operator=(DebugMessenger const&) = delete;

    DebugMessenger(DebugMessenger&& rhs) noexcept 
        : instance_(rhs.instance_)
        , handle_(std::exchange(rhs.handle_, std::nullopt))
    {}

    DebugMessenger& operator=(DebugMessenger&& rhs) noexcept;

    operator VkDebugUtilsMessengerEXT() const 
    {
        gsl_Expects(handle_); 
        return *handle_; 
    }

     static tl::expected<DebugMessenger, std::runtime_error> create(Instance const&) noexcept;

private:
    DebugMessenger(Instance const&, VkDebugUtilsMessengerEXT handle) noexcept;

    gsl::not_null<Instance const*> instance_;
    std::optional<VkDebugUtilsMessengerEXT> handle_;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    [[nodiscard]] bool isComplete() const noexcept
    {
        return graphicsFamily.has_value();
    }
};

class PhysicalDevice
{
public:
    PhysicalDevice() = default;
    PhysicalDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndices indices) noexcept
        : physicalDevice_(physicalDevice)
        , indices_(std::move(indices))
    {}

    operator VkPhysicalDevice() const 
    {
        return physicalDevice_;
    }

    QueueFamilyIndices const& queueFamilyIndices() const noexcept { return indices_; }

    static tl::expected<PhysicalDevice, std::runtime_error> pickPhysicalDevice(Instance const& instance) noexcept;

private:
    VkPhysicalDevice physicalDevice_;
    QueueFamilyIndices indices_;
};

class Device 
{
public:
    Device() = default;
    ~Device() noexcept;
    Device(Device const&) = delete;
    Device& operator=(Device const&) = delete;

    Device(Device&& rhs) noexcept
        : device_(std::exchange(rhs.device_, std::nullopt))
        , graphicsQueue_(rhs.graphicsQueue_)
    {}

    Device& operator=(Device&& rhs) noexcept;

    static tl::expected<Device, std::runtime_error> create(PhysicalDevice const& physicalDevice) noexcept;

private:
    Device(VkDevice device, PhysicalDevice const& physicalDevice) noexcept;
    std::optional<VkDevice> device_;
    VkQueue graphicsQueue_;
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) noexcept;



} // namespace vk 
