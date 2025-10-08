#include "game.h"

int main() {
    Game game = {0};

    // 監督に初期化を指示
    game_init(&game);

    // 監督にメインループの実行を指示
    while (game.currentState != GAME_STATE_EXIT) {
        game_mainLoop(&game);
    }

    // 監督に終了処理を指示
    game_finish(&game);

    return 0;
}