#include "md2view/vk/buffer.hpp"
#include "md2view/vk/gpu.hpp"

#include <gsl-lite/gsl-lite.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VK {

BoundBuffer createDynamicVertexBuffer(GPU const& gpu, vk::DeviceSize size) {
    return gpu.createBuffer(size, vk::BufferUsageFlagBits::eVertexBuffer,
                            vk::MemoryPropertyFlagBits::eHostVisible |
                                vk::MemoryPropertyFlagBits::eHostCoherent);
}

BoundBuffer createStaticVertexBuffer(GPU const& gpu, vk::DeviceSize size) {
    return gpu.createBuffer(size,
                            vk::BufferUsageFlagBits::eTransferDst |
                                vk::BufferUsageFlagBits::eVertexBuffer,
                            vk::MemoryPropertyFlagBits::eDeviceLocal);
}

BoundBuffer createIndexBuffer(GPU const& gpu, vk::DeviceSize size) {
    return gpu.createBuffer(size,
                            vk::BufferUsageFlagBits::eTransferDst |
                                vk::BufferUsageFlagBits::eIndexBuffer,
                            vk::MemoryPropertyFlagBits::eDeviceLocal);
}

BoundBuffer createStagingBuffer(GPU const& gpu, vk::DeviceSize size) {
    return gpu.createBuffer(size, vk::BufferUsageFlagBits::eTransferSrc,
                            vk::MemoryPropertyFlagBits::eHostVisible |
                                vk::MemoryPropertyFlagBits::eHostCoherent);
}

BoundBuffer createUniformBuffer(GPU const& gpu, vk::DeviceSize size) {
    auto buf = gpu.createBuffer(size, vk::BufferUsageFlagBits::eUniformBuffer,
                                vk::MemoryPropertyFlagBits::eHostVisible |
                                    vk::MemoryPropertyFlagBits::eHostCoherent);
    buf.map();
    return buf;
}

void copyBuffer(BoundBuffer const& src,
                BoundBuffer const& dst,
                GPU const& gpu) {

    gsl_Assert(src.size == dst.size);
    vk::BufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = src.size;

    gpu.submitOnce([&](auto& cmdBuf) {
        cmdBuf.copyBuffer(*src.buffer, *dst.buffer, copyRegion);
    });
}

} // namespace VK
