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
    : instance_(nullptr)
    , debugMessenger_(nullptr)
    , surface_(nullptr)
    , physicalDevice_(nullptr)
    , device_(nullptr)
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
    auto result = forceUnwrap(pickPhysicalDevice(instance_, *surface_));
    physicalDevice_ = std::move(result.first);
    queueFamilyIndices_ = result.second;
    device_ = forceUnwrap(createDevice(physicalDevice_, queueFamilyIndices_));

    graphicsQueue_ = device_.getQueue(queueFamilyIndices_.graphicsFamily.value(), 0);
    presentQueue_ = device_.getQueue(queueFamilyIndices_.presentFamily.value(), 0);

    std::tie(swapChain_, swapChainSupportDetails_) = forceUnwrap(createSwapChain(physicalDevice_, device_, window_, *surface_));
    swapChainImages_ = swapChain_.getImages();
    imageViews_ = forceUnwrap(createImageViews(device_, swapChainImages_, swapChainSupportDetails_));

    createRenderPass();
    createGraphicsPipeline();
    frameBuffers_ = forceUnwrap(Framebuffer::create(imageViews_, *renderPass_, swapChainSupportDetails_.extent, *device_));
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
    frameBuffers_ = forceUnwrap(Framebuffer::create(imageViews_, *renderPass_, swapChainSupportDetails_.extent, *device_));
    spdlog::info("recreated swap chain");
}

void VKEngine::createGraphicsPipeline()
{
    spdlog::info("creating graphics pipeline");
    auto vertShaderModule = forceUnwrap(ShaderModule::create("data/shaders/vert.spv", *device_));
    auto fragShaderModule = forceUnwrap(ShaderModule::create("data/shaders/frag.spv", *device_));

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(*device_, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
    pipelineLayout_.emplace(pipelineLayout, *device_);

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(*device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    graphicsPipeline_.emplace(pipeline, *device_);
}

void VKEngine::createRenderPass() 
{
    spdlog::info("create render pass");
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = static_cast<VkFormat>(swapChainSupportDetails_.surfaceFormat.format); // swapChain_->imageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkRenderPass renderPass;
    if (vkCreateRenderPass(*device_, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
    renderPass_.emplace(renderPass, *device_);
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