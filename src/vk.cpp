#include "vk.hpp"

#include <range/v3/algorithm/find_if.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <array>
#include <limits>
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

}
