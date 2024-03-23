#pragma once

#include <GLFW/glfw3.h>
#include <gsl/gsl-lite.hpp>
#include <tl/expected.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <filesystem>
#include <set>
#include <stdexcept>
#include <optional>
#include <utility>
#include <vector>

namespace myvk {

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

    [[nodiscard]] GLFWwindow * get() const noexcept { return window_; }

    static tl::expected<Window, std::runtime_error> create(int width, int height) noexcept;

private:
    GLFWwindow * window_;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    [[nodiscard]] bool isComplete() const noexcept
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;

    vk::SurfaceFormatKHR surfaceFormat;
    vk::Extent2D extent;
};

[[nodiscard]] SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice, vk::SurfaceKHR const& surface) noexcept;

tl::expected<vk::raii::Instance, std::runtime_error> createInstance(vk::raii::Context&) noexcept;

tl::expected<vk::raii::DebugUtilsMessengerEXT, std::runtime_error> createDebugUtilsMessenger(vk::raii::Instance&) noexcept;

tl::expected<vk::raii::SurfaceKHR, std::runtime_error> createSurface(vk::raii::Instance&, Window const&) noexcept;

tl::expected<std::pair<vk::raii::PhysicalDevice, QueueFamilyIndices>, std::runtime_error> 
    pickPhysicalDevice(vk::raii::Instance&, vk::SurfaceKHR const&) noexcept;

tl::expected<vk::raii::Device, std::runtime_error> 
    createDevice(vk::raii::PhysicalDevice const& physicalDevice, QueueFamilyIndices const&) noexcept;

tl::expected<std::vector<vk::raii::Semaphore>, std::runtime_error>
    createSemaphores(vk::raii::Device const& device, unsigned int numSemaphores) noexcept;

tl::expected<std::vector<vk::raii::Fence>, std::runtime_error>
    createFences(vk::raii::Device const& device, unsigned int numFences) noexcept;

tl::expected<vk::raii::CommandPool, std::runtime_error> 
    createCommandPool(vk::raii::Device const& device, QueueFamilyIndices const& indices) noexcept;

tl::expected<std::pair<vk::raii::SwapchainKHR, SwapChainSupportDetails>, std::runtime_error>
    createSwapChain(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::Device const& device, Window const& window, vk::SurfaceKHR const& surface) noexcept;

 tl::expected<std::vector<vk::raii::ImageView>, std::runtime_error> 
    createImageViews(vk::raii::Device const& device, std::vector<vk::Image>& images, SwapChainSupportDetails const& support);

tl::expected<vk::raii::ShaderModule, std::runtime_error> 
    createShaderModule(std::filesystem::path const& path, vk::raii::Device const& device) noexcept;

class Framebuffer 
{
public:
    Framebuffer(VkFramebuffer buffer, vk::Device const& device) noexcept;
    ~Framebuffer() noexcept;
    Framebuffer(Framebuffer const&) = delete;
    Framebuffer& operator=(Framebuffer const&) = delete;
    Framebuffer(Framebuffer&&) noexcept;
    Framebuffer& operator=(Framebuffer&&) noexcept;

    operator VkFramebuffer() const
    {
        gsl_Assert(buffer_);
        return *buffer_;
    }

    static tl::expected<std::vector<Framebuffer>, std::runtime_error>
    create(std::vector<vk::raii::ImageView> const& imageViews, 
           vk::raii::RenderPass const& renderPass, 
           vk::Extent2D swapChainExtent,
           vk::Device const& device) noexcept;

private:
    std::optional<VkFramebuffer> buffer_;
    vk::Device device_;
};

} // namespace vk 

