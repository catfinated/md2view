/// @file
/// Vulkan renderer utility types and methods
#pragma once

#include "md2view/vk/buffer.hpp" // IWYU pragma: export
#include "md2view/vk/gpu.hpp"    // IWYU pragma: export
#include "md2view/vk/vertex.hpp" // IWYU pragma: export

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <utility>
#include <vector>

class Window;

namespace VK {

vk::raii::Instance createInstance(vk::raii::Context& context);

vk::raii::DebugUtilsMessengerEXT
createDebugUtilsMessenger(vk::raii::Instance& instance);

vk::raii::SurfaceKHR createSurface(vk::raii::Instance& instance,
                                   Window const& window);

std::vector<vk::raii::Semaphore>
createSemaphores(vk::raii::Device const& device, unsigned int numSemaphores);

std::vector<vk::raii::Fence> createFences(vk::raii::Device const& device,
                                          unsigned int numFences);

std::pair<vk::raii::SwapchainKHR, SwapChainSupportDetails> createSwapChain(
    GPU const& gpu, Window const& window, vk::SurfaceKHR const& surface);

std::vector<vk::raii::ImageView>
createImageViews(vk::raii::Device const& device,
                 std::vector<vk::Image>& images,
                 SwapChainSupportDetails const& support);

vk::raii::ShaderModule createShaderModule(std::filesystem::path const& path,
                                          vk::raii::Device const& device);

std::vector<vk::raii::Framebuffer>
createFrameBuffers(std::vector<vk::raii::ImageView> const& imageViews,
                   vk::raii::RenderPass const& renderPass,
                   vk::Extent2D swapChainExtent,
                   vk::raii::Device const& device);

vk::raii::DescriptorSetLayout
createDescriptorSetLayout(vk::raii::Device const& device);

vk::raii::DescriptorPool createDescriptorPool(vk::raii::Device const& device,
                                              unsigned int maxFramesInFlight);

std::vector<vk::raii::DescriptorSet>
createDescriptorSets(vk::raii::Device const& device,
                     vk::raii::DescriptorPool const& pool,
                     vk::raii::DescriptorSetLayout const& layout,
                     unsigned int maxFramesInFlight);

} // namespace VK
