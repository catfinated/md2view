#include "vk.hpp"

#include <range/v3/algorithm/find_if.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <array>
#include <string_view>
#include <vector>

namespace vk {
namespace {

static constexpr std::array<char const*, 1> validationLayers = {
    "VK_LAYER_KHRONOS_validation"    
};

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    spdlog::info("validation layer: {}", pCallbackData->pMessage);

    return VK_FALSE;
}

VkResult create_debug_utils_messenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroy_debug_utils_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = vk_debug_callback;
}

}

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
    populate_debug_messenger_create_info(debugCreateInfo);

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
        destroy_debug_utils_messenger(*instance_, *handle_, nullptr);
    }
}

DebugMessenger& DebugMessenger::operator=(DebugMessenger&& rhs) noexcept
{
    if (this != std::addressof(rhs)) {
        if (handle_) {
            destroy_debug_utils_messenger(*instance_, *handle_, nullptr);  
        }
        instance_ = rhs.instance_;
        handle_ = std::exchange(rhs.handle_, std::nullopt);
    }
    return *this;
}

tl::expected<DebugMessenger, std::runtime_error> DebugMessenger::create(Instance const& instance) noexcept
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populate_debug_messenger_create_info(createInfo);
    VkDebugUtilsMessengerEXT debug_messenger;

    if (auto ret = create_debug_utils_messenger(instance, &createInfo, nullptr, &debug_messenger); ret != VK_SUCCESS) {
        spdlog::error("failed to set up debug messenger {}", static_cast<int>(ret));
        return tl::make_unexpected(std::runtime_error("failed to set up debug messenger!"));
    }
    return DebugMessenger{instance, debug_messenger};
}

Device::Device(VkDevice device, PhysicalDevice const& physicalDevices) noexcept
    : device_(device)
{
    vkGetDeviceQueue(device, physicalDevices.queueFamilyIndices().graphicsFamily.value(), 0, &graphicsQueue_);
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
    static constexpr float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = physicalDevice.queueFamilyIndices().graphicsFamily.value();
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 0; 
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    VkDevice device;
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
       tl::make_unexpected(std::runtime_error("failed to create logical device!"));
    }
    return Device{device, physicalDevice};
}

tl::expected<PhysicalDevice, std::runtime_error> PhysicalDevice::pickPhysicalDevice(Instance const& instance) noexcept
{
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
        auto const queueFamilyIndices = findQueueFamilies(device);
        if (queueFamilyIndices.isComplete()) {
            return PhysicalDevice{device, queueFamilyIndices};
        }
    }
    return tl::make_unexpected(std::runtime_error("no suitable device found"));
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) noexcept
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
        if (indices.isComplete()) {
            break;
        }
    }
    return indices;
}

}