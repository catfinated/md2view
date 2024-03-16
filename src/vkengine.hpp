#pragma once

#include "engine.hpp"

#include "vk.hpp"

#include <optional>

namespace vk {

class VKEngine : public Engine
{
public:
    VKEngine();
    ~VKEngine();

    bool init(int argc, char const * argv[]);
    void run_game();

private:
    void initWindow();
    void initVulkan();
    void createGraphicsPipeline();
    void createRenderPass();
    void recordCommandBuffer(uint32_t imageIndex);
    void drawFrame();

    Window window_;
    Instance instance_;
    std::optional<DebugMessenger> debugMessenger_;
    std::optional<Surface> surface_;
    PhysicalDevice physicalDevice_;
    Device device_;
    std::optional<SwapChain> swapChain_;
    std::vector<ImageView> imageViews_;
    std::optional<InplaceRenderPass> renderPass_;
    std::optional<InplacePipelineLayout> pipelineLayout_;
    std::optional<InplacePipeline> graphicsPipeline_;
    std::vector<Framebuffer> frameBuffers_;
    std::optional<CommandPool> commandPool_;
    std::optional<CommandBuffer> commandBuffer_;
    std::optional<Semaphore> imageAvailableSemaphore_;
    std::optional<Semaphore> renderFinishedSemaphore_;
    std::optional<Fence> inflightFence_;
    VkViewport viewport{};
    VkRect2D scissor{};
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
};

}