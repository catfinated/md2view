#pragma once

#include <GLFW/glfw3.h>
#include <gsl/gsl-lite.hpp>
#include <tl/expected.hpp>

#include <cstdint>
#include <filesystem>
#include <set>
#include <stdexcept>
#include <optional>
#include <utility>
#include <vector>

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

    [[nodiscard]] GLFWwindow * get() const noexcept { return window_; }

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

class Surface
{
public:
    ~Surface() noexcept;
    Surface(Surface const&) = delete;
    Surface& operator=(Surface const&) = delete;
    Surface(Surface&& rhs) noexcept
        : instance_(rhs.instance_) 
        , surface_(std::exchange(rhs.surface_, std::nullopt))
    {}
    Surface& operator=(Surface&& rhs) noexcept;

    operator VkSurfaceKHR() const 
    {
        gsl_Assert(surface_);
        return *surface_;
    }

    static tl::expected<Surface, std::runtime_error> create(Instance const& instance, Window const& window) noexcept;

private:
    Surface(Instance const& instance, VkSurfaceKHR surface)
        : instance_(std::addressof(instance))
        , surface_(surface)
    {}

    gsl::not_null<Instance const*> instance_;
    std::optional<VkSurfaceKHR> surface_;
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
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
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

    std::set<uint32_t> uniqueQueueFamilies() const
    {
        gsl_Expects(queueFamilyIndices().isComplete());
        return {indices_.graphicsFamily.value(), indices_.presentFamily.value()};
    }

    SwapChainSupportDetails querySwapChainSupport(Surface const& surface) const noexcept;

    static tl::expected<PhysicalDevice, std::runtime_error> 
    pickPhysicalDevice(Instance const& instance, Surface const& surface) noexcept;

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

    operator VkDevice() const
    {
        gsl_Assert(device_);
        return *device_;
    }

    static tl::expected<Device, std::runtime_error> create(PhysicalDevice const& physicalDevice) noexcept;

private:
    Device(VkDevice device, PhysicalDevice const& physicalDevice) noexcept;
    std::optional<VkDevice> device_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;
};

class ImageView
{
public:
    ImageView(VkImageView, Device const& device) noexcept;
    ~ImageView() noexcept;
    ImageView(ImageView const&) = delete;
    ImageView& operator=(ImageView const&) = delete;
    ImageView(ImageView&&) noexcept;
    ImageView& operator=(ImageView&&) noexcept;

    operator VkImageView() const
    {
        gsl_Assert(view_);
        return *view_;
    }
    
private:
    std::optional<VkImageView> view_;
    gsl::not_null<Device const*> device_;
};

class SwapChain
{
public:
    ~SwapChain() noexcept;
    SwapChain(SwapChain const&) = delete;
    SwapChain& operator=(SwapChain const&) = delete;
    SwapChain(SwapChain&& rhs) noexcept;
    SwapChain& operator=(SwapChain&& rhs) noexcept;

    operator VkSwapchainKHR() const 
    {
        gsl_Assert(swapChain_);
        return *swapChain_; 
    }

    VkExtent2D extent() const noexcept { return swapChainExtent_; }
    VkFormat imageFormat() const noexcept { return swapChainImageFormat_; }

    tl::expected<std::vector<ImageView>, std::runtime_error> createImageViews(Device const& device) const;

    static tl::expected<SwapChain, std::runtime_error>
    create(PhysicalDevice const& physicalDevice, Device const& device, Window const& window, Surface const& surface) noexcept;

private:
    SwapChain(VkSwapchainKHR swapChain, 
              VkFormat swapChainImageFormat,
              VkExtent2D swapChainExtent,
              std::vector<VkImage> images, 
              Device const& device) noexcept;

    std::optional<VkSwapchainKHR> swapChain_;
    std::vector<VkImage> images_;
    VkFormat swapChainImageFormat_;
    VkExtent2D swapChainExtent_;
    gsl::not_null<Device const*> device_;
};

class ShaderModule 
{
public:
    ~ShaderModule() noexcept;
    ShaderModule(ShaderModule const&) = delete;
    ShaderModule& operator=(ShaderModule const&) = delete;
    ShaderModule(ShaderModule&&) noexcept;
    ShaderModule& operator=(ShaderModule&&) noexcept;

    operator VkShaderModule() const
    {
        gsl_Assert(module_);
        return *module_;
    }

    static tl::expected<ShaderModule, std::runtime_error> 
    create(std::filesystem::path const& path, Device const& device);
                                                    
private:
    ShaderModule(VkShaderModule module, Device const& device) noexcept;

    std::optional<VkShaderModule> module_;
    gsl::not_null<Device const*> device_;
};

class InplaceRenderPass
{
public:
    InplaceRenderPass(VkRenderPass renderPass, Device const& device)
        : renderPass_(renderPass)
        , device_(std::addressof(device))
    {}

    ~InplaceRenderPass()
    {
        vkDestroyRenderPass(*device_, renderPass_, nullptr);
    }

    InplaceRenderPass(InplaceRenderPass const&) = delete;
    InplaceRenderPass& operator=(InplaceRenderPass const&) = delete;
    InplaceRenderPass(InplaceRenderPass&&) = delete;
    InplaceRenderPass& operator=(InplaceRenderPass&&) = delete;

    operator VkRenderPass() const { return renderPass_; }

private:
    VkRenderPass renderPass_;
    gsl::not_null<Device const*> device_;
};

class InplacePipelineLayout
{
public:
    InplacePipelineLayout(VkPipelineLayout pipelineLayout, Device const& device)
        : pipelineLayout_(pipelineLayout)
        , device_(std::addressof(device))
    {}

    ~InplacePipelineLayout()
    {
        vkDestroyPipelineLayout(*device_, pipelineLayout_, nullptr);
    }

    InplacePipelineLayout(InplacePipelineLayout const&) = delete;
    InplacePipelineLayout& operator=(InplacePipelineLayout const&) = delete;
    InplacePipelineLayout(InplacePipelineLayout&&) = delete;
    InplacePipelineLayout& operator=(InplacePipelineLayout&&) = delete;

    operator VkPipelineLayout() const { return pipelineLayout_; }

private:
    VkPipelineLayout pipelineLayout_;
    gsl::not_null<Device const*> device_;
};

class InplacePipeline
{
public:
    InplacePipeline(VkPipeline pipeline, Device const& device)
        : pipeline_(pipeline)
        , device_(std::addressof(device))
    {}

    ~InplacePipeline()
    {
        vkDestroyPipeline(*device_, pipeline_, nullptr);
    }

    InplacePipeline(InplacePipeline const&) = delete;
    InplacePipeline& operator=(InplacePipeline const&) = delete;
    InplacePipeline(InplacePipeline&&) = delete;
    InplacePipeline& operator=(InplacePipeline&&) = delete;

    operator VkPipeline() const { return pipeline_; }

private:
    VkPipeline pipeline_;
    gsl::not_null<Device const*> device_;
};

class Framebuffer 
{
public:
    Framebuffer(VkFramebuffer buffer, Device const& device) noexcept;
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
    create(std::vector<ImageView> const& imageViews, 
           InplaceRenderPass const& renderPass, 
           VkExtent2D swapChainExtent,
           Device const& device) noexcept;

private:
    std::optional<VkFramebuffer> buffer_;
    gsl::not_null<Device const*> device_;
};

class CommandBuffer
{
public:
    CommandBuffer(VkCommandBuffer buffer)
        : buffer_(buffer)
    {}

    operator VkCommandBuffer() const { return buffer_; }

private:
    VkCommandBuffer buffer_;
};

class CommandPool
{
public:
    ~CommandPool() noexcept;
    CommandPool(CommandPool const&) = delete;
    CommandPool& operator=(CommandPool const&) = delete;
    CommandPool(CommandPool&&) noexcept;
    CommandPool& operator=(CommandPool&&) noexcept;

    operator VkCommandPool() const
    {
        gsl_Assert(pool_);
        return *pool_;
    }

    tl::expected<CommandBuffer, std::runtime_error> createBuffer() const noexcept;

    static tl::expected<CommandPool, std::runtime_error> 
    create(PhysicalDevice const& physicalDevice, Device const& device) noexcept;

private:
    CommandPool(VkCommandPool pool, Device const& device) noexcept;

    std::optional<VkCommandPool> pool_;
    gsl::not_null<Device const*> device_;
};

} // namespace vk 
