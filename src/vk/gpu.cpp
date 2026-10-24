#include "md2view/vk/gpu.hpp"

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
    , presentQueue_{
          device_.getQueue(queueFamilyIndices_.presentFamily.value(), 0)} {}

void GPU::waitIdle() {
    if (*device_ != nullptr) {
        device_.waitIdle();
    }
}

} // namespace VK
