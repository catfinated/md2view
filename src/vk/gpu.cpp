#include "md2view/vk/gpu.hpp"

#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace VK {

namespace {

#ifdef __APPLE__
// MoltenVK is a portability-subset implementation
constexpr std::array<char const*, 2> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_portability_subset",
};
#else
constexpr std::array<char const*, 1> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
#endif

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
    std::set<std::string> requiredExtensions(kDeviceExtensions.begin(),
                                             kDeviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
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
#ifdef __APPLE__
        // Apple GPUs are reported as integrated.
        auto const isSuitable =
            isDiscrete || deviceProperties.deviceType ==
                              vk::PhysicalDeviceType::eIntegratedGpu;
#else
        auto const isSuitable = isDiscrete;
#endif
        if (!isSuitable) {
            continue;
        }
        spdlog::info("found GPU {}",
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

vk::raii::Device createDevice(vk::raii::PhysicalDevice const& physicalDevice,
                              QueueFamilyIndices const& queueFamilyIndices) {
    spdlog::info("create logical device");
    static constexpr float queuePriority = 1.0f;

    std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilyIndices.graphicsFamily.value(),
        queueFamilyIndices.presentFamily.value(),
    };

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueQueueFamilies.size());
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo{
            {}, queueFamily, 1, &queuePriority};
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // NB: device layers are deprecated and ignored; the instance layers
    // already cover the device
    vk::DeviceCreateInfo createInfo{
        {}, queueCreateInfos, {}, kDeviceExtensions};
    return vk::raii::Device{physicalDevice, createInfo};
}

vk::raii::Queue getQueue(vk::raii::Device const& device,
                         std::optional<uint32_t> family) {
    return device.getQueue(family.value(), 0);
}

vk::raii::CommandPool createCommandPool(vk::raii::Device const& device,
                                        QueueFamilyIndices const& indices) {
    spdlog::info("creating command pool");
    return device.createCommandPool(vk::CommandPoolCreateInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        indices.graphicsFamily.value()));
}

} // namespace

GPU::GPU(vk::raii::Instance& instance, vk::SurfaceKHR const& surface)
    : GPU{pickPhysicalDevice(instance, surface)} {}

GPU::GPU(std::pair<vk::raii::PhysicalDevice, QueueFamilyIndices>&& args)
    : physicalDevice_{std::move(args).first}
    , queueFamilyIndices_{std::move(args).second}
    , memoryProperties_{physicalDevice_.getMemoryProperties()}
    , device_{createDevice(physicalDevice_, queueFamilyIndices_)}
    , graphicsQueue_{getQueue(device_, queueFamilyIndices_.graphicsFamily)}
    , presentQueue_{getQueue(device_, queueFamilyIndices_.presentFamily)}
    , commandPool_{createCommandPool(device_, queueFamilyIndices_)} {}

void GPU::waitIdle() {
    if (*device_ != nullptr) {
        device_.waitIdle();
    }
}

uint32_t GPU::findMemoryType(uint32_t typeFilter,
                             vk::MemoryPropertyFlags properties) const {
    for (uint32_t i = 0; i < memoryProperties_.memoryTypeCount; i++) {
        auto const hasType = (typeFilter & (1 << i)) != 0U;
        auto const propFlags =
            gsl_lite::at(memoryProperties_.memoryTypes, i).propertyFlags;
        auto const hasMemProps = (propFlags & properties) == properties;

        if (hasType && hasMemProps) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

SwapChainSupportDetails
GPU::swapChainSupport(vk::SurfaceKHR const& surface) const {
    return querySwapChainSupport(physicalDevice_, surface);
}

} // namespace VK
