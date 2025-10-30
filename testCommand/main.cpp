#include "shellGame.hpp"
#include <iostream>

int main() {
    std::cerr << "main start\n";
    ShellGame game;
    std::cerr << "game constructed\n";
    game.run();
    std::cout << game.outputString.str();
    std::cerr << "game.run returned\n";
    return 0;
}