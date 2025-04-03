#include "game.hpp"

int main() {
    srand(static_cast<unsigned>(time(0)));
    Game game;
    
    if (!game.initialize()) {
        return -1;
    }
    
    game.run();
    game.cleanup();
    return 0;
}