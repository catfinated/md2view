#include "md2view/vk/engine.hpp"

#include <spdlog/spdlog.h>

#include <span>

int main(int argc, char const* argv[]) {
    myvk::VKEngine engine;
    engine.init(std::span{argv, static_cast<size_t>(argc)});
    try {
        engine.run_game();
    } catch (std::exception const& excp) {
        spdlog::error("exception caught in main: {}", excp.what());
    }
    return 0;
}
