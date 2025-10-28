#include "shellGame.hpp"
#include <iostream>

int main() {
    std::cerr << "main start\n";
    Game game; // ★Gameの実体を作成するために game.hpp が必要
    std::cerr << "game constructed\n";
    game.run();
    std::cout << game.outputString.str();
    std::cerr << "game.run returned\n";
    return 0;
}