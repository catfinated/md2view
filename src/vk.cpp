#include "vk.hpp"

#include <range/v3/algorithm/find_if.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <array>
#include <string_view>

namespace vk {
namespace {

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

Window::Window(GLFWwindow * window)
    : window_(window)
{}

Window::~Window()
{
    if (window_) {
        glfwDestroyWindow(window_);
    }
}

tl::expected<Window, std::runtime_error> Window::create(int width, int height)
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

Instance::Instance(VkInstance handle)
    : handle_(handle)
{}

Instance::~Instance()
{
    if (handle_) {
        vkDestroyInstance(*handle_, nullptr);
    }
}

tl::expected<Instance, std::runtime_error> Instance::create()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "vkmd2v";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    static constexpr std::array<char const*, 1> validationLayers = {
        "VK_LAYER_KHRONOS_validation"    
    };

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
            throw std::runtime_error(
                fmt::format("validation layer not available {}", sv));
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

DebugMessenger::DebugMessenger(Instance& instance,  VkDebugUtilsMessengerEXT handle)
    : instance_(std::addressof(instance))
    , handle_(handle)
{}

DebugMessenger::~DebugMessenger()
{
    if (handle_) {
        destroy_debug_utils_messenger(*instance_, *handle_, nullptr);
    }
}

tl::expected<DebugMessenger, std::runtime_error> DebugMessenger::create(Instance& instance)
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

}