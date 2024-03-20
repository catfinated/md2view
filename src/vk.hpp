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
};

tl::expected<vk::raii::Instance, std::runtime_error> createInstance(vk::raii::Context&) noexcept;
tl::expected<vk::raii::DebugUtilsMessengerEXT, std::runtime_error> createDebugUtilsMessenger(vk::raii::Instance&) noexcept;
tl::expected<vk::raii::SurfaceKHR, std::runtime_error> createSurface(vk::raii::Instance&, Window const&) noexcept;
tl::expected<std::pair<vk::raii::PhysicalDevice, QueueFamilyIndices>, std::runtime_error> 
pickPhysicalDevice(vk::raii::Instance&, vk::SurfaceKHR const&) noexcept;
tl::expected<vk::raii::Device, std::runtime_error> createDevice(vk::raii::PhysicalDevice const& physicalDevice, QueueFamilyIndices const&) noexcept;

class ImageView
{
public:
    ImageView(VkImageView, vk::Device const& device) noexcept;
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
    vk::Device device_;
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

    tl::expected<std::vector<ImageView>, std::runtime_error> createImageViews(vk::Device const& device) const;

    static tl::expected<SwapChain, std::runtime_error>
    create(vk::PhysicalDevice const& physicalDevice, vk::Device const& device, Window const& window, vk::SurfaceKHR const& surface) noexcept;

private:
    SwapChain(VkSwapchainKHR swapChain, 
              VkFormat swapChainImageFormat,
              VkExtent2D swapChainExtent,
              std::vector<VkImage> images, 
              vk::Device const& device) noexcept;

    std::optional<VkSwapchainKHR> swapChain_;
    std::vector<VkImage> images_;
    VkFormat swapChainImageFormat_;
    VkExtent2D swapChainExtent_;
    vk::Device device_;
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
    create(std::filesystem::path const& path, vk::Device const& device);
                                                    
private:
    ShaderModule(VkShaderModule module, vk::Device const& device) noexcept;

    std::optional<VkShaderModule> module_;
    vk::Device device_;
};

class InplaceRenderPass
{
public:
    InplaceRenderPass(VkRenderPass renderPass, vk::Device const& device)
        : renderPass_(renderPass)
        , device_(device)
    {}

    ~InplaceRenderPass()
    {
        vkDestroyRenderPass(device_, renderPass_, nullptr);
    }

    InplaceRenderPass(InplaceRenderPass const&) = delete;
    InplaceRenderPass& operator=(InplaceRenderPass const&) = delete;
    InplaceRenderPass(InplaceRenderPass&&) = delete;
    InplaceRenderPass& operator=(InplaceRenderPass&&) = delete;

    operator VkRenderPass() const { return renderPass_; }

private:
    VkRenderPass renderPass_;
    vk::Device device_;
};

class InplacePipelineLayout
{
public:
    InplacePipelineLayout(VkPipelineLayout pipelineLayout, vk::Device const& device)
        : pipelineLayout_(pipelineLayout)
        , device_(device)
    {}

    ~InplacePipelineLayout()
    {
        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
    }

    InplacePipelineLayout(InplacePipelineLayout const&) = delete;
    InplacePipelineLayout& operator=(InplacePipelineLayout const&) = delete;
    InplacePipelineLayout(InplacePipelineLayout&&) = delete;
    InplacePipelineLayout& operator=(InplacePipelineLayout&&) = delete;

    operator VkPipelineLayout() const { return pipelineLayout_; }

private:
    VkPipelineLayout pipelineLayout_;
    vk::Device device_;
};

class InplacePipeline
{
public:
    InplacePipeline(VkPipeline pipeline, vk::Device const& device)
        : pipeline_(pipeline)
        , device_(device)
    {}

    ~InplacePipeline()
    {
        vkDestroyPipeline(device_, pipeline_, nullptr);
    }

    InplacePipeline(InplacePipeline const&) = delete;
    InplacePipeline& operator=(InplacePipeline const&) = delete;
    InplacePipeline(InplacePipeline&&) = delete;
    InplacePipeline& operator=(InplacePipeline&&) = delete;

    operator VkPipeline() const { return pipeline_; }

private:
    VkPipeline pipeline_;
    vk::Device device_;
};

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
    create(std::vector<ImageView> const& imageViews, 
           InplaceRenderPass const& renderPass, 
           VkExtent2D swapChainExtent,
           vk::Device const& device) noexcept;

private:
    std::optional<VkFramebuffer> buffer_;
    vk::Device device_;
};

class CommandBuffer
{
public:
    CommandBuffer(VkCommandBuffer buffer)
        : buffer_(buffer)
    {}

    operator VkCommandBuffer() const { return buffer_; }

    VkCommandBuffer const * toPtr() const { return std::addressof(buffer_); }

private:
    VkCommandBuffer buffer_;
};

using CommandBufferVec = std::vector<VkCommandBuffer>;

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

    tl::expected<CommandBufferVec, std::runtime_error> createBuffers(unsigned int numBuffers) const noexcept;

    static tl::expected<CommandPool, std::runtime_error> 
    create(vk::PhysicalDevice const& physicalDevice, vk::Device const& device, QueueFamilyIndices const& indices) noexcept;

private:
    CommandPool(VkCommandPool pool, vk::Device const& device) noexcept;

    std::optional<VkCommandPool> pool_;
    vk::Device device_;
};

class Fence
{
public:
    ~Fence() noexcept;
    Fence(Fence const&) = delete;
    Fence& operator=(Fence const&) = delete;
    Fence(Fence&&) noexcept;
    Fence& operator=(Fence&&) noexcept;

    operator VkFence() const
    {
        gsl_Assert(fence_);
        return *fence_;
    }

    void wait() const
    {
        vkWaitForFences(device_, 1, std::addressof(fence_.value()), VK_TRUE, UINT64_MAX);
    }

    void reset()
    {
        vkResetFences(device_, 1, std::addressof(fence_.value()));
    }

    static tl::expected<std::vector<Fence>, std::runtime_error>
    createVec(vk::Device const& device, unsigned int numFences) noexcept;

    static tl::expected<Fence, std::runtime_error> create(vk::Device const& device) noexcept;

private:
    Fence(VkFence fence, vk::Device const& device) noexcept;

    std::optional<VkFence> fence_;
    vk::Device device_;
};

class Semaphore
{
public:
    ~Semaphore() noexcept;
    Semaphore(Semaphore const&) = delete;
    Semaphore& operator=(Semaphore const&) = delete;
    Semaphore(Semaphore&&) noexcept;
    Semaphore& operator=(Semaphore&&) noexcept;

    operator VkSemaphore() const 
    {
        gsl_Assert(semaphore_);
        return *semaphore_;
    }

    static tl::expected<std::vector<Semaphore>, std::runtime_error>
    createVec(vk::Device const& device, unsigned int numSemaphores) noexcept;

    static tl::expected<Semaphore, std::runtime_error> create(vk::Device const& device) noexcept;

private:
    Semaphore(VkSemaphore semaphore, vk::Device const& device) noexcept;

    std::optional<VkSemaphore> semaphore_;
    vk::Device device_;
};

} // namespace vk 
