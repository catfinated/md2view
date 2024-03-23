#include "vk.hpp"

#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/transform.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/std.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <iterator>
#include <string_view>
#include <vector>

namespace myvk {

static constexpr std::array<char const*, 1> validationLayers = {
    "VK_LAYER_KHRONOS_validation"    
};

static constexpr std::array<char const*, 1> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    spdlog::info("validation layer: {}", pCallbackData->pMessage);

    return VK_FALSE;
}

VkResult createDebugUtilsMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo) 
{
    createInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose 
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning 
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral 
        | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
        | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    createInfo.pfnUserCallback = debugCallback;
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && 
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return vk::PresentModeKHR::eMailbox;
        }
    }

    return vk::PresentModeKHR::eFifo;  
}

vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities, Window const& window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window.get(), &width, &height);

        vk::Extent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }
}

[[nodiscard]] QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, vk::SurfaceKHR const& surface) noexcept;
[[nodiscard]] bool checkDeviceExtensionSupport(VkPhysicalDevice device) noexcept;

Window::Window(GLFWwindow * window) noexcept
    : window_(window)
{}

Window::~Window() noexcept
{
    if (window_) {
        glfwDestroyWindow(window_);
    }
}

Window& Window::operator=(Window&& rhs) noexcept 
{
    if (this != std::addressof(rhs)) {
        if (window_) {
            glfwDestroyWindow(window_);
        }
        window_ = std::exchange(rhs.window_, nullptr);
    }
    return *this;
}

tl::expected<Window, std::runtime_error> Window::create(int width, int height) noexcept
{
    spdlog::info("create window");
    gsl_Expects(width > 0);
    gsl_Expects(height > 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(width, height, "vkmd2v", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    spdlog::info("{} extensions supported", extensionCount);   
    return Window{window}; 
}

tl::expected<vk::raii::Instance, std::runtime_error> createInstance(vk::raii::Context& context) noexcept
{
    spdlog::info("create instance");
    vk::ApplicationInfo appInfo("vkmd2v", 1, "No Engine", 1, VK_API_VERSION_1_1);

    auto availableLayers = context.enumerateInstanceLayerProperties();

    for (auto layerName : validationLayers) {
        std::string_view sv{layerName};
        auto iter = ranges::find_if(availableLayers, [sv](auto const& layer) {
            return sv == std::string_view{layer.layerName};
        });
        if (iter == ranges::end(availableLayers)) {
            tl::make_unexpected(std::runtime_error(fmt::format("validation layer not available {}", sv)));
        } 
        spdlog::info("found validation layer {}", sv);
    }

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    populateDebugMessengerCreateInfo(debugCreateInfo);

    vk::InstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    char const** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<char const*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    createInfo.pNext = std::addressof(debugCreateInfo);
    
    try {
        return vk::raii::Instance(context, createInfo);
    } catch (std::exception const& excp) {
        return tl::make_unexpected(std::runtime_error(excp.what()));
    }
}

tl::expected<vk::raii::DebugUtilsMessengerEXT, std::runtime_error> createDebugUtilsMessenger(vk::raii::Instance& instance) noexcept
{
    vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);
    try {
        return vk::raii::DebugUtilsMessengerEXT{instance, createInfo};
    } catch (std::exception const& excp) {
        return tl::make_unexpected(std::runtime_error(excp.what()));
    }
}

tl::expected<vk::raii::Device, std::runtime_error> 
createDevice(vk::raii::PhysicalDevice const& physicalDevice, QueueFamilyIndices const& queueFamilyIndices) noexcept
{
    spdlog::info("create logical device {} {}", 
        queueFamilyIndices.graphicsFamily.value(),
        queueFamilyIndices.presentFamily.value());
    static constexpr float queuePriority = 1.0f;

    vk::PhysicalDeviceFeatures deviceFeatures{};

    std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices.graphicsFamily.value(), 
                                              queueFamilyIndices.presentFamily.value()};

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueQueueFamilies.size());
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo{{}, queueFamily, 1, &queuePriority};
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::DeviceCreateInfo createInfo{{}, queueCreateInfos, validationLayers, deviceExtensions};
    return vk::raii::Device{physicalDevice, createInfo};
}

SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR const& surface) noexcept
{
    SwapChainSupportDetails details;
    details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    details.formats = physicalDevice.getSurfaceFormatsKHR(surface);
    details.presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
    return details;
}

tl::expected<std::pair<vk::raii::PhysicalDevice, QueueFamilyIndices>, std::runtime_error> 
pickPhysicalDevice(vk::raii::Instance& instance, vk::SurfaceKHR const& surface) noexcept
{
    spdlog::info("pick physical device");
    auto devices = instance.enumeratePhysicalDevices();

    for (const auto& device : devices) {
        auto const deviceProperties = device.getProperties();
        auto const isDiscrete = deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
        if (!isDiscrete) {
            continue;
        }
        spdlog::info("found GPU discrete {}", std::string_view{deviceProperties.deviceName});
        auto const queueFamilyIndices = findQueueFamilies(*device, surface);
        if (!queueFamilyIndices.isComplete()) {
            continue;
        }
        bool const extensionsSupported = checkDeviceExtensionSupport(*device);
        if (!extensionsSupported) {
            continue;
        }
        auto swapChainSupport = querySwapChainSupport(*device, surface);
        bool const swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        if (swapChainAdequate) {
            return std::make_pair(device, queueFamilyIndices);
        }
    }
    return tl::make_unexpected(std::runtime_error("no suitable device found"));
}

tl::expected<vk::raii::SurfaceKHR, std::runtime_error> createSurface(vk::raii::Instance& instance, Window const& window) noexcept
{
    spdlog::info("create surface");
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*instance, window.get(), nullptr, &surface) != VK_SUCCESS) {
        return tl::make_unexpected(std::runtime_error("failed to create window surface!"));
    }
    try {
        return vk::raii::SurfaceKHR{instance, surface}; 
    } catch (std::exception const& excp) {
        return tl::make_unexpected(std::runtime_error(excp.what()));
    }
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, vk::SurfaceKHR const& surface) noexcept
{
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (auto i{0}; i < queueFamilies.size(); ++i) {
        auto const& queueFamily = queueFamilies.at(i);
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
    }
    spdlog::info("queue family indices: {} {}", indices.graphicsFamily.value(), indices.presentFamily.value());
    return indices;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) noexcept
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

tl::expected<std::pair<vk::raii::SwapchainKHR, SwapChainSupportDetails>, std::runtime_error>
    createSwapChain(vk::raii::PhysicalDevice const& physicalDevice, 
                    vk::raii::Device const& device, 
                    Window const& window, 
                    vk::SurfaceKHR const& surface) noexcept
{
    spdlog::info("create swap chain");
    auto swapChainSupport = querySwapChainSupport(*physicalDevice, surface);
    swapChainSupport.surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    auto const presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    swapChainSupport.extent = chooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;   
    }
    
    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = swapChainSupport.surfaceFormat.format;
    createInfo.imageColorSpace = swapChainSupport.surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChainSupport.extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment; // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto const indices = findQueueFamilies(*physicalDevice, surface);
    std::array<uint32_t, 2> queueFamilyIndices = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent; 
        createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive; 
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque; // VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    try {
        return std::make_pair(vk::raii::SwapchainKHR(device, createInfo), swapChainSupport);
    } catch (std::exception const& excp) {
        spdlog::error(excp.what());
        return tl::make_unexpected(std::runtime_error(excp.what()));
    }
}

tl::expected<std::vector<vk::raii::ImageView>, std::runtime_error> 
createImageViews(vk::raii::Device const& device, std::vector<vk::Image>& images, SwapChainSupportDetails const& swapChainSupport)
{
    spdlog::info("create image views");
    std::vector<vk::raii::ImageView> views;
    views.reserve(images.size());

    for (auto const& image : images) {
        vk::ImageViewCreateInfo createInfo{};
        createInfo.image = image;
        createInfo.viewType = vk::ImageViewType::e2D; 
        createInfo.format =  swapChainSupport.surfaceFormat.format; 
        createInfo.components.r = vk::ComponentSwizzle::eIdentity; 
        createInfo.components.g = vk::ComponentSwizzle::eIdentity; 
        createInfo.components.b = vk::ComponentSwizzle::eIdentity; 
        createInfo.components.a = vk::ComponentSwizzle::eIdentity; 
        createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        try {
            views.emplace_back(device, createInfo);
        } catch (std::runtime_error const& err) {
            return tl::make_unexpected(err);
        }
    }

    return views;
}

ShaderModule::ShaderModule(VkShaderModule module, vk::Device const& device) noexcept
    : module_(module)
    , device_(device)
{}

ShaderModule::~ShaderModule() noexcept
{
    if (module_) {
        vkDestroyShaderModule(device_, *module_, nullptr);
    }
}

ShaderModule::ShaderModule(ShaderModule&& rhs) noexcept
    : module_(std::exchange(rhs.module_, std::nullopt))
    , device_(rhs.device_)
{}

ShaderModule& ShaderModule::operator=(ShaderModule&& rhs) noexcept
{
    if (this != std::addressof(rhs)) {
        if (module_) {
            vkDestroyShaderModule(device_, *module_, nullptr);
        }
        module_ = std::exchange(rhs.module_, std::nullopt);
        device_ = rhs.device_;        
    }
    return *this;
}

tl::expected<ShaderModule, std::runtime_error> 
ShaderModule::create(std::filesystem::path const& path, vk::Device const& device)
{
    std::vector<char> buffer;
    {
        std::ifstream inf(path, std::ios::ate | std::ios::binary);

        if (!inf.is_open()) {
            return tl::make_unexpected(
                std::runtime_error(fmt::format("failed to open file '{}'!", path)));
        }

        auto const fileSize = static_cast<size_t>(inf.tellg());
        inf.seekg(0);
        buffer.resize(fileSize);
        inf.read(buffer.data(), fileSize);
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<uint32_t const*>(buffer.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return tl::make_unexpected(std::runtime_error("failed to create shader module!"));
    }

    return ShaderModule{shaderModule, device};
}

Framebuffer::Framebuffer(VkFramebuffer buffer, vk::Device const& device) noexcept
    : buffer_(buffer)
    , device_(device)
{}

Framebuffer::~Framebuffer() noexcept
{
    if (buffer_) {
        vkDestroyFramebuffer(device_, *buffer_, nullptr);
    }
}

Framebuffer::Framebuffer(Framebuffer&& rhs) noexcept
    : buffer_(std::exchange(rhs.buffer_, std::nullopt))
    , device_(rhs.device_)
{}

Framebuffer& Framebuffer::operator=(Framebuffer&& rhs) noexcept
{
    if (this != std::addressof(rhs)) {
        if (buffer_) {
            vkDestroyFramebuffer(device_, *buffer_, nullptr);
        }        
        buffer_ = std::exchange(rhs.buffer_, std::nullopt);
        device_ = rhs.device_;
    }
    return *this;
}

tl::expected<std::vector<Framebuffer>, std::runtime_error>
Framebuffer::create(std::vector<vk::raii::ImageView> const& imageViews, 
                    InplaceRenderPass const& renderPass,
                    vk::Extent2D swapChainExtent,
                    vk::Device const& device) noexcept
{
    spdlog::info("create frame buffer");
    std::vector<VkFramebuffer> swapChainFramebuffers;
    swapChainFramebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); i++) {
        VkImageView attachments[] = {*imageViews[i]};
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            return tl::make_unexpected(std::runtime_error("failed to create framebuffer!"));
        }
    }

    std::vector<Framebuffer> buffers;
    buffers.reserve(swapChainFramebuffers.size());
    for (auto const scfb : swapChainFramebuffers) {
        buffers.emplace_back(scfb, device);
    }
    return buffers;
}

tl::expected<vk::raii::CommandPool, std::runtime_error> 
createCommandPool(vk::raii::Device const& device, QueueFamilyIndices const& indices) noexcept
{
    spdlog::info("creating command pool");   
    try {
        return device.createCommandPool(vk::CommandPoolCreateInfo(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer, indices.graphicsFamily.value()));
    } catch (std::runtime_error const& excp) {
            return tl::make_unexpected(excp);
    }
}

tl::expected<std::vector<vk::raii::Fence>, std::runtime_error>
    createFences(vk::raii::Device const& device, unsigned int numFences) noexcept
{
    std::vector<vk::raii::Fence> fences;
    fences.reserve(numFences);

    for (auto i{0U}; i < numFences; ++i) {
        try {
            fences.emplace_back(device.createFence(vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled}));
        } catch (std::runtime_error const& excp) {
            return tl::make_unexpected(excp);
        }
    }
    return fences;
}

tl::expected<std::vector<vk::raii::Semaphore>, std::runtime_error>
    createSemaphores(vk::raii::Device const& device, unsigned int numSemaphores) noexcept
{
    std::vector<vk::raii::Semaphore> semaphores;
    semaphores.reserve(numSemaphores);

    for (auto i{0U}; i < numSemaphores; ++i) {
        try {
            semaphores.emplace_back(device.createSemaphore(vk::SemaphoreCreateInfo{}));
        } catch (std::runtime_error const& excp) {
            return tl::make_unexpected(excp);
        }
    }
    return semaphores;   
}

}