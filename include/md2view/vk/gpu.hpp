/// @file
/// Vulkan GPU class
#pragma once

#include "md2view/vk/buffer.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <concepts>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace VK {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    [[nodiscard]] bool isComplete() const noexcept {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;

    vk::SurfaceFormatKHR surfaceFormat;
    vk::Extent2D extent;
};

class GPU {
public:
    GPU() = default;
    ~GPU() = default;
    GPU(vk::raii::Instance& instance, vk::SurfaceKHR const& surface);

    GPU(GPU const&) = delete;
    GPU& operator=(GPU const&) = delete;
    GPU(GPU&&) = default;

    /// Only valid when this GPU is empty
    GPU& operator=(GPU&&) = default;

    [[nodiscard]] vk::raii::PhysicalDevice const& physical() const {
        return physicalDevice_;
    }
    [[nodiscard]] vk::raii::Device const& logical() const { return device_; }
    [[nodiscard]] QueueFamilyIndices const& queueFamilyIndices() const {
        return queueFamilyIndices_;
    }
    [[nodiscard]] vk::raii::Queue const& graphicsQueue() const {
        return graphicsQueue_;
    }
    [[nodiscard]] vk::raii::Queue const& presentQueue() const {
        return presentQueue_;
    }
    [[nodiscard]] vk::raii::CommandPool const& commandPool() const {
        return commandPool_;
    }

    [[nodiscard]] uint32_t
    findMemoryType(uint32_t typeFilter,
                   vk::MemoryPropertyFlags properties) const;

    /// Create a buffer bound to newly allocated device memory
    ///
    /// @param size The size of the buffer
    /// @param usage The usage flags for the buffer
    /// @param properties The memory properties for the buffer memory
    /// @throws vk::SystemError if a Vulkan call fails
    [[nodiscard]] BoundBuffer
    createBuffer(vk::DeviceSize size,
                 vk::BufferUsageFlags usage,
                 vk::MemoryPropertyFlags properties) const;

    [[nodiscard]] SwapChainSupportDetails
    swapChainSupport(vk::SurfaceKHR const& surface) const;

    void waitIdle();

    void submitOnce(std::invocable<vk::raii::CommandBuffer&> auto func) const {
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
