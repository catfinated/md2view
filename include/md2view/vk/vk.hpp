#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <gsl-lite/gsl-lite.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace myvk {

class Window {
public:
    Window(GLFWwindow* window = nullptr) noexcept;
    ~Window() noexcept;

    Window(Window const&) = delete;
    Window& operator=(Window const&) = delete;

    Window(Window&& rhs) noexcept
        : window_(std::exchange(rhs.window_, nullptr)) {}

    Window& operator=(Window&& rhs) noexcept;

    [[nodiscard]] bool shouldClose() const noexcept {
        return glfwWindowShouldClose(window_) > 0;
    }

    [[nodiscard]] GLFWwindow* get() const noexcept { return window_; }

    static std::expected<Window, std::runtime_error>
    create(int width, int height) noexcept;

private:
    GLFWwindow* window_;
};

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

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;
        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 2>
    getAttributeDescriptions() {
        std::array<vk::VertexInputAttributeDescription, 2>
            attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        return attributeDescriptions;
    }
};

std::expected<vk::raii::Buffer, std::runtime_error>
createVertexBuffer(vk::raii::Device const& device,
                   std::size_t bufSize) noexcept;

[[nodiscard]] uint32_t
findMemoryType(uint32_t typeFilter,
               vk::MemoryPropertyFlags properties,
               vk::raii::PhysicalDevice const& physicalDevice);

vk::raii::DeviceMemory
allocateVertexBufferMemory(vk::raii::Device const& device,
                           vk::raii::Buffer const& vbuffer,
                           vk::raii::PhysicalDevice const& physicalDevice);

} // namespace myvk
