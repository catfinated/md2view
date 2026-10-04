/**
 * @brief Vulkan renderer utility types and methods
 */
#pragma once

#include "md2view/vk/buffer.hpp" // IWYU pragma: export
#include "md2view/vk/vertex.hpp" // IWYU pragma: export

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <vector>

namespace VK {
class Window;

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

[[nodiscard]] SwapChainSupportDetails
querySwapChainSupport(vk::PhysicalDevice physicalDevice,
                      vk::SurfaceKHR const& surface);

vk::raii::Instance createInstance(vk::raii::Context& context);

vk::raii::DebugUtilsMessengerEXT
createDebugUtilsMessenger(vk::raii::Instance& instance);

vk::raii::SurfaceKHR createSurface(vk::raii::Instance& instance,
                                   Window const& window);

std::pair<vk::raii::PhysicalDevice, QueueFamilyIndices>
pickPhysicalDevice(vk::raii::Instance& instance, vk::SurfaceKHR const& surface);

vk::raii::Device createDevice(vk::raii::PhysicalDevice const& physicalDevice,
                              QueueFamilyIndices const& queueFamilyIndices);

std::vector<vk::raii::Semaphore>
createSemaphores(vk::raii::Device const& device, unsigned int numSemaphores);

std::vector<vk::raii::Fence> createFences(vk::raii::Device const& device,
                                          unsigned int numFences);

vk::raii::CommandPool createCommandPool(vk::raii::Device const& device,
                                        QueueFamilyIndices const& indices);

std::pair<vk::raii::SwapchainKHR, SwapChainSupportDetails>
createSwapChain(vk::raii::PhysicalDevice const& physicalDevice,
                vk::raii::Device const& device,
                Window const& window,
                vk::SurfaceKHR const& surface,
                QueueFamilyIndices const& queueFamilyIndices);

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
