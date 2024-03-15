#pragma once

#include "engine.hpp"

#include "vk.hpp"

#include <optional>

namespace vk {

class VKEngine : public Engine
{
public:
    VKEngine();
    ~VKEngine();

    bool init(int argc, char const * argv[]);
    void run_game();

private:
    void init_window();
    void init_vulkan();

    Window window_;
    Instance instance_;
    std::optional<DebugMessenger> debugMessenger_;
    std::optional<Surface> surface_;
    PhysicalDevice physicalDevice_;
    Device device_;
};

}