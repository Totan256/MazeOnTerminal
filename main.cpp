#include "game.hpp"

int main() {
    Game game;
    try{
        game.run();
    }
    catch (const std::exception& e) {
        std::cerr << "例外発生: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "不明な例外が発生しました" << std::endl;
    }

    return 0;
}