#include "Game.h"

#include <iostream>

int main() {
    Game game;
    if (!game.init()) {
        std::cerr << "CASEFIRE failed to initialize.\n";
        return 1;
    }

    game.run();
    return 0;
}
