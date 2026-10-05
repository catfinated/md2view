#include "md2view/vk/engine.hpp"
#include "md2view/vk/ubo.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <array>
#include <cstdint>
#include <ranges>
#include <stdexcept>

namespace VK {

static std::vector<Vertex> const vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};

static std::vector<uint16_t> const indices = {0, 1, 2, 2, 3, 0};

void VKEngine::doInit() {
    spdlog::info("GLFW version: {}", glfwGetVersionString());
    spdlog::info("Vulkan supported: {}",
                 glfwVulkanSupported() != 0 ? "yes" : "no");

    initWindow();
    initVulkan();
}

void VKEngine::initWindow() {
    constexpr std::array hints{WindowHint{GLFW_CLIENT_API, GLFW_NO_API}};
    window_ = Window::create(width_, height_, "vkmd2v", hints);
    window_.setFramebufferResizeHandler(
        [this](int /*width*/, int /*height*/) { frameBufferResized_ = true; });
}

void VKEngine::initVulkan() {
    instance_ = createInstance(context_);
    debugMessenger_ = createDebugUtilsMessenger(instance_);
    surface_ = createSurface(instance_, window_);
    std::tie(physicalDevice_, queueFamilyIndices_) =
        pickPhysicalDevice(instance_, *surface_);
    device_ = createDevice(physicalDevice_, queueFamilyIndices_);

    graphicsQueue_ =
        device_.getQueue(queueFamilyIndices_.graphicsFamily.value(), 0);
    presentQueue_ =
        device_.getQueue(queueFamilyIndices_.presentFamily.value(), 0);

    std::tie(swapChain_, swapChainSupportDetails_) = createSwapChain(
        physicalDevice_, device_, window_, *surface_, queueFamilyIndices_);
    swapChainImages_ = swapChain_.getImages();
    imageViews_ =
        createImageViews(device_, swapChainImages_, swapChainSupportDetails_);

    createRenderPass();
    createGraphicsPipeline();
    frameBuffers_ = createFrameBuffers(
        imageViews_, renderPass_, swapChainSupportDetails_.extent, device_);
    commandPool_ = createCommandPool(device_, queueFamilyIndices_);

    vk::CommandBufferAllocateInfo allocInfo{
        *commandPool_, vk::CommandBufferLevel::ePrimary, kMaxFramesInFlight};
    commandBuffers_ = device_.allocateCommandBuffers(allocInfo);
    imageAvailableSemaphores_ = createSemaphores(device_, kMaxFramesInFlight);
    // NB: one per swap chain image rather than per frame in flight. A render
    // finished semaphore stays in use by its presentation until that image is
    // acquired again, so it cannot be cycled with the frame index.
    renderFinishedSemaphores_ = createSemaphores(
        device_, static_cast<unsigned int>(swapChainImages_.size()));
    inflightFences_ = createFences(device_, kMaxFramesInFlight);

    // copy vertices to high performance on device gpu memory
    auto bufSize = sizeof(Vertex) * vertices.size();
    auto stagingBuffer = createStagingBuffer(device_, physicalDevice_, bufSize);
    stagingBuffer.memcpy(vertices);
    vertexBuffer_ = createStaticVertexBuffer(device_, physicalDevice_, bufSize);
    copyBuffer(stagingBuffer, vertexBuffer_, device_, commandPool_,
               graphicsQueue_);

    // copy indices to high performance on device gpu memory
    bufSize = sizeof(indices[0]) * indices.size();
    stagingBuffer = createStagingBuffer(device_, physicalDevice_, bufSize);
    stagingBuffer.memcpy(indices);
    indexBuffer_ = createIndexBuffer(device_, physicalDevice_, bufSize);
    copyBuffer(stagingBuffer, indexBuffer_, device_, commandPool_,
               graphicsQueue_);

    uniformBuffers_.reserve(kMaxFramesInFlight);
    for (auto i = 0U; i < kMaxFramesInFlight; ++i) {
        uniformBuffers_.push_back(createUniformBuffer(
            device_, physicalDevice_, sizeof(UniformBufferObject)));
    }

    descriptorPool_ = createDescriptorPool(device_, kMaxFramesInFlight);
    descriptorSets_ = createDescriptorSets(
        device_, descriptorPool_, descriptorSetLayout_, kMaxFramesInFlight);
    gsl_Assert(descriptorSets_.size() == uniformBuffers_.size());

    for (auto [boundBuffer, descriptorSet] :
         std::views::zip(uniformBuffers_, descriptorSets_)) {
        vk::DescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = *boundBuffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        vk::WriteDescriptorSet descriptorWrite{};
        descriptorWrite.dstSet = *descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        device_.updateDescriptorSets(descriptorWrite, nullptr);
    }

    spdlog::info("vulkan initialization complete. num views={}",
                 imageViews_.size());
}

void VKEngine::recreateSwapChain() {
    int width{};
    int height{};
    glfwGetFramebufferSize(window_.get(), &width, &height);
    while (width == 0 || height == 0) {
        if (window_.shouldClose()) {
            return;
        }
        glfwGetFramebufferSize(window_.get(), &width, &height);
        glfwWaitEvents();
    }

    device_.waitIdle();
    frameBuffers_.clear();
    imageViews_.clear();
    swapChain_ = nullptr; // NB: must destroy previous swap chain first!

    std::tie(swapChain_, swapChainSupportDetails_) = createSwapChain(
        physicalDevice_, device_, window_, *surface_, queueFamilyIndices_);
    swapChainImages_ = swapChain_.getImages();
    imageViews_ =
        createImageViews(device_, swapChainImages_, swapChainSupportDetails_);
    frameBuffers_ = createFrameBuffers(
        imageViews_, renderPass_, swapChainSupportDetails_.extent, device_);
    // the image count can change, and each render finished semaphore belongs
    // to an image, so rebuild them with the swap chain
    renderFinishedSemaphores_ = createSemaphores(
        device_, static_cast<unsigned int>(swapChainImages_.size()));
    spdlog::debug("recreated swap chain");
}

void VKEngine::createGraphicsPipeline() {
    spdlog::info("creating graphics pipeline");
    auto vertShaderModule =
        createShaderModule("data/shaders/vert.spv", device_);
    auto fragShaderModule =
        createShaderModule("data/shaders/frag.spv", device_);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.stage =
        vk::ShaderStageFlagBits::eVertex; // VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = *vertShaderModule;
    vertShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.stage =
        vk::ShaderStageFlagBits::eFragment; //  VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = *fragShaderModule;
    fragShaderStageInfo.pName = "main";

    std::array<vk::PipelineShaderStageCreateInfo, 2UL> shaderStages = {
        vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.topology = vk::PrimitiveTopology::
        eTriangleList; // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = vk::False; // VK_FALSE;

    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.depthClampEnable = vk::False;         // VK_FALSE;
    rasterizer.rasterizerDiscardEnable = vk::False;  // VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill; // VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack; // VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = vk::False; // VK_FALSE;

    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sampleShadingEnable = vk::False; // VK_FALSE;
    multisampling.rasterizationSamples =
        vk::SampleCountFlagBits::e1; // VK_SAMPLE_COUNT_1_BIT;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    // VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    // VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = vk::False; // VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.logicOpEnable = vk::False;    // VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy; // VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport, // VK_DYNAMIC_STATE_VIEWPORT,
        vk::DynamicState::eScissor   // VK_DYNAMIC_STATE_SCISSOR
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{
        {}, static_cast<uint32_t>(dynamicStates.size()), dynamicStates.data()};

    descriptorSetLayout_ = createDescriptorSetLayout(device_);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &*descriptorSetLayout_;
    pipelineLayout_ = device_.createPipelineLayout(pipelineLayoutInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages.data();
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
    pipelineInfo.basePipelineIndex = -1;              // Optional

    graphicsPipeline_ = device_.createGraphicsPipeline(nullptr, pipelineInfo);
}

void VKEngine::createRenderPass() {
    spdlog::info("create render pass");
    vk::AttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainSupportDetails_.surfaceFormat.format;
    colorAttachment.samples =
        vk::SampleCountFlagBits::e1; // VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp =
        vk::AttachmentLoadOp::eClear; // VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp =
        vk::AttachmentStoreOp::eStore; // VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp =
        vk::AttachmentLoadOp::eDontCare; // VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp =
        vk::AttachmentStoreOp::eDontCare; // VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout =
        vk::ImageLayout::eUndefined; // VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout =
        vk::ImageLayout::ePresentSrcKHR; // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    vk::AttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::
        eColorAttachmentOptimal; // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint =
        vk::PipelineBindPoint::eGraphics; // VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::RenderPassCreateInfo renderPassInfo{};
    // renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
    // VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    vk::SubpassDependency dependency{};
    dependency.srcSubpass = vk::SubpassExternal; // VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::
        eColorAttachmentOutput; // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = vk::AccessFlagBits::eNone;
    dependency.dstStageMask = vk::PipelineStageFlagBits::
        eColorAttachmentOutput; //  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = vk::AccessFlagBits::
        eColorAttachmentWrite; // VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    renderPass_ = device_.createRenderPass(renderPassInfo);
}

void VKEngine::recordCommandBuffer(vk::raii::CommandBuffer& commandBuffer,
                                   uint32_t imageIndex) {
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.pInheritanceInfo = nullptr; // Optional

    commandBuffer.begin(beginInfo);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    clearValue_.color = vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f};
    vk::RenderPassBeginInfo renderPassInfo{
        *renderPass_, *(frameBuffers_.at(imageIndex)),
        vk::Rect2D{vk::Offset2D{0, 0}, swapChainSupportDetails_.extent},
        clearValue_};

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                               *graphicsPipeline_);

    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainSupportDetails_.extent.width);
    viewport.height =
        static_cast<float>(swapChainSupportDetails_.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    commandBuffer.setViewport(0, {viewport});

    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D{0, 0};
    scissor.extent = swapChainSupportDetails_.extent;
    commandBuffer.setScissor(0, {scissor});

    std::array<vk::Buffer, 1UL> vertexBuffers{*vertexBuffer_.buffer};
    std::array<vk::DeviceSize, 1UL> offsets{0};
    commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
    commandBuffer.bindIndexBuffer(*indexBuffer_.buffer, 0,
                                  vk::IndexType::eUint16);
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, pipelineLayout_, 0,
        {*descriptorSets_.at(currentFrame_)}, nullptr);
    commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0,
                              0);

    // commandBuffer.draw(vertices.size(), 1, 0, 0);

    commandBuffer.endRenderPass();
    commandBuffer.end();
}

void VKEngine::drawFrame(float time) {
    auto& fence = inflightFences_.at(currentFrame_);
    gsl_Assert(device_.waitForFences({*fence}, true, UINT64_MAX) ==
               vk::Result::eSuccess);

    auto& imageAvailableSemaphore = imageAvailableSemaphores_.at(currentFrame_);
    uint32_t imageIndex;
    // vk hpp throws exception on OutOfDateKHR result
    // https://github.com/KhronosGroup/Vulkan-Hpp/issues/599
    auto result = static_cast<vk::Result>(
        swapChain_.getDispatcher()->vkAcquireNextImageKHR(
            static_cast<VkDevice>(swapChain_.getDevice()),
            static_cast<VkSwapchainKHR>(*swapChain_), UINT64_MAX,
            static_cast<VkSemaphore>(*imageAvailableSemaphore), VK_NULL_HANDLE,
            &imageIndex));

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return;
    }

    if (result != vk::Result::eSuccess &&
        result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image");
    }

    device_.resetFences({*fence});

    auto& commandBuffer = commandBuffers_.at(currentFrame_);
    commandBuffer.reset();
    recordCommandBuffer(commandBuffer, imageIndex);

    std::array<vk::Semaphore, 1UL> waitSemaphores{*imageAvailableSemaphore};
    std::array<vk::CommandBuffer, 1UL> commandBuffers{*commandBuffer};
    std::array<vk::PipelineStageFlags, 1UL> waitStages{
        vk::PipelineStageFlagBits::eColorAttachmentOutput};

    updateUniformBuffer(currentFrame_, time);

    vk::SubmitInfo submitInfo{};
    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.commandBufferCount = commandBuffers.size();
    submitInfo.pCommandBuffers = commandBuffers.data();

    auto& renderFinishedSemaphore = renderFinishedSemaphores_.at(imageIndex);
    std::array<vk::Semaphore, 1UL> signalSemaphores{*renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = signalSemaphores.size();
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    graphicsQueue_.submit({submitInfo}, *fence);

    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = signalSemaphores.size();
    presentInfo.pWaitSemaphores = signalSemaphores.data();

    std::array<vk::SwapchainKHR, 1UL> swapChains{*swapChain_};
    presentInfo.swapchainCount = swapChains.size();
    presentInfo.pSwapchains = swapChains.data();
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    // vk hpp throws exception on OutOfDateKHR result
    // https://github.com/KhronosGroup/Vulkan-Hpp/issues/599
    result = static_cast<vk::Result>(
        presentQueue_.getDispatcher()->vkQueuePresentKHR(
            static_cast<VkQueue>(*presentQueue_),
            reinterpret_cast<const VkPresentInfoKHR*>(&presentInfo)));

    currentFrame_ = (currentFrame_ + 1U) % kMaxFramesInFlight;

    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR || frameBufferResized_) {
        frameBufferResized_ = false;
        recreateSwapChain();
    } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swap chain image!");
    }
}

void VKEngine::updateUniformBuffer(uint32_t currentImage, float time) {
    UniformBufferObject ubo{};
    auto const width = swapChainSupportDetails_.extent.width;
    auto const height =
        static_cast<float>(swapChainSupportDetails_.extent.height);
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                            glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view =
        glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj =
        glm::perspective(glm::radians(45.0f), width / height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
    uniformBuffers_.at(currentImage).memcpy(ubo);
}

void VKEngine::run_game() {
    while (!window_.shouldClose()) {
        beginFrame();
        glfwPollEvents();
        drawFrame(gsl_lite::narrow_cast<float>(glfwGetTime()));
    }
    device_.waitIdle();
}

} // namespace VK
