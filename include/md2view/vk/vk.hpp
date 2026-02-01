/**
 * @brief Vulkan renderer utility types and methods
 */
#pragma once

#include "md2view/vk/buffer.hpp" // IWYU pragma: export
#include "md2view/vk/vertex.hpp" // IWYU pragma: export
#include "md2view/vk/window.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <vector>

namespace md2v {

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
                      vk::SurfaceKHR const& surface) noexcept;

std::expected<vk::raii::Instance, std::runtime_error>
createInstance(vk::raii::Context& context) noexcept;

std::expected<vk::raii::DebugUtilsMessengerEXT, std::runtime_error>
createDebugUtilsMessenger(vk::raii::Instance& instance) noexcept;

std::expected<vk::raii::SurfaceKHR, std::runtime_error>
createSurface(vk::raii::Instance& instance, Window const& window) noexcept;

std::expected<std::pair<vk::raii::PhysicalDevice, QueueFamilyIndices>,
              std::runtime_error>
pickPhysicalDevice(vk::raii::Instance& instance,
                   vk::SurfaceKHR const& surface) noexcept;

std::expected<vk::raii::Device, std::runtime_error>
createDevice(vk::raii::PhysicalDevice const& physicalDevice,
             QueueFamilyIndices const& queueFamilyIndices) noexcept;

std::expected<std::vector<vk::raii::Semaphore>, std::runtime_error>
createSemaphores(vk::raii::Device const& device,
                 unsigned int numSemaphores) noexcept;

std::expected<std::vector<vk::raii::Fence>, std::runtime_error>
createFences(vk::raii::Device const& device, unsigned int numFences) noexcept;

std::expected<vk::raii::CommandPool, std::runtime_error>
createCommandPool(vk::raii::Device const& device,
                  QueueFamilyIndices const& indices) noexcept;

std::expected<std::pair<vk::raii::SwapchainKHR, SwapChainSupportDetails>,
              std::runtime_error>
createSwapChain(vk::raii::PhysicalDevice const& physicalDevice,
                vk::raii::Device const& device,
                Window const& window,
                vk::SurfaceKHR const& surface,
                QueueFamilyIndices const& queueFamilyIndices) noexcept;

std::expected<std::vector<vk::raii::ImageView>, std::runtime_error>
createImageViews(vk::raii::Device const& device,
                 std::vector<vk::Image>& images,
                 SwapChainSupportDetails const& support);

std::expected<vk::raii::ShaderModule, std::runtime_error>
createShaderModule(std::filesystem::path const& path,
                   vk::raii::Device const& device) noexcept;

std::expected<std::vector<vk::raii::Framebuffer>, std::runtime_error>
createFrameBuffers(std::vector<vk::raii::ImageView> const& imageViews,
                   vk::raii::RenderPass const& renderPass,
                   vk::Extent2D swapChainExtent,
                   vk::raii::Device const& device) noexcept;

} // namespace md2v
