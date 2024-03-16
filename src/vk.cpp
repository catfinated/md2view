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

namespace vk {

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

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR> const& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(std::vector<VkPresentModeKHR> const& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR const& capabilities, Window const& window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window.get(), &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }
}

[[nodiscard]] QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, Surface const& surface) noexcept;
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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto window = glfwCreateWindow(width, height, "vkmd2v", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    spdlog::info("{} extensions supported", extensionCount);   
    return Window{window}; 
}

Instance::Instance(VkInstance handle) noexcept
    : handle_(handle)
{}

Instance::~Instance() noexcept
{
    if (handle_) {
        vkDestroyInstance(*handle_, nullptr);
    }
}

Instance& Instance::operator=(Instance&& rhs) noexcept 
{
    if (this != std::addressof(rhs)) {
        if (handle_) {
            vkDestroyInstance(*handle_, nullptr);
        }        
        handle_ = std::exchange(rhs.handle_, std::nullopt);
    }
    return *this;
}

tl::expected<Instance, std::runtime_error> Instance::create() noexcept
{
    spdlog::info("create instance");
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "vkmd2v";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

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

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    populateDebugMessengerCreateInfo(debugCreateInfo);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
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
    
    VkInstance vkInstance;
    if (vkCreateInstance(&createInfo, nullptr, &vkInstance) != VK_SUCCESS) {
        return tl::make_unexpected(std::runtime_error("failed to create instance!"));
    } 
    return Instance{vkInstance};
}

DebugMessenger::DebugMessenger(Instance const& instance,  VkDebugUtilsMessengerEXT handle) noexcept
    : instance_(std::addressof(instance))
    , handle_(handle)
{}

DebugMessenger::~DebugMessenger() noexcept
{
    if (handle_) {
        destroyDebugUtilsMessenger(*instance_, *handle_, nullptr);
    }
}

DebugMessenger& DebugMessenger::operator=(DebugMessenger&& rhs) noexcept
{
    if (this != std::addressof(rhs)) {
        if (handle_) {
            destroyDebugUtilsMessenger(*instance_, *handle_, nullptr);  
        }
        instance_ = rhs.instance_;
        handle_ = std::exchange(rhs.handle_, std::nullopt);
    }
    return *this;
}

tl::expected<DebugMessenger, std::runtime_error> DebugMessenger::create(Instance const& instance) noexcept
{
    spdlog::info("create debug messenger");
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);
    VkDebugUtilsMessengerEXT debug_messenger;

    if (auto ret = createDebugUtilsMessenger(instance, &createInfo, nullptr, &debug_messenger); ret != VK_SUCCESS) {
        spdlog::error("failed to set up debug messenger {}", static_cast<int>(ret));
        return tl::make_unexpected(std::runtime_error("failed to set up debug messenger!"));
    }
    return DebugMessenger{instance, debug_messenger};
}

Device::Device(VkDevice device, PhysicalDevice const& physicalDevice) noexcept
    : device_(device)
{
    vkGetDeviceQueue(device, physicalDevice.queueFamilyIndices().graphicsFamily.value(), 0, &graphicsQueue_);
    vkGetDeviceQueue(device, physicalDevice.queueFamilyIndices().presentFamily.value(), 0, &presentQueue_);
}

Device::~Device() noexcept
{
    if (device_) {
        vkDestroyDevice(*device_, nullptr);
    }   
}

Device& Device::operator=(Device&& rhs) noexcept
{
    if (this != std::addressof(rhs)) {
        if (device_) {
            vkDestroyDevice(*device_, nullptr);
        }
        device_ = std::exchange(rhs.device_, std::nullopt);
        graphicsQueue_ = rhs.graphicsQueue_;
        presentQueue_ = rhs.presentQueue_;
    }
    return *this;
}

tl::expected<Device, std::runtime_error> Device::create(PhysicalDevice const& physicalDevice) noexcept
{
    spdlog::info("create logical device {} {}", physicalDevice.queueFamilyIndices().graphicsFamily.value(),
    physicalDevice.queueFamilyIndices().presentFamily.value());
    static constexpr float queuePriority = 1.0f;

    VkPhysicalDeviceFeatures deviceFeatures{};

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    auto const uniqueQueueFamilies = physicalDevice.uniqueQueueFamilies();

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures; 
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    VkDevice device;
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
       tl::make_unexpected(std::runtime_error("failed to create logical device!"));
    }
    return Device{device, physicalDevice};
}

SwapChainSupportDetails PhysicalDevice::querySwapChainSupport(Surface const& surface) const noexcept
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface, &details.capabilities);

    uint32_t formatCount;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface, &formatCount, nullptr);
        formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface, &presentModeCount, nullptr); 
        presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

tl::expected<PhysicalDevice, std::runtime_error> 
PhysicalDevice::pickPhysicalDevice(Instance const& instance, Surface const& surface) noexcept
{
    spdlog::info("pick physical device");
    uint32_t deviceCount = 0;
    if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr); deviceCount == 0) {
        return tl::make_unexpected(std::runtime_error("no Vulkan supported GPU found"));
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        auto const isDiscrete = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        if (!isDiscrete) {
            continue;
        }
        spdlog::info("found GPU discrete {}", deviceProperties.deviceName);
        auto const queueFamilyIndices = findQueueFamilies(device, surface);
        if (!queueFamilyIndices.isComplete()) {
            continue;
        }
        bool const extensionsSupported = checkDeviceExtensionSupport(device);
        if (!extensionsSupported) {
            continue;
        }
        PhysicalDevice physicalDevice{device, queueFamilyIndices};
        auto swapChainSupport = physicalDevice.querySwapChainSupport(surface);
        bool const swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        if (swapChainAdequate) {
            return physicalDevice;
        }
    }
    return tl::make_unexpected(std::runtime_error("no suitable device found"));
}

Surface::~Surface() noexcept
{
    if (surface_) {
        vkDestroySurfaceKHR(*instance_, *surface_, nullptr);
    }
}

Surface& Surface::operator=(Surface&& rhs) noexcept
{
    if (this != std::addressof(rhs)) {
        if (surface_) {
            vkDestroySurfaceKHR(*instance_, *surface_, nullptr);
        }    
        instance_ = rhs.instance_;
        surface_ = std::exchange(rhs.surface_, std::nullopt);   
    }
    return *this;
}

tl::expected<Surface, std::runtime_error> Surface::create(Instance const& instance, Window const& window) noexcept
{
    spdlog::info("create surface");
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window.get(), nullptr, &surface) != VK_SUCCESS) {
        return tl::make_unexpected(std::runtime_error("failed to create window surface!"));
    }
    return Surface{instance, surface};
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, Surface const& surface) noexcept
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

SwapChain::SwapChain(VkSwapchainKHR swapChain, 
              VkFormat swapChainImageFormat,
              VkExtent2D swapChainExtent,
              std::vector<VkImage> images, 
              Device const& device) noexcept
    : swapChainImageFormat_(swapChainImageFormat)
    , swapChainExtent_(swapChainExtent)
    , swapChain_(swapChain)
    , images_(std::move(images))
    , device_(std::addressof(device))
{}

SwapChain::~SwapChain() noexcept
{
    if (swapChain_) {
        vkDestroySwapchainKHR(*device_, *swapChain_, nullptr);
    }
}

SwapChain::SwapChain(SwapChain&& rhs) noexcept 
    : swapChain_(std::exchange(rhs.swapChain_, std::nullopt))
    , swapChainImageFormat_(rhs.swapChainImageFormat_)
    , swapChainExtent_(rhs.swapChainExtent_)
    , images_(std::move(rhs.images_))
    , device_(rhs.device_)
{}

SwapChain& SwapChain::operator=(SwapChain&& rhs) noexcept
{
    if (this != std::addressof(rhs)) {
        if (swapChain_) {
            vkDestroySwapchainKHR(*device_, *swapChain_, nullptr);
        }
        swapChain_ = std::exchange(rhs.swapChain_, std::nullopt);  
        swapChainImageFormat_ = rhs.swapChainImageFormat_;
        swapChainExtent_ = rhs.swapChainExtent_;
        images_ = std::move(rhs.images_);
        device_ = rhs.device_;
    }
    return *this;
}

tl::expected<SwapChain, std::runtime_error>
SwapChain::create(PhysicalDevice const& physicalDevice, Device const& device, Window const& window, Surface const& surface) noexcept
{
    spdlog::info("create swap chain");
    auto const swapChainSupport = physicalDevice.querySwapChainSupport(surface);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;   
    }
    
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto const indices = physicalDevice.queueFamilyIndices();
    std::array<uint32_t, 2> queueFamilyIndices = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain;
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        return tl::make_unexpected(std::runtime_error("failed to create swap chain!"));
    }

    std::vector<VkImage> swapChainImages;
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    return SwapChain{swapChain, surfaceFormat.format, extent, std::move(swapChainImages), device};
}

tl::expected<std::vector<ImageView>, std::runtime_error> SwapChain::createImageViews(Device const& device) const
{
    std::vector<VkImageView> swapChainImageViews;
    swapChainImageViews.resize(images_.size());

    for (size_t i = 0; i < images_.size(); ++i) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = images_.at(i);
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat_;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            tl::make_unexpected(std::runtime_error("failed to create image views!"));
        }
    }

    std::vector<ImageView> views;
    views.reserve(swapChainImageViews.size());
    for (auto const sciv : swapChainImageViews) {
        views.emplace_back(sciv, device);
    }
    return views;
}

ImageView::ImageView(VkImageView view, Device const& device) noexcept
    : view_(view)
    , device_(std::addressof(device))
{}

ImageView::~ImageView() noexcept
{
    if (view_) {
        vkDestroyImageView(*device_, *view_, nullptr);
    }
}

ImageView::ImageView(ImageView&& rhs) noexcept
    : view_(std::exchange(rhs.view_, std::nullopt))
    , device_(rhs.device_)
{}
    
ImageView& ImageView::operator=(ImageView&& rhs) noexcept
{
    if (this != std::addressof(rhs)) {
        if (view_) {
            vkDestroyImageView(*device_, *view_, nullptr); 
        }
        view_ = std::exchange(rhs.view_, std::nullopt);
        device_ = rhs.device_;
    }
    return *this;
}

ShaderModule::ShaderModule(VkShaderModule module, Device const& device) noexcept
    : module_(module)
    , device_(std::addressof(device))
{}

ShaderModule::~ShaderModule() noexcept
{
    if (module_) {
        vkDestroyShaderModule(*device_, *module_, nullptr);
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
            vkDestroyShaderModule(*device_, *module_, nullptr);
        }
        module_ = std::exchange(rhs.module_, std::nullopt);
        device_ = rhs.device_;        
    }
    return *this;
}

tl::expected<ShaderModule, std::runtime_error> 
ShaderModule::create(std::filesystem::path const& path, Device const& device)
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

Framebuffer::Framebuffer(VkFramebuffer buffer, Device const& device) noexcept
    : buffer_(buffer)
    , device_(std::addressof(device))
{}

Framebuffer::~Framebuffer() noexcept
{
    if (buffer_) {
        vkDestroyFramebuffer(*device_, *buffer_, nullptr);
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
            vkDestroyFramebuffer(*device_, *buffer_, nullptr);
        }        
        buffer_ = std::exchange(rhs.buffer_, std::nullopt);
        device_ = rhs.device_;
    }
    return *this;
}

tl::expected<std::vector<Framebuffer>, std::runtime_error>
Framebuffer::create(std::vector<ImageView> const& imageViews, 
                    InplaceRenderPass const& renderPass,
                    VkExtent2D swapChainExtent,
                    Device const& device) noexcept
{
    std::vector<VkFramebuffer> swapChainFramebuffers;
    swapChainFramebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); i++) {
        VkImageView attachments[] = {imageViews[i]};
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

CommandPool::CommandPool(VkCommandPool pool, Device const& device) noexcept
    : pool_(pool)
    , device_(std::addressof(device))
{}

CommandPool::~CommandPool() noexcept
{
    if (pool_) {
        vkDestroyCommandPool(*device_, *pool_, nullptr);
    }
}

CommandPool::CommandPool(CommandPool&& rhs) noexcept
    : pool_(std::exchange(rhs.pool_, std::nullopt))
    , device_(rhs.device_)
{}
    
CommandPool& CommandPool::operator=(CommandPool&& rhs) noexcept
{
    if (this != std::addressof(rhs)) {
        if (pool_) {
            vkDestroyCommandPool(*device_, *pool_, nullptr);
        }      
        pool_ = std::exchange(rhs.pool_, std::nullopt);
        device_ = rhs.device_; 
    }
    return *this;
}

tl::expected<CommandPool, std::runtime_error> 
CommandPool::create(PhysicalDevice const& physicalDevice, Device const& device) noexcept
{
    spdlog::info("creating command pool");
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = physicalDevice.queueFamilyIndices().graphicsFamily.value();

    VkCommandPool pool;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        return tl::make_unexpected(std::runtime_error("failed to create command pool!"));
    }

    return CommandPool{pool, device};
}

tl::expected<CommandBuffer, std::runtime_error> CommandPool::createBuffer() const noexcept
{
    spdlog::info("create command buffer");
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = *pool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer buffer;
    if (vkAllocateCommandBuffers(*device_, &allocInfo, &buffer) != VK_SUCCESS) {
        return tl::make_unexpected(std::runtime_error("failed to allocate command buffer!"));
    }
    return CommandBuffer{buffer};
}

tl::expected<CommandBufferVec, std::runtime_error> 
CommandPool::createBuffers(unsigned int numBuffers) const noexcept
{
    spdlog::info("create command buffers");
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = *pool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = numBuffers;

    CommandBufferVec buffers{numBuffers};
    if (vkAllocateCommandBuffers(*device_, &allocInfo, buffers.data()) != VK_SUCCESS) {
        return tl::make_unexpected(std::runtime_error("failed to allocate command buffers!"));
    }   

    return buffers;
}

Fence::Fence(VkFence fence, Device const& device) noexcept
    : fence_(fence)
    , device_(std::addressof(device))
{}

Fence::~Fence() noexcept
{
    if (fence_) {
        vkDestroyFence(*device_, *fence_, nullptr);
    }
}

Fence::Fence(Fence&& rhs) noexcept
    : fence_(std::exchange(rhs.fence_, std::nullopt))
    , device_(rhs.device_)
{}

Fence& Fence::operator=(Fence&& rhs) noexcept
{
    if (this != std::addressof(rhs)) {
        if (fence_) {
            vkDestroyFence(*device_, *fence_, nullptr);
        }       
        fence_ = std::exchange(rhs.fence_, std::nullopt);
        device_ = rhs.device_; 
    }
    return *this;
}

tl::expected<Fence, std::runtime_error> Fence::create(Device const& device) noexcept
{
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkFence fence;
    if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
        return tl::make_unexpected(std::runtime_error("failed to create fence"));
    }
    return Fence{fence, device};
}

tl::expected<std::vector<Fence>, std::runtime_error> 
Fence::createVec(Device const& device, unsigned int numFences) noexcept
{
    std::vector<Fence> fences;
    fences.reserve(numFences);

    for (auto i{0U}; i < numFences; ++i) {
        auto expectedFence = create(device);
        if (!expectedFence) {
            return tl::make_unexpected(std::move(expectedFence).error());
        }
        fences.emplace_back(std::move(expectedFence).value());
    }
    return fences;
}

Semaphore::Semaphore(VkSemaphore semaphore, Device const& device) noexcept
    : semaphore_(semaphore)
    , device_(std::addressof(device))
{}

Semaphore::~Semaphore() noexcept
{
    if (semaphore_) {
        vkDestroySemaphore(*device_, *semaphore_, nullptr);
    }
}

Semaphore::Semaphore(Semaphore&& rhs) noexcept
    : semaphore_(std::exchange(rhs.semaphore_, std::nullopt))
    , device_(rhs.device_)
{}

Semaphore& Semaphore::operator=(Semaphore&& rhs) noexcept
{
    if (this != std::addressof(rhs)) {
        if (semaphore_) {
            vkDestroySemaphore(*device_, *semaphore_, nullptr);
        }
        semaphore_ = std::exchange(rhs.semaphore_, std::nullopt);
        device_ = rhs.device_;
    }
    return *this;
}

tl::expected<Semaphore, std::runtime_error> Semaphore::create(Device const& device) noexcept
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphore semaphore;
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS) {
        return tl::make_unexpected(std::runtime_error("failed to create semaphore"));
    }
    return Semaphore{semaphore, device};
}

tl::expected<std::vector<Semaphore>, std::runtime_error>
Semaphore::createVec(Device const& device, unsigned int numSemaphores) noexcept
{
    std::vector<Semaphore> semaphores;
    semaphores.reserve(numSemaphores);

    for (auto i{0U}; i < numSemaphores; ++i) {
        auto expectedSemaphore = create(device);
        if (!expectedSemaphore) {
            return tl::make_unexpected(std::move(expectedSemaphore).error());
        }
        semaphores.emplace_back(std::move(expectedSemaphore).value());
    }
    return semaphores;   
}

}