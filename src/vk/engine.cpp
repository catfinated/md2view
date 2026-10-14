#include "md2view/vk/engine.hpp"

#include "md2view/cli.hpp"
#include "md2view/engine.hpp"
#include "md2view/renderer.hpp"
#include "md2view/vk/renderer.hpp"
#include "md2view/window.hpp"

#include <GLFW/glfw3.h>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <memory>
#include <stdexcept>

namespace VK {

Engine::Engine(CliArgs const& args)
    : ::Engine{args} {
    spdlog::info("GLFW version: {}", glfwGetVersionString());
    spdlog::info("Vulkan supported: {}",
                 glfwVulkanSupported() != 0 ? "yes" : "no");

    window_ = Window::create(width_, height_, "vkmd2v", Renderer::kWindowHints);
    renderer_ = std::make_unique<Renderer>(window_);
    window_.setInputListener(this);
}

::Renderer& Engine::renderer() {
    throw std::logic_error("VK::Renderer does not implement ::Renderer yet");
}

void Engine::runGame() {
    while (!window_.shouldClose()) {
        beginFrame();
        glfwPollEvents();
        renderer_->drawFrame(gsl_lite::narrow_cast<float>(glfwGetTime()));
    }
}

} // namespace VK
