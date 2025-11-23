/**
 * @brief Types and functions related to Vulkan buffers
 */
#pragma once

#include <gsl-lite/gsl-lite.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstring>

namespace md2v {

/**
 * @brief Buffer bound to device memory
 *
 * In Vulkan, a buffer has to be bound to some associated
 * device memory. This struct manages both the buffer and the
 * the memory together to simplify creation and usage.
 */
struct BoundBuffer {
    vk::raii::Buffer buffer{nullptr};
    vk::raii::DeviceMemory memory{nullptr};
    vk::DeviceSize size{};

    /**
     * @brief Copy bytes into the buffer
     *
     * @param vec Source vector
     */
    template <typename T> void memcpy(std::vector<T> const& vec) const {
        auto const srcSize = sizeof(T) * vec.size();
        gsl_Assert(srcSize == size);
        auto* dst = memory.mapMemory(0U, size);
        std::memcpy(dst, vec.data(), size);
        memory.unmapMemory();
    }

    /**
     * @brief Create BoundBuffer
     *
     * @param device The logical device to create the buffer for
     * @param physicalDevice The physical device to allocate memory on
     * @param size The size of the buffer
     * @param usage The usage flags for the buffer
     * @param properties The memory properties for the buffer memory
     * @return An expected BoundBuffer or an error
     */
    static std::expected<BoundBuffer, std::runtime_error>
    create(vk::raii::Device const& device,
           vk::raii::PhysicalDevice const& physicalDevice,
           vk::DeviceSize size,
           vk::BufferUsageFlags usage,
           vk::MemoryPropertyFlags properties) noexcept;
};

/**
 * @brief Create a BoundBuffer suitable for dynamic vertex data
 *
 * This will be a host visible buffer that can be mapped to cpu
 * accessible memory.
 *
 * @param device The logical device to create the buffer for
 * @param physicalDevice The physical device to allocate memory on
 * @param size The size of the buffer
 * @return An expected BoundBuffer or an error
 */
std::expected<BoundBuffer, std::runtime_error>
createDynamicVertexBuffer(vk::raii::Device const& device,
                          vk::raii::PhysicalDevice const& physicalDevice,
                          vk::DeviceSize size) noexcept;

/**
 * @brief Create a BoundBuffer suitable for static vertex data
 *
 * This will be a device local buffer that can not be mapped to cpu
 * accessible memory.
 *
 * @param device The logical device to create the buffer for
 * @param physicalDevice The physical device to allocate memory on
 * @param size The size of the buffer
 * @return An expected BoundBuffer or an error
 */
std::expected<BoundBuffer, std::runtime_error>
createStaticVertexBuffer(vk::raii::Device const& device,
                         vk::raii::PhysicalDevice const& physicalDevice,
                         vk::DeviceSize size) noexcept;

/**
 * @brief Create a BoundBuffer suitable for index data
 *
 * This will be a device local buffer that can not be mapped to cpu
 * accessible memory.
 *
 * @param device The logical device to create the buffer for
 * @param physicalDevice The physical device to allocate memory on
 * @param size The size of the buffer
 * @return An expected BoundBuffer or an error
 */
std::expected<BoundBuffer, std::runtime_error>
createIndexBuffer(vk::raii::Device const& device,
                  vk::raii::PhysicalDevice const& physicalDevice,
                  vk::DeviceSize size) noexcept;

/**
 * @brief Create a BoundBuffer suitable for staging data
 *
 * @param device The logical device to create the buffer for
 * @param physicalDevice The physical device to allocate memory on
 * @param size The size of the buffer
 * @return An expected BoundBuffer or an error
 */
std::expected<BoundBuffer, std::runtime_error>
createStagingBuffer(vk::raii::Device const& device,
                    vk::raii::PhysicalDevice const& physicalDevice,
                    vk::DeviceSize size) noexcept;

/**
 * @brief Copy a local buffer to a device buffer
 *
 * @param src The local buffer
 * @param dst The device buffer
 * @param device The logical device to perform the operation on
 * @param commandPool The command pool to allocate command buffers from
 * @param graphicsQueue The queue to submit the copy command to
 */
void copyBuffer(BoundBuffer const& src,
                BoundBuffer const& dst,
                vk::raii::Device const& device,
                vk::raii::CommandPool const& commandPool,
                vk::raii::Queue const& graphicsQueue);

} // namespace md2v
