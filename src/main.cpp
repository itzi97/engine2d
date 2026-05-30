#include "core/Game.hpp"
#include <cstdlib>
#include <iostream>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    Game game;

    if (!game.Initialize()) {
        std::cerr << "[main] Game::Initialize() failed. Exiting.\n";
        return EXIT_FAILURE;
    }

    game.Run();
    game.Shutdown();

    return EXIT_SUCCESS;
}
