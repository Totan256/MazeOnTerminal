#pragma once
#include "vec.h"
#include "input.h"
#include "console.h"
#include "player.h"
#include "time.h"
#include "maze.h"
#include "render.h"

typedef enum {
    GAME_STATE_START_ANIM, // スタートアニメーション中
    GAME_STATE_PLAYING,    // ゲームプレイ中
    GAME_STATE_END_ANIM,   // 終了アニメーション中
    GAME_STATE_EXIT        // 終了
} GameState;

typedef struct {
    // システム関連
    Console console;
    LARGE_INTEGER freq, lastTime, currentTime;
    double deltaTime;
    double FPS;

    // ゲームオブジェクト
    Player player;
    Map map;
    dvec3 portalPos;
    dvec3 portalNormal;

    // ゲームの状態
    GameState currentState;
    double animationFrame; // アニメーションのフレーム管理用
    double animationSpeed;
}Game;


double waitFPS(LARGE_INTEGER *currentTime, LARGE_INTEGER *lastTime, LARGE_INTEGER *freq, double fps){//時間処理
    double deltaTime = 0.0;
    do{
        QueryPerformanceCounter(currentTime);
        deltaTime = (double)(currentTime->QuadPart - lastTime->QuadPart) / freq->QuadPart;
    }while(deltaTime < 1.0/fps);
    return deltaTime;    
}

void game_init(Game *game){
    //初期数値
    const double defaultFPS = 60.0;
    const int mapSizeX = 3;
    const int mapSizeY = 3;
    const dvec3 portalStartNormal = (dvec3){1., 0., 0.0};
    const dvec3 portaldefaultPos = (dvec3){5.5,0.2,5.5};
    const dvec3 playerStartPos = (dvec3){1.5,0.0,1.5};
    const double playerStartDirX = -3.14*0.25;
    const double playerStartDirY = 3.14*0.0;
    const double playerMoveSpeed = 2.;
    const double playerRotSpeed = 0.9;
    const double animationDefaultSpeed = 4.0/200.;

    console_init(&game->console);

    //時間処理の用意
    game->FPS = defaultFPS;
    game->deltaTime = 0;
    QueryPerformanceFrequency(&game->freq);//タイマーの周波数
    QueryPerformanceCounter(&game->lastTime);//基準時間

    //ゴールポータルの設定
    game->portalPos = portaldefaultPos;   // ポータルの中心座標
    game->portalNormal = portalStartNormal; // ポータルの向き(法線ベクトル)
    vec_normalize3D(&game->portalNormal);

    //マップ用意
    maze_generate(&game->map, mapSizeX, mapSizeY);
    //testMaze(&map);  //test用

    //プレイヤー情報の初期化
    player_init(&game->player, playerStartPos, -3.14*0.25, 3.14*0., 2.,0.9);
    
    //アニメーション設定
    game->animationSpeed = animationDefaultSpeed;
    
    game->currentState = GAME_STATE_START_ANIM;
    game->animationFrame = 0;

}

void game_mainLoop(Game* game) {
    // --- 共通処理 ---
    game->deltaTime = waitFPS(&game->currentTime, &game->lastTime, &game->freq, game->FPS);
    console_checkResizeAndReallocBuffer(&game->console.gameScreen, game->console.hGameConsole);
    input_update();
    const InputState* input = input_getState();

    // --- 現在のシーンに応じた処理 ---
    switch (game->currentState) {
        case GAME_STATE_START_ANIM: {
            ScreenBuffer firstGameScreen={0};
            firstGameScreen.width = game->console.gameScreen.width;
            firstGameScreen.height = game->console.gameScreen.height;
            console_setScreenBuffer(&firstGameScreen);
            render_setBuffer(&game->player, &game->map, &firstGameScreen, game->portalPos, game->portalNormal);
        
            render_transAnimation(&game->console.gameScreen, &game->console.originalScreen, &firstGameScreen, game->animationFrame);
            game->animationFrame += game->animationSpeed;
            if (game->animationFrame >= 1.0) {
                game->currentState = GAME_STATE_PLAYING; // 次のシーンへ
            }
            break;
        }

        case GAME_STATE_PLAYING: {
            {//オブジェクト動作
                game->portalPos.y=0.2+game->portalNormal.x*0.05;
                vec_rotate2double(&game->portalNormal.x, &game->portalNormal.z, game->deltaTime);//y軸中心回転
            }
            //プレイヤー操作
            player_handleInput(&game->player, input, game->deltaTime, &game->map);
            render_setBuffer(&game->player, &game->map, &game->console.gameScreen, game->portalPos, game->portalNormal);

            //ゴールポータル接触判定
            dvec3 relativeCoord;
            vec_sub3D(game->portalPos, game->player.pos,&relativeCoord);
            if(vec_length3D(relativeCoord)<0.3){
                game->currentState = GAME_STATE_END_ANIM; // 次のシーンへ
                game->animationFrame = 0; // アニメーションフレームをリセット
                break;
            }

            //強制終了判定
            if (input->isPressed[ACTION_QUIT_GAME])
                game->currentState = GAME_STATE_EXIT;
            break;
        }

        case GAME_STATE_END_ANIM: {
            ScreenBuffer lastGameScreen = {0};
            lastGameScreen.width = game->console.gameScreen.width;
            lastGameScreen.height = game->console.gameScreen.height;
            console_setScreenBuffer(&lastGameScreen);
            render_setBuffer(&game->player, &game->map, &lastGameScreen, 
                game->portalPos, game->portalNormal);
        
            render_transAnimation(&game->console.gameScreen, &lastGameScreen, &game->console.originalScreen, game->animationFrame);
            game->animationFrame += game->animationSpeed;
            if (game->animationFrame >= 1.0) {
                game->currentState = GAME_STATE_EXIT; // 終了へ
            }
            break;
        }

        case GAME_STATE_EXIT:
            // 何もしない
            break;
    }

    // --- 共通の描画と時間更新 ---
    console_draw(&game->console);
    game->lastTime = game->currentTime;
}

void game_finish(Game *game){
    console_finish(&game->console);

    printf("GAME CLEAR\n");
    printf("total walk diatance : %f\n", game->player.totalWallDist);
    maze_printBinaryMap(&game->map);
    console_waitKeyUP(ACTION_QUIT_GAME);
}