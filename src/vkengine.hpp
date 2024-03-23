#pragma once

#include "engine.hpp"

#include "vk.hpp"

#include <optional>

namespace myvk {

class VKEngine : public Engine
{
public:
    VKEngine();
    ~VKEngine();

    bool init(int argc, char const * argv[]);
    void run_game();

private:
    static constexpr unsigned int kMaxFramesInFlight{2U};

    void initWindow();
    void initVulkan();
    void createGraphicsPipeline();
    void createRenderPass();
    void recordCommandBuffer(vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex);
    void drawFrame();
    void recreateSwapChain();

    Window window_;
    vk::raii::Context context_;
    vk::raii::Instance instance_{nullptr};
    vk::raii::DebugUtilsMessengerEXT debugMessenger_{nullptr};
    vk::raii::SurfaceKHR surface_{nullptr};
    vk::raii::PhysicalDevice physicalDevice_{nullptr};
    QueueFamilyIndices queueFamilyIndices_;
    vk::raii::Device device_{nullptr};
    vk::raii::Queue graphicsQueue_{nullptr};
    vk::raii::Queue presentQueue_{nullptr};
    vk::raii::SwapchainKHR swapChain_{nullptr};
    std::vector<vk::Image> swapChainImages_;
    SwapChainSupportDetails swapChainSupportDetails_;
    std::vector<vk::raii::ImageView> imageViews_;
    vk::raii::RenderPass renderPass_{nullptr};
    vk::raii::PipelineLayout pipelineLayout_{nullptr};
    vk::raii::Pipeline graphicsPipeline_{nullptr};
    std::vector<Framebuffer> frameBuffers_;

    vk::raii::CommandPool commandPool_{nullptr};
    std::vector<vk::raii::CommandBuffer> commandBuffers_;
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores_;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores_;
    std::vector<vk::raii::Fence> inflightFences_;

    VkViewport viewport{};
    VkRect2D scissor{};
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    uint32_t currentFrame_{0U};
    bool frameBufferResized_{false};
};

}