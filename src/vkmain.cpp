#include "vkengine.hpp"

#include <spdlog/spdlog.h>

int main(int argc, char const * argv[])
{
    myvk::VKEngine engine;
    engine.init(argc, argv);
    try {
        engine.run_game();
    } catch (std::exception const& excp) {
        spdlog::error("{}", excp.what());
    }
    return 0;
}
