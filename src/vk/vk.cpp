#include "md2view/vk/vk.hpp"

#include "md2view/window.hpp"

#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <glm/glm.hpp>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <limits>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace VK {

static constexpr std::array<char const*, 1> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

static constexpr std::array<char const*, 1> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT /* messageSeverity */,
              vk::DebugUtilsMessageTypeFlagsEXT /* messageType */,
              vk::DebugUtilsMessengerCallbackDataEXT const* pCallbackData,
              void* /* pUserData */) {

    spdlog::info("validation layer: {}", pCallbackData->pMessage);

    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(
    vk::DebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                             vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                             vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    createInfo.pfnUserCallback = debugCallback;
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
    std::vector<vk::SurfaceFormatKHR> const& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR chooseSwapPresentMode(
    std::vector<vk::PresentModeKHR> const& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return vk::PresentModeKHR::eMailbox;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities,
                              Window const& window) {
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    int width{};
    int height{};
    glfwGetFramebufferSize(window.get(), &width, &height);

    vk::Extent2D actualExtent = {static_cast<uint32_t>(width),
                                 static_cast<uint32_t>(height)};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);
    return actualExtent;
}

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device,
                                     vk::SurfaceKHR const& surface) {
    QueueFamilyIndices indices;
    auto const queueFamilies = device.getQueueFamilyProperties();

    for (auto i{0}; i < gsl_lite::narrow<int>(queueFamilies.size()); ++i) {
        auto const& queueFamily = queueFamilies.at(i);
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily = i;
        }
        if (device.getSurfaceSupportKHR(i, surface) != 0U) {
            indices.presentFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
    }
    spdlog::info("queue family indices: {} {}", indices.graphicsFamily.value(),
                 indices.presentFamily.value());
    return indices;
}

bool checkDeviceExtensionSupport(vk::PhysicalDevice device) {
    auto const availableExtensions =
        device.enumerateDeviceExtensionProperties();
    std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                             deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

vk::raii::Instance createInstance(vk::raii::Context& context) {
    spdlog::info("create instance");
    auto const supportedExtensions =
        context.enumerateInstanceExtensionProperties();
    spdlog::info("{} extensions supported", supportedExtensions.size());
    for (auto const& ext : supportedExtensions) {
        spdlog::debug("  {}", std::string_view{ext.extensionName});
    }

    vk::ApplicationInfo appInfo("vkmd2v", 1, "No Engine", 1,
                                VK_API_VERSION_1_1);

    auto availableLayers = context.enumerateInstanceLayerProperties();

    for (auto const* layerName : validationLayers) {
        std::string_view sv{layerName};
        auto iter =
            std::ranges::find_if(availableLayers, [sv](auto const& layer) {
                return sv == std::string_view{layer.layerName};
            });
        if (iter == std::ranges::end(availableLayers)) {
            throw std::runtime_error(
                fmt::format("validation layer not available {}", sv));
        }
        spdlog::info("found validation layer {}", sv);
    }

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    populateDebugMessengerCreateInfo(debugCreateInfo);

    vk::InstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    char const** glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    spdlog::info("glfwExtentionCount: {}", glfwExtensionCount);
    std::span extSpan{glfwExtensions, glfwExtensionCount};
    std::vector<char const*> extensions(extSpan.begin(), extSpan.end());

    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    for (auto const* ext : extensions) {
        spdlog::info("requesting ext '{}'", ext);
    }

    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    createInfo.pNext = std::addressof(debugCreateInfo);

    return vk::raii::Instance{context, createInfo};
}

vk::raii::DebugUtilsMessengerEXT
createDebugUtilsMessenger(vk::raii::Instance& instance) {
    vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);
    return vk::raii::DebugUtilsMessengerEXT{instance, createInfo};
}

vk::raii::Device createDevice(vk::raii::PhysicalDevice const& physicalDevice,
                              QueueFamilyIndices const& queueFamilyIndices) {
    spdlog::info("create logical device");
    static constexpr float queuePriority = 1.0f;

    // vk::PhysicalDeviceFeatures deviceFeatures{};

    std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilyIndices.graphicsFamily.value(),
        queueFamilyIndices.presentFamily.value()};

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueQueueFamilies.size());
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo{
            {}, queueFamily, 1, &queuePriority};
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // NB: device layers are deprecated and ignored; the instance layers
    // already cover the device
    vk::DeviceCreateInfo createInfo{{}, queueCreateInfos, {}, deviceExtensions};
    return vk::raii::Device{physicalDevice, createInfo};
}

SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice physicalDevice,
                                              vk::SurfaceKHR const& surface) {
    SwapChainSupportDetails details;
    details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    details.formats = physicalDevice.getSurfaceFormatsKHR(surface);
    details.presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
    return details;
}

std::pair<vk::raii::PhysicalDevice, QueueFamilyIndices>
pickPhysicalDevice(vk::raii::Instance& instance,
                   vk::SurfaceKHR const& surface) {
    spdlog::info("pick physical device");
    auto devices = instance.enumeratePhysicalDevices();

    for (const auto& device : devices) {
        auto const deviceProperties = device.getProperties();
        auto const isDiscrete =
            deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
        if (!isDiscrete) {
            continue;
        }
        spdlog::info("found GPU discrete {}",
                     std::string_view{deviceProperties.deviceName});
        auto const queueFamilyIndices = findQueueFamilies(*device, surface);
        if (!queueFamilyIndices.isComplete()) {
            continue;
        }
        bool const extensionsSupported = checkDeviceExtensionSupport(*device);
        if (!extensionsSupported) {
            continue;
        }
        auto swapChainSupport = querySwapChainSupport(*device, surface);
        bool const swapChainAdequate = !swapChainSupport.formats.empty() &&
                                       !swapChainSupport.presentModes.empty();
        if (swapChainAdequate) {
            return std::make_pair(device, queueFamilyIndices);
        }
    }
    throw std::runtime_error("no suitable device found");
}

vk::raii::SurfaceKHR createSurface(vk::raii::Instance& instance,
                                   Window const& window) {
    spdlog::info("create surface");
    VkSurfaceKHR surface;
    auto const result =
        glfwCreateWindowSurface(*instance, window.get(), nullptr, &surface);
    if (result != VK_SUCCESS) {
        throw std::runtime_error(fmt::format(
            "failed to create window surface! {}", static_cast<int>(result)));
    }
    return vk::raii::SurfaceKHR{instance, surface};
}

std::pair<vk::raii::SwapchainKHR, SwapChainSupportDetails>
createSwapChain(vk::raii::PhysicalDevice const& physicalDevice,
                vk::raii::Device const& device,
                Window const& window,
                vk::SurfaceKHR const& surface,
                QueueFamilyIndices const& queueFamilyIndices) {
    spdlog::debug("create swap chain");
    auto swapChainSupport = querySwapChainSupport(*physicalDevice, surface);
    swapChainSupport.surfaceFormat =
        chooseSwapSurfaceFormat(swapChainSupport.formats);
    auto const presentMode =
        chooseSwapPresentMode(swapChainSupport.presentModes);
    swapChainSupport.extent =
        chooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = swapChainSupport.surfaceFormat.format;
    createInfo.imageColorSpace = swapChainSupport.surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChainSupport.extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::
        eColorAttachment; // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    std::array<uint32_t, 2> indices = {
        queueFamilyIndices.graphicsFamily.value(),
        queueFamilyIndices.presentFamily.value()};

    if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = indices.size();
        createInfo.pQueueFamilyIndices = indices.data();
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::
        eOpaque; // VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    return std::make_pair(vk::raii::SwapchainKHR{device, createInfo},
                          swapChainSupport);
}

std::vector<vk::raii::ImageView>
createImageViews(vk::raii::Device const& device,
                 std::vector<vk::Image>& images,
                 SwapChainSupportDetails const& swapChainSupport) {
    spdlog::debug("create image views");
    std::vector<vk::raii::ImageView> views;
    views.reserve(images.size());

    for (auto const& image : images) {
        vk::ImageViewCreateInfo createInfo{};
        createInfo.image = image;
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = swapChainSupport.surfaceFormat.format;
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;
        createInfo.subresourceRange.aspectMask =
            vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        views.emplace_back(device, createInfo);
    }

    return views;
}

vk::raii::ShaderModule createShaderModule(std::filesystem::path const& path,
                                          vk::raii::Device const& device) {
    spdlog::info("creating shader module for {}", path.string());
    std::vector<char> buffer;
    {
        std::ifstream inf(path, std::ios::ate | std::ios::binary);

        if (!inf.is_open()) {
            throw std::runtime_error(
                fmt::format("failed to open file '{}'!", path.string()));
        }

        auto const fileSize = static_cast<size_t>(inf.tellg());
        inf.seekg(0);
        buffer.resize(fileSize);
        inf.read(buffer.data(), gsl_lite::narrow<std::streamsize>(fileSize));
    }

    vk::ShaderModuleCreateInfo createInfo{
        {}, buffer.size(), reinterpret_cast<uint32_t const*>(buffer.data())};

    return device.createShaderModule(createInfo);
}

std::vector<vk::raii::Framebuffer>
createFrameBuffers(std::vector<vk::raii::ImageView> const& imageViews,
                   vk::raii::RenderPass const& renderPass,
                   vk::Extent2D swapChainExtent,
                   vk::raii::Device const& device) {
    spdlog::debug("create frame buffer");
    std::vector<vk::raii::Framebuffer> frameBuffers;
    frameBuffers.reserve(imageViews.size());

    for (auto const& imageView : imageViews) {
        std::array<vk::ImageView, 1UL> attachments{*imageView};
        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.renderPass = *renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        frameBuffers.emplace_back(device.createFramebuffer(framebufferInfo));
    }

    return frameBuffers;
}

vk::raii::CommandPool createCommandPool(vk::raii::Device const& device,
                                        QueueFamilyIndices const& indices) {
    spdlog::info("creating command pool");
    return device.createCommandPool(vk::CommandPoolCreateInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        indices.graphicsFamily.value()));
}

std::vector<vk::raii::Fence> createFences(vk::raii::Device const& device,
                                          unsigned int numFences) {
    std::vector<vk::raii::Fence> fences;
    fences.reserve(numFences);

    for (auto i{0U}; i < numFences; ++i) {
        fences.emplace_back(device.createFence(
            vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled}));
    }
    return fences;
}

std::vector<vk::raii::Semaphore>
createSemaphores(vk::raii::Device const& device, unsigned int numSemaphores) {
    std::vector<vk::raii::Semaphore> semaphores;
    semaphores.reserve(numSemaphores);

    for (auto i{0U}; i < numSemaphores; ++i) {
        semaphores.emplace_back(
            device.createSemaphore(vk::SemaphoreCreateInfo{}));
    }
    return semaphores;
}

uint32_t findMemoryType(uint32_t typeFilter,
                        vk::MemoryPropertyFlags properties,
                        vk::raii::PhysicalDevice const& physicalDevice) {
    auto const memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        auto const hasType = (typeFilter & (1 << i)) != 0U;
        auto const propFlags =
            gsl_lite::at(memProperties.memoryTypes, i).propertyFlags;
        auto const hasMemProps = (propFlags & properties) == properties;

        if (hasType && hasMemProps) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

BoundBuffer BoundBuffer::create(vk::raii::Device const& device,
                                vk::raii::PhysicalDevice const& physicalDevice,
                                vk::DeviceSize size,
                                vk::BufferUsageFlags usage,
                                vk::MemoryPropertyFlags properties) {
    // first create the buffer
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    auto buf = device.createBuffer(bufferInfo);

    // next allocate the memory
    auto const memRequirements = buf.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               properties, physicalDevice);
    auto mem = device.allocateMemory(allocInfo);

    // bind buffer to memory and return
    buf.bindMemory(*mem, 0UL);
    return BoundBuffer{
        .buffer = std::move(buf), .memory = std::move(mem), .size = size};
}

BoundBuffer
createDynamicVertexBuffer(vk::raii::Device const& device,
                          vk::raii::PhysicalDevice const& physicalDevice,
                          vk::DeviceSize size) {
    return BoundBuffer::create(device, physicalDevice, size,
                               vk::BufferUsageFlagBits::eVertexBuffer,
                               vk::MemoryPropertyFlagBits::eHostVisible |
                                   vk::MemoryPropertyFlagBits::eHostCoherent);
}

BoundBuffer
createStaticVertexBuffer(vk::raii::Device const& device,
                         vk::raii::PhysicalDevice const& physicalDevice,
                         vk::DeviceSize size) {
    return BoundBuffer::create(device, physicalDevice, size,
                               vk::BufferUsageFlagBits::eTransferDst |
                                   vk::BufferUsageFlagBits::eVertexBuffer,
                               vk::MemoryPropertyFlagBits::eDeviceLocal);
}

BoundBuffer createIndexBuffer(vk::raii::Device const& device,
                              vk::raii::PhysicalDevice const& physicalDevice,
                              vk::DeviceSize size) {
    return BoundBuffer::create(device, physicalDevice, size,
                               vk::BufferUsageFlagBits::eTransferDst |
                                   vk::BufferUsageFlagBits::eIndexBuffer,
                               vk::MemoryPropertyFlagBits::eDeviceLocal);
}

BoundBuffer createStagingBuffer(vk::raii::Device const& device,
                                vk::raii::PhysicalDevice const& physicalDevice,
                                vk::DeviceSize size) {
    return BoundBuffer::create(device, physicalDevice, size,
                               vk::BufferUsageFlagBits::eTransferSrc,
                               vk::MemoryPropertyFlagBits::eHostVisible |
                                   vk::MemoryPropertyFlagBits::eHostCoherent);
}

BoundBuffer createUniformBuffer(vk::raii::Device const& device,
                                vk::raii::PhysicalDevice const& physicalDevice,
                                vk::DeviceSize size) {
    auto buf = BoundBuffer::create(
        device, physicalDevice, size, vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
    buf.map();
    return buf;
}

void copyBuffer(BoundBuffer const& src,
                BoundBuffer const& dst,
                vk::raii::Device const& device,
                vk::raii::CommandPool const& commandPool,
                vk::raii::Queue const& graphicsQueue) {

    gsl_Assert(src.size == dst.size);
    vk::CommandBufferAllocateInfo allocInfo{
        *commandPool, vk::CommandBufferLevel::ePrimary, 1};

    auto commandBuffers = device.allocateCommandBuffers(allocInfo);
    auto& commandBuffer = commandBuffers.at(0);

    vk::CommandBufferBeginInfo beginInfo{
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

    vk::BufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = src.size;

    commandBuffer.begin(beginInfo);
    commandBuffer.copyBuffer(*src.buffer, *dst.buffer, copyRegion);
    commandBuffer.end();

    vk::SubmitInfo submitInfo{nullptr, nullptr, *commandBuffer};

    graphicsQueue.submit(submitInfo, nullptr);
    graphicsQueue.waitIdle();
}

vk::raii::DescriptorSetLayout
createDescriptorSetLayout(vk::raii::Device const& device) {
    vk::DescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

    return device.createDescriptorSetLayout(
        vk::DescriptorSetLayoutCreateInfo({}, uboLayoutBinding));
}

vk::raii::DescriptorPool createDescriptorPool(vk::raii::Device const& device,
                                              unsigned int maxFramesInFlight) {
    vk::DescriptorPoolSize poolSize{};
    poolSize.type = vk::DescriptorType::eUniformBuffer;
    poolSize.descriptorCount = static_cast<uint32_t>(maxFramesInFlight);

    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(maxFramesInFlight);
    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

    return device.createDescriptorPool(poolInfo);
}

std::vector<vk::raii::DescriptorSet>
createDescriptorSets(vk::raii::Device const& device,
                     vk::raii::DescriptorPool const& pool,
                     vk::raii::DescriptorSetLayout const& layout,
                     unsigned int maxFramesInFlight) {

    std::vector<vk::DescriptorSetLayout> layouts(maxFramesInFlight, *layout);
    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.descriptorPool = *pool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(maxFramesInFlight);
    allocInfo.pSetLayouts = layouts.data();

    return device.allocateDescriptorSets(allocInfo);
}

} // namespace VK
