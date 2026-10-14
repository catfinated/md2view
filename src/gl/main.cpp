#include "md2view/cli.hpp"
#include "md2view/gl/engine.hpp"
#include "md2view/md2view.hpp"

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <exception>
#include <memory>
#include <span>

int main(int argc, char const* argv[]) {
    try {
        auto const args = parseCli(std::span{argv, static_cast<size_t>(argc)});
        if (!args) {
            return EXIT_FAILURE;
        }
        spdlog::set_level(args->logLevel);

        GL::Engine engine{*args, std::make_unique<MD2View>()};

        engine.runGame();
    } catch (std::exception const& excp) {
        spdlog::error("exception caught in main: {}", excp.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
