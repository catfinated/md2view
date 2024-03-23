#include "vkengine.hpp"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace myvk {

template <class T, class E>
T forceUnwrap(tl::expected<T, E>&& expectedT)
{
    if (!expectedT) {
        throw expectedT.error();
    }
    return std::move(expectedT).value();
}

VKEngine::VKEngine()
{
    gsl_Ensures(glfwInit() == GLFW_TRUE);
}

VKEngine::~VKEngine()
{
    glfwTerminate();
}

bool VKEngine::init(int argc, char const * argv[])
{
    return parse_args(argc, argv);
}

void VKEngine::initWindow()
{
    window_ = forceUnwrap(Window::create(width_, height_));    

    auto framebufferResizeCallback = [](GLFWwindow * window, int width, int height) {
        VKEngine * engine = static_cast<VKEngine *>(glfwGetWindowUserPointer(window));
        gsl_Assert(engine);
        engine->frameBufferResized_ = true;
    };

    glfwSetWindowUserPointer(window_.get(), this);
    glfwSetFramebufferSizeCallback(window_.get(), framebufferResizeCallback);
}

void VKEngine::initVulkan()
{
    instance_ = forceUnwrap(createInstance(context_));
    debugMessenger_ = forceUnwrap(createDebugUtilsMessenger(instance_));
    surface_ = forceUnwrap(createSurface(instance_, window_));
    std::tie(physicalDevice_, queueFamilyIndices_) = forceUnwrap(pickPhysicalDevice(instance_, *surface_));
    device_ = forceUnwrap(createDevice(physicalDevice_, queueFamilyIndices_));

    graphicsQueue_ = device_.getQueue(queueFamilyIndices_.graphicsFamily.value(), 0);
    presentQueue_ = device_.getQueue(queueFamilyIndices_.presentFamily.value(), 0);

    std::tie(swapChain_, swapChainSupportDetails_) = forceUnwrap(createSwapChain(physicalDevice_, device_, window_, *surface_));
    swapChainImages_ = swapChain_.getImages();
    imageViews_ = forceUnwrap(createImageViews(device_, swapChainImages_, swapChainSupportDetails_));

    createRenderPass();
    createGraphicsPipeline();
    frameBuffers_ = forceUnwrap(Framebuffer::create(imageViews_, renderPass_, swapChainSupportDetails_.extent, *device_));
    commandPool_ = forceUnwrap(createCommandPool(device_, queueFamilyIndices_));

    vk::CommandBufferAllocateInfo allocInfo{*commandPool_, vk::CommandBufferLevel::ePrimary, kMaxFramesInFlight};
    commandBuffers_ = device_.allocateCommandBuffers(allocInfo);
    imageAvailableSemaphores_ = forceUnwrap(createSemaphores(device_, kMaxFramesInFlight));
    renderFinishedSemaphores_ = forceUnwrap(createSemaphores(device_, kMaxFramesInFlight));
    inflightFences_ = forceUnwrap(createFences(device_, kMaxFramesInFlight));

    spdlog::info("vulkan initialization complete. num views={}", imageViews_.size());
}

void VKEngine::recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window_.get(), &width, &height);
    while (width == 0 || height == 0) {
        if (window_.shouldClose()) return;
        glfwGetFramebufferSize(window_.get(), &width, &height);
        glfwWaitEvents();
    }

    device_.waitIdle();
    frameBuffers_.clear();
    imageViews_.clear();
    swapChain_ = nullptr; // NB: must destroy previous swap chain first!

    std::tie(swapChain_, swapChainSupportDetails_) = forceUnwrap(createSwapChain(physicalDevice_, device_, window_, *surface_));
    swapChainImages_ = swapChain_.getImages();
    imageViews_ = forceUnwrap(createImageViews(device_, swapChainImages_, swapChainSupportDetails_));
    frameBuffers_ = forceUnwrap(Framebuffer::create(imageViews_, renderPass_, swapChainSupportDetails_.extent, *device_));
    spdlog::info("recreated swap chain");
}

void VKEngine::createGraphicsPipeline()
{
    spdlog::info("creating graphics pipeline");
    auto vertShaderModule = forceUnwrap(createShaderModule("data/shaders/vert.spv", device_));
    auto fragShaderModule = forceUnwrap(createShaderModule("data/shaders/frag.spv", device_));

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex; // VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = *vertShaderModule;
    vertShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment; //  VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = *fragShaderModule;
    fragShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList; // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = vk::False; // VK_FALSE;

    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.depthClampEnable = vk::False; // VK_FALSE;
    rasterizer.rasterizerDiscardEnable = vk::False; // VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill; // VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack; // VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = vk::FrontFace::eClockwise; // VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = vk::False; // VK_FALSE;

    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sampleShadingEnable = vk::False; // VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1; // VK_SAMPLE_COUNT_1_BIT;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
                                          vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB |
                                          vk::ColorComponentFlagBits::eA; 
    //VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = vk::False; // VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.logicOpEnable = vk::False; // VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy; // VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport, // VK_DYNAMIC_STATE_VIEWPORT,
        vk::DynamicState::eScissor // VK_DYNAMIC_STATE_SCISSOR
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{{}, static_cast<uint32_t>(dynamicStates.size()), dynamicStates.data()};
    //dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    //dynamicState.pDynamicStates = dynamicStates.data();

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    pipelineLayout_ = device_.createPipelineLayout(pipelineLayoutInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = *pipelineLayout_;
    pipelineInfo.renderPass = *renderPass_;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    graphicsPipeline_ = device_.createGraphicsPipeline(nullptr, pipelineInfo);
}

void VKEngine::createRenderPass() 
{
    spdlog::info("create render pass");
    vk::AttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainSupportDetails_.surfaceFormat.format;
    colorAttachment.samples = vk::SampleCountFlagBits::e1; // VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear; // VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore; // VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare; // VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare; // VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined; // VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR; // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    vk::AttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal; // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics; // VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::RenderPassCreateInfo renderPassInfo{};
    //renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;  VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    vk::SubpassDependency dependency{};
    dependency.srcSubpass = vk::SubpassExternal; // VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput; // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = vk::AccessFlagBits::eNone;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput; //  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; // VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    renderPass_ = device_.createRenderPass(renderPassInfo);
}

void VKEngine::recordCommandBuffer(vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(*commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = *renderPass_;
    renderPassInfo.framebuffer = frameBuffers_.at(imageIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainSupportDetails_.extent;

    clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(*commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *graphicsPipeline_);

    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainSupportDetails_.extent.width);
    viewport.height = static_cast<float>(swapChainSupportDetails_.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(*commandBuffer, 0, 1, &viewport);

    scissor.offset = {0, 0};
    scissor.extent = swapChainSupportDetails_.extent;
    vkCmdSetScissor(*commandBuffer, 0, 1, &scissor);

    vkCmdDraw(*commandBuffer, 3, 1, 0, 0); 

    vkCmdEndRenderPass(*commandBuffer); 

    if (vkEndCommandBuffer(*commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    } 
}

void VKEngine::drawFrame()
{
    auto& fence = inflightFences_.at(currentFrame_);
    gsl_Assert(device_.waitForFences({*fence}, true, UINT64_MAX) == vk::Result::eSuccess);
    device_.resetFences({*fence});

    auto& imageAvailableSemaphore = imageAvailableSemaphores_.at(currentFrame_);
    uint32_t imageIndex;
    vkAcquireNextImageKHR(*device_, *swapChain_, UINT64_MAX, *imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    auto& commandBuffer = commandBuffers_.at(currentFrame_);
    commandBuffer.reset();
    recordCommandBuffer(commandBuffer, imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {*imageAvailableSemaphore};
    VkCommandBuffer commandBuffers[] = {*commandBuffer};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = commandBuffers;

    auto& renderFinishedSemaphore = renderFinishedSemaphores_.at(currentFrame_);
    VkSemaphore signalSemaphores[] = {*renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(*graphicsQueue_, 1, &submitInfo, *fence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {*swapChain_};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    auto const result = vkQueuePresentKHR(*presentQueue_, &presentInfo);
    currentFrame_ = (currentFrame_ + 1U) % kMaxFramesInFlight;

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frameBufferResized_) {
        frameBufferResized_ = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
}

void VKEngine::run_game()
{
    initWindow();
    initVulkan();

    while(!window_.shouldClose()) {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(*device_);

    glfwTerminate();
}

}