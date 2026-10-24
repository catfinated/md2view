#include "md2view/vk/buffer.hpp"
#include "md2view/vk/gpu.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VK {

BoundBuffer BoundBuffer::create(GPU const& gpu,
                                vk::DeviceSize size,
                                vk::BufferUsageFlags usage,
                                vk::MemoryPropertyFlags properties) {
    // first create the buffer
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    auto buf = gpu.logical().createBuffer(bufferInfo);

    // next allocate the memory
    auto const memRequirements = buf.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        gpu.findMemoryType(memRequirements.memoryTypeBits, properties);
    auto mem = gpu.logical().allocateMemory(allocInfo);

    // bind buffer to memory and return
    buf.bindMemory(*mem, 0UL);
    return BoundBuffer{
        .buffer = std::move(buf),
        .memory = std::move(mem),
        .size = size,
    };
}

BoundBuffer createDynamicVertexBuffer(GPU const& gpu, vk::DeviceSize size) {
    return BoundBuffer::create(gpu, size,
                               vk::BufferUsageFlagBits::eVertexBuffer,
                               vk::MemoryPropertyFlagBits::eHostVisible |
                                   vk::MemoryPropertyFlagBits::eHostCoherent);
}

BoundBuffer createStaticVertexBuffer(GPU const& gpu, vk::DeviceSize size) {
    return BoundBuffer::create(gpu, size,
                               vk::BufferUsageFlagBits::eTransferDst |
                                   vk::BufferUsageFlagBits::eVertexBuffer,
                               vk::MemoryPropertyFlagBits::eDeviceLocal);
}

BoundBuffer createIndexBuffer(GPU const& gpu, vk::DeviceSize size) {
    return BoundBuffer::create(gpu, size,
                               vk::BufferUsageFlagBits::eTransferDst |
                                   vk::BufferUsageFlagBits::eIndexBuffer,
                               vk::MemoryPropertyFlagBits::eDeviceLocal);
}

BoundBuffer createStagingBuffer(GPU const& gpu, vk::DeviceSize size) {
    return BoundBuffer::create(gpu, size, vk::BufferUsageFlagBits::eTransferSrc,
                               vk::MemoryPropertyFlagBits::eHostVisible |
                                   vk::MemoryPropertyFlagBits::eHostCoherent);
}

BoundBuffer createUniformBuffer(GPU const& gpu, vk::DeviceSize size) {
    auto buf =
        BoundBuffer::create(gpu, size, vk::BufferUsageFlagBits::eUniformBuffer,
                            vk::MemoryPropertyFlagBits::eHostVisible |
                                vk::MemoryPropertyFlagBits::eHostCoherent);
    buf.map();
    return buf;
}

void copyBuffer(BoundBuffer const& src,
                BoundBuffer const& dst,
                GPU const& gpu,
                vk::raii::CommandPool const& commandPool) {

    gsl_Assert(src.size == dst.size);
    vk::CommandBufferAllocateInfo allocInfo{
        *commandPool, vk::CommandBufferLevel::ePrimary, 1};

    auto commandBuffers = gpu.logical().allocateCommandBuffers(allocInfo);
    auto& commandBuffer = commandBuffers.at(0);

    vk::CommandBufferBeginInfo beginInfo{
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

    vk::BufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = src.size;

    commandBuffer.begin(beginInfo);
    commandBuffer.copyBuffer(*src.buffer, *dst.buffer, copyRegion);
    commandBuffer.end();

    vk::SubmitInfo submitInfo{nullptr, nullptr, *commandBuffer};

    gpu.graphicsQueue().submit(submitInfo, nullptr);
    gpu.graphicsQueue().waitIdle();
}

} // namespace VK
