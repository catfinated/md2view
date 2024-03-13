#pragma once

#include "engine.hpp"

#include <GLFW/glfw3.h>

class VKEngine : public Engine
{
public:
    bool init(int argc, char const * argv[]);
    void run_game();

private:
    void init_window();
    void init_vulkan();
    void create_instance();
    void create_debug_messenger();

    GLFWwindow * window_;
    VkInstance vkInstance_;
    VkDebugUtilsMessengerEXT debug_messenger_;
};