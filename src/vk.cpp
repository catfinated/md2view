#include "md2view/vk/vk.hpp"

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
#include <string_view>
#include <vector>

namespace md2v {

Window::Window(GLFWwindow* window) noexcept
    : window_(window) {}

Window::~Window() noexcept {
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
    }
}

Window& Window::operator=(Window&& rhs) noexcept {
    if (this != std::addressof(rhs)) {
        if (window_ != nullptr) {
            glfwDestroyWindow(window_);
        }
        window_ = std::exchange(rhs.window_, nullptr);
    }
    return *this;
}

std::expected<Window, std::runtime_error> Window::create(int width,
                                                         int height) noexcept {
    spdlog::info("create window");
    gsl_Expects(width > 0);
    gsl_Expects(height > 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto* window = glfwCreateWindow(width, height, "vkmd2v", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    spdlog::info("{} extensions supported", extensionCount);
    return Window{window};
}

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
                                     vk::SurfaceKHR const& surface) noexcept {
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

bool checkDeviceExtensionSupport(vk::PhysicalDevice device) noexcept {
    auto const availableExtensions =
        device.enumerateDeviceExtensionProperties();
    std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                             deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

std::expected<vk::raii::Instance, std::runtime_error>
createInstance(vk::raii::Context& context) noexcept {
    spdlog::info("create instance");
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
            std::unexpected(std::runtime_error(
                fmt::format("validation layer not available {}", sv)));
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

    try {
        return vk::raii::Instance(context, createInfo);
    } catch (std::exception const& excp) {
        return std::unexpected(std::runtime_error(excp.what()));
    }
}

std::expected<vk::raii::DebugUtilsMessengerEXT, std::runtime_error>
createDebugUtilsMessenger(vk::raii::Instance& instance) noexcept {
    vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);
    try {
        return vk::raii::DebugUtilsMessengerEXT{instance, createInfo};
    } catch (std::exception const& excp) {
        return std::unexpected(std::runtime_error(excp.what()));
    }
}

std::expected<vk::raii::Device, std::runtime_error>
createDevice(vk::raii::PhysicalDevice const& physicalDevice,
             QueueFamilyIndices const& queueFamilyIndices) noexcept {
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

    vk::DeviceCreateInfo createInfo{
        {}, queueCreateInfos, validationLayers, deviceExtensions};
    return vk::raii::Device{physicalDevice, createInfo};
}

SwapChainSupportDetails
querySwapChainSupport(vk::PhysicalDevice physicalDevice,
                      vk::SurfaceKHR const& surface) noexcept {
    SwapChainSupportDetails details;
    details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    details.formats = physicalDevice.getSurfaceFormatsKHR(surface);
    details.presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
    return details;
}

std::expected<std::pair<vk::raii::PhysicalDevice, QueueFamilyIndices>,
              std::runtime_error>
pickPhysicalDevice(vk::raii::Instance& instance,
                   vk::SurfaceKHR const& surface) noexcept {
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
    return std::unexpected(std::runtime_error("no suitable device found"));
}

std::expected<vk::raii::SurfaceKHR, std::runtime_error>
createSurface(vk::raii::Instance& instance, Window const& window) noexcept {
    spdlog::info("create surface");
    VkSurfaceKHR surface;
    auto const result =
        glfwCreateWindowSurface(*instance, window.get(), nullptr, &surface);
    if (result != VK_SUCCESS) {
        return std::unexpected(std::runtime_error(fmt::format(
            "failed to create window surface! {}", static_cast<int>(result))));
    }
    try {
        return vk::raii::SurfaceKHR{instance, surface};
    } catch (std::exception const& excp) {
        return std::unexpected(std::runtime_error(excp.what()));
    }
}

std::expected<std::pair<vk::raii::SwapchainKHR, SwapChainSupportDetails>,
              std::runtime_error>
createSwapChain(vk::raii::PhysicalDevice const& physicalDevice,
                vk::raii::Device const& device,
                Window const& window,
                vk::SurfaceKHR const& surface,
                QueueFamilyIndices const& queueFamilyIndices) noexcept {
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

    try {
        return std::make_pair(vk::raii::SwapchainKHR(device, createInfo),
                              swapChainSupport);
    } catch (std::exception const& excp) {
        spdlog::error(excp.what());
        return std::unexpected(std::runtime_error(excp.what()));
    }
}

std::expected<std::vector<vk::raii::ImageView>, std::runtime_error>
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

        try {
            views.emplace_back(device, createInfo);
        } catch (std::runtime_error const& err) {
            return std::unexpected(err);
        }
    }

    return views;
}

std::expected<vk::raii::ShaderModule, std::runtime_error>
createShaderModule(std::filesystem::path const& path,
                   vk::raii::Device const& device) noexcept {
    spdlog::info("creating shader module for {}", path.string());
    std::vector<char> buffer;
    {
        std::ifstream inf(path, std::ios::ate | std::ios::binary);

        if (!inf.is_open()) {
            return std::unexpected(std::runtime_error(
                fmt::format("failed to open file '{}'!", path.string())));
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

std::expected<std::vector<vk::raii::Framebuffer>, std::runtime_error>
createFrameBuffers(std::vector<vk::raii::ImageView> const& imageViews,
                   vk::raii::RenderPass const& renderPass,
                   vk::Extent2D swapChainExtent,
                   vk::raii::Device const& device) noexcept {
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

std::expected<vk::raii::CommandPool, std::runtime_error>
createCommandPool(vk::raii::Device const& device,
                  QueueFamilyIndices const& indices) noexcept {
    spdlog::info("creating command pool");
    try {
        return device.createCommandPool(vk::CommandPoolCreateInfo(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            indices.graphicsFamily.value()));
    } catch (std::runtime_error const& excp) {
        return std::unexpected(excp);
    }
}

std::expected<std::vector<vk::raii::Fence>, std::runtime_error>
createFences(vk::raii::Device const& device, unsigned int numFences) noexcept {
    std::vector<vk::raii::Fence> fences;
    fences.reserve(numFences);

    for (auto i{0U}; i < numFences; ++i) {
        try {
            fences.emplace_back(device.createFence(
                vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled}));
        } catch (std::runtime_error const& excp) {
            return std::unexpected(excp);
        }
    }
    return fences;
}

std::expected<std::vector<vk::raii::Semaphore>, std::runtime_error>
createSemaphores(vk::raii::Device const& device,
                 unsigned int numSemaphores) noexcept {
    std::vector<vk::raii::Semaphore> semaphores;
    semaphores.reserve(numSemaphores);

    for (auto i{0U}; i < numSemaphores; ++i) {
        try {
            semaphores.emplace_back(
                device.createSemaphore(vk::SemaphoreCreateInfo{}));
        } catch (std::runtime_error const& excp) {
            return std::unexpected(excp);
        }
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

std::expected<BoundBuffer, std::runtime_error>
BoundBuffer::create(vk::raii::Device const& device,
                    vk::raii::PhysicalDevice const& physicalDevice,
                    vk::DeviceSize size,
                    vk::BufferUsageFlags usage,
                    vk::MemoryPropertyFlags properties) noexcept {
    try {
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
        allocInfo.memoryTypeIndex = findMemoryType(
            memRequirements.memoryTypeBits, properties, physicalDevice);
        auto mem = device.allocateMemory(allocInfo);

        // bind buffer to memory and return
        buf.bindMemory(*mem, 0UL);
        return BoundBuffer{
            .buffer = std::move(buf), .memory = std::move(mem), .size = size};
    } catch (std::runtime_error const& excp) {
        return std::unexpected(excp);
    }
}

std::expected<BoundBuffer, std::runtime_error>
createDynamicVertexBuffer(vk::raii::Device const& device,
                          vk::raii::PhysicalDevice const& physicalDevice,
                          vk::DeviceSize size) noexcept {
    return BoundBuffer::create(device, physicalDevice, size,
                               vk::BufferUsageFlagBits::eVertexBuffer,
                               vk::MemoryPropertyFlagBits::eHostVisible |
                                   vk::MemoryPropertyFlagBits::eHostCoherent);
}

std::expected<BoundBuffer, std::runtime_error>
createStaticVertexBuffer(vk::raii::Device const& device,
                         vk::raii::PhysicalDevice const& physicalDevice,
                         vk::DeviceSize size) noexcept {
    return BoundBuffer::create(device, physicalDevice, size,
                               vk::BufferUsageFlagBits::eTransferDst |
                                   vk::BufferUsageFlagBits::eVertexBuffer,
                               vk::MemoryPropertyFlagBits::eDeviceLocal);
}

std::expected<BoundBuffer, std::runtime_error>
createStagingBuffer(vk::raii::Device const& device,
                    vk::raii::PhysicalDevice const& physicalDevice,
                    vk::DeviceSize size) noexcept {
    return BoundBuffer::create(device, physicalDevice, size,
                               vk::BufferUsageFlagBits::eTransferSrc,
                               vk::MemoryPropertyFlagBits::eHostVisible |
                                   vk::MemoryPropertyFlagBits::eHostCoherent);
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

} // namespace md2v
