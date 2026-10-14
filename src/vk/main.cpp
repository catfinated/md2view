#include "md2view/cli.hpp"
#include "md2view/vk/engine.hpp"

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include <cstdlib>
#include <exception>
#include <span>

int main(int argc, char const* argv[]) {
    try {
        auto const args = parseCli(std::span{argv, static_cast<size_t>(argc)});
        if (!args) {
            return EXIT_FAILURE;
        }
        spdlog::set_level(args->logLevel);

        VK::Engine engine{*args};

        engine.runGame();
    } catch (vk::SystemError const& excp) {
        // vk::SystemError carries the VkResult as an error code; log it
        // alongside the message rather than flattening it to a string
        spdlog::error("vulkan error: {} ({})", excp.what(),
                      excp.code().message());
        return EXIT_FAILURE;
    } catch (std::exception const& excp) {
        spdlog::error("exception caught in main: {}", excp.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
