/// @file
/// Types and functions related to Vulkan buffers
#pragma once

#include <gsl-lite/gsl-lite.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstring>
#include <type_traits>
#include <vector>

namespace VK {

class GPU;

/// Buffer bound to device memory
///
/// In Vulkan, a buffer has to be bound to some associated
/// device memory. This struct manages both the buffer and the
/// the memory together to simplify creation and usage.
struct BoundBuffer {
    vk::raii::Buffer buffer{nullptr};
    vk::raii::DeviceMemory memory{nullptr};
    vk::DeviceSize size{};
    void* mapped{nullptr}; ///< non-null if persistently mapped

    /// Persistently map the whole buffer
    ///
    /// Memory must be host visible. The mapping is released implicitly
    /// when the memory is freed.
    ///
    /// @throws vk::SystemError if a Vulkan call fails
    void map() {
        gsl_Expects(mapped == nullptr);
        mapped = memory.mapMemory(0U, size);
    }

    /// Copy bytes into the buffer
    ///
    /// @param vec Source vector
    template <typename T> void memcpy(std::vector<T> const& vec) const {
        copyBytes(vec.data(), sizeof(T) * vec.size());
    }

    /// Copy a single object into the buffer
    ///
    /// @param obj Source object
    template <typename T>
        requires std::is_trivially_copyable_v<T>
    void memcpy(T const& obj) const {
        copyBytes(&obj, sizeof(T));
    }

    /// Create BoundBuffer
    ///
    /// @param gpu Gpu to allocate memory on
    /// @param size The size of the buffer
    /// @param usage The usage flags for the buffer
    /// @param properties The memory properties for the buffer memory
    /// @return The created BoundBuffer
    /// @throws vk::SystemError if a Vulkan call fails
    static BoundBuffer create(GPU const& gpu,
                              vk::DeviceSize size,
                              vk::BufferUsageFlags usage,
                              vk::MemoryPropertyFlags properties);

private:
    void copyBytes(void const* src, std::size_t srcSize) const {
        gsl_Assert(srcSize == size);
        if (mapped != nullptr) {
            std::memcpy(mapped, src, srcSize);
        } else {
            auto* dst = memory.mapMemory(0U, size);
            std::memcpy(dst, src, srcSize);
            memory.unmapMemory();
        }
    }
};

/// Create a BoundBuffer suitable for dynamic vertex data
///
/// This will be a host visible buffer that can be mapped to cpu
/// accessible memory.
///
/// @param gpu Gpu to allocate memory on
/// @param size The size of the buffer
/// @return The created BoundBuffer
/// @throws vk::SystemError if a Vulkan call fails
BoundBuffer createDynamicVertexBuffer(GPU const& gpu, vk::DeviceSize size);

/// Create a BoundBuffer suitable for static vertex data
///
/// This will be a device local buffer that can not be mapped to cpu
/// accessible memory.
///
/// @param gpu Gpu to allocate memory on
/// @param size The size of the buffer
/// @return The created BoundBuffer
/// @throws vk::SystemError if a Vulkan call fails
BoundBuffer createStaticVertexBuffer(GPU const& gpu, vk::DeviceSize size);

/// Create a BoundBuffer suitable for index data
///
/// This will be a device local buffer that can not be mapped to cpu
/// accessible memory.
///
/// @param gpu Gpu to allocate memory on
/// @param size The size of the buffer
/// @return The created BoundBuffer
/// @throws vk::SystemError if a Vulkan call fails
BoundBuffer createIndexBuffer(GPU const& gpu, vk::DeviceSize size);

/// Create a BoundBuffer suitable for staging data
///
/// @param gpu Gpu to allocate memory on
/// @param size The size of the buffer
/// @return The created BoundBuffer
/// @throws vk::SystemError if a Vulkan call fails
BoundBuffer createStagingBuffer(GPU const& gpu, vk::DeviceSize size);

/// Create a persistently mapped BoundBuffer for uniform data
///
/// This will be a host visible, host coherent buffer that is mapped
/// for its entire lifetime.
///
/// @param gpu Gpu to allocate memory on
/// @param size The size of the buffer
/// @return The created BoundBuffer
/// @throws vk::SystemError if a Vulkan call fails
BoundBuffer createUniformBuffer(GPU const& gpu, vk::DeviceSize size);

/// Copy a local buffer to a device buffer
///
/// @param src The local buffer
/// @param dst The device buffer
/// @param gpu The gpu to perform the operation on
void copyBuffer(BoundBuffer const& src, BoundBuffer const& dst, GPU const& gpu);

} // namespace VK
