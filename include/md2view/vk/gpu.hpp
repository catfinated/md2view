/// @file
/// Vulkan GPU class
#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "md2view/vk/vk.hpp"

namespace VK {

class GPU {
public:
    GPU() = default;
    GPU(vk::raii::Instance& instance, vk::SurfaceKHR const& surface);

private:
    explicit GPU(
        std::pair<vk::raii::PhysicalDevice, QueueFamilyIndices>&& args);

    vk::raii::PhysicalDevice physicalDevice_{nullptr};
    QueueFamilyIndices queueFamilyIndices_;
    vk::PhysicalDeviceMemoryProperties memoryProperties_;
    vk::raii::Device device_{nullptr};
    vk::raii::Queue graphicsQueue_{nullptr};
    vk::raii::Queue presentQueue_{nullptr};
};

} // namespace VK
