#include "vkengine.hpp"

int main(int argc, char const * argv[])
{
    VKEngine engine;
    engine.init(argc, argv);
    engine.run_game();
    return 0;
}