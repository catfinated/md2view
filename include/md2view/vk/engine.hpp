/**
 * @brief md2view Vulkan graphics engine
 *
 * This is very much a work in progress.
 */
#pragma once

#include "md2view/engine.hpp"
#include "md2view/vk/renderer.hpp"

#include <memory>

namespace VK {

class Engine : public ::Engine {
public:
    Engine() = default;
    ~Engine() override = default;

    Engine(Engine const&) = delete;
    Engine& operator=(Engine const&) = delete;
    Engine(Engine&&) noexcept = delete;
    Engine& operator=(Engine&&) noexcept = delete;

    void runGame();

private:
    void doInit() final;

    std::unique_ptr<Renderer> renderer_;
};

} // namespace VK
