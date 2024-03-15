#include "vkengine.hpp"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace vk {

template <class T, class E>
T forceUnwrap(tl::expected<T, E>&& expectedT)
{
    if (!expectedT) {
        throw expectedT.error();
    }
    return std::move(expectedT).value();
}

VKEngine::VKEngine()
{
    gsl_Ensures(glfwInit() == GLFW_TRUE);
}

VKEngine::~VKEngine()
{
    glfwTerminate();
}

bool VKEngine::init(int argc, char const * argv[])
{
    return parse_args(argc, argv);
}

void VKEngine::init_window()
{
    window_ = forceUnwrap(Window::create(width_, height_));    
}

void VKEngine::init_vulkan()
{
    instance_ = forceUnwrap(Instance::create());
    debugMessenger_ = forceUnwrap(DebugMessenger::create(instance_));
    physicalDevice_ = forceUnwrap(PhysicalDevice::pickPhysicalDevice(instance_));
    device_ = forceUnwrap(Device::create(physicalDevice_));
}

void VKEngine::run_game()
{
    init_window();
    init_vulkan();

    while(!window_.shouldClose()) {
        glfwPollEvents();
    }

    glfwTerminate();
}

}