#include "md2view/vk/engine.hpp"

#include "md2view/window.hpp"

#include <GLFW/glfw3.h>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <memory>

namespace VK {

void Engine::doInit() {
    spdlog::info("GLFW version: {}", glfwGetVersionString());
    spdlog::info("Vulkan supported: {}",
                 glfwVulkanSupported() != 0 ? "yes" : "no");

    window_ = Window::create(width_, height_, "vkmd2v", Renderer::kWindowHints);
    renderer_ = std::make_unique<Renderer>(window_);
}

void Engine::run_game() {
    while (!window_.shouldClose()) {
        beginFrame();
        glfwPollEvents();
        renderer_->drawFrame(gsl_lite::narrow_cast<float>(glfwGetTime()));
    }
}

} // namespace VK
