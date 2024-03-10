#include "glengine.ipp"
#include "md2view.hpp"

int main(int argc, char const * argv[])
{
   GLEngine<MD2View> engine;

    if (!engine.init(argc, argv)) {
        return EXIT_FAILURE;
    }

    engine.run_game();
    return EXIT_SUCCESS;
}
