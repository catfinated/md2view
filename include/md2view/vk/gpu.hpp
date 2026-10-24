/// @file
/// Vulkan GPU class
#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "md2view/vk/vk.hpp"

#include <concepts>
#include <cstdint>

namespace VK {

class GPU {
public:
    GPU() = default;
    ~GPU() = default;
    GPU(vk::raii::Instance& instance, vk::SurfaceKHR const& surface);

    GPU(GPU const&) = delete;
    GPU& operator=(GPU const&) = delete;
    GPU(GPU&&) = default;
    GPU& operator=(GPU&&) = default;

    vk::raii::PhysicalDevice const& physical() const { return physicalDevice_; }
    vk::raii::Device const& logical() const { return device_; }
    QueueFamilyIndices const& queueFamilyIndices() const {
        return queueFamilyIndices_;
    }
    vk::raii::Queue const& graphicsQueue() const { return graphicsQueue_; }
    vk::raii::Queue const& presentQueue() const { return presentQueue_; }
    vk::raii::CommandPool const& commandPool() const { return commandPool_; }

    uint32_t findMemoryType(uint32_t typeFilter,
                            vk::MemoryPropertyFlags properties) const;

    void waitIdle();

    void submitOnce(std::invocable<vk::CommandBuffer&> auto func) const {
        vk::CommandBufferAllocateInfo allocInfo{
            *commandPool_, vk::CommandBufferLevel::ePrimary, 1};

        auto commandBuffers = device_.allocateCommandBuffers(allocInfo);
        auto& commandBuffer = commandBuffers.at(0);

        vk::CommandBufferBeginInfo beginInfo{
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

        commandBuffer.begin(beginInfo);
        func(commandBuffer);
        commandBuffer.end();

        vk::SubmitInfo submitInfo{nullptr, nullptr, *commandBuffer};

        graphicsQueue_.submit(submitInfo, nullptr);
        graphicsQueue_.waitIdle();
    }

private:
    explicit GPU(
        std::pair<vk::raii::PhysicalDevice, QueueFamilyIndices>&& args);

    vk::raii::PhysicalDevice physicalDevice_{nullptr};
    QueueFamilyIndices queueFamilyIndices_;
    vk::PhysicalDeviceMemoryProperties memoryProperties_;
    vk::raii::Device device_{nullptr};
    vk::raii::Queue graphicsQueue_{nullptr};
    vk::raii::Queue presentQueue_{nullptr};
    vk::raii::CommandPool commandPool_{nullptr};
};

} // namespace VK
