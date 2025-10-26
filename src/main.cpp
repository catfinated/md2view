#include "glengine.ipp"
#include "md2view.hpp"

int main(int argc, char const* argv[]) {
    GLEngine<MD2View> engine;

    if (!engine.init(std::span{argv, static_cast<size_t>(argc)})) {
        return EXIT_FAILURE;
    }

    engine.run_game();
    return EXIT_SUCCESS;
}
