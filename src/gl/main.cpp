#include "md2view/gl/engine.hpp"
#include "md2view/gl/md2view.hpp"

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <exception>
#include <memory>
#include <span>

int main(int argc, char const* argv[]) {
    try {
        GL::Engine engine{std::make_unique<MD2View>()};

        if (!engine.init(std::span{argv, static_cast<size_t>(argc)})) {
            return EXIT_FAILURE;
        }

        engine.runGame();
    } catch (std::exception const& excp) {
        spdlog::error("exception caught in main: {}", excp.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
