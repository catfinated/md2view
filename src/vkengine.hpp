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
    void recordCommandBuffer(CommandBuffer& commandBuffer, uint32_t imageIndex);
    void drawFrame();
    void recreateSwapChain();

    Window window_;
    vk::raii::Context context_;
    vk::raii::Instance instance_;
    vk::raii::DebugUtilsMessengerEXT debugMessenger_;
    vk::raii::SurfaceKHR surface_;
    vk::raii::PhysicalDevice physicalDevice_;
    QueueFamilyIndices queueFamilyIndices_;
    vk::raii::Device device_;
    vk::raii::Queue graphicsQueue_{nullptr};
    vk::raii::Queue presentQueue_{nullptr};


    std::optional<SwapChain> swapChain_;
    std::vector<ImageView> imageViews_;
    std::optional<InplaceRenderPass> renderPass_;
    std::optional<InplacePipelineLayout> pipelineLayout_;
    std::optional<InplacePipeline> graphicsPipeline_;
    std::vector<Framebuffer> frameBuffers_;
    std::optional<CommandPool> commandPool_;
    CommandBufferVec commandBuffers_;

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