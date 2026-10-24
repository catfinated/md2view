#include "md2view/vk/gpu.hpp"

#include <gsl-lite/gsl-lite.hpp>

namespace VK {

GPU::GPU(vk::raii::Instance& instance, vk::SurfaceKHR const& surface)
    : GPU{pickPhysicalDevice(instance, surface)} {}

GPU::GPU(std::pair<vk::raii::PhysicalDevice, QueueFamilyIndices>&& args)
    : physicalDevice_{std::move(args).first}
    , queueFamilyIndices_{std::move(args).second}
    , memoryProperties_{physicalDevice_.getMemoryProperties()}
    , device_{createDevice(physicalDevice_, queueFamilyIndices_)}
    , graphicsQueue_{device_.getQueue(
          queueFamilyIndices_.graphicsFamily.value(), 0)}
    , presentQueue_{device_.getQueue(queueFamilyIndices_.presentFamily.value(),
                                     0)}
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

} // namespace VK
