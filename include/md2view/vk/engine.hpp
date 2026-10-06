/**
 * @brief md2view Vulkan graphics engine
 *
 * This is very much a work in progress.
 */
#pragma once

#include "md2view/engine.hpp"
#include "md2view/vk/renderer.hpp"

namespace VK {

class Engine : public ::Engine {
public:
    Engine() = default;
    ~Engine() = default;

    Engine(Engine const&) = delete;
    Engine& operator=(Engine const&) = delete;
    Engine(Engine&&) noexcept = delete;
    Engine& operator=(Engine&&) noexcept = delete;

    void run_game();

private:
    void doInit() final;

    std::unique_ptr<Renderer> renderer_;
};

} // namespace VK
