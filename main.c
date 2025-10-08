//#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "maze.h"
#include "vec.h"
#include "player.h"
#include "rayCast.h"
#include "console.h"
#include "design.h"
#include "input.h"
#include "render.h"
#define FPS_TIME 60.0

double waitFPS(LARGE_INTEGER *currentTime, LARGE_INTEGER *lastTime, LARGE_INTEGER *freq){//時間処理
    double deltaTime = 0.0;
    do{
        QueryPerformanceCounter(currentTime);
        deltaTime = (double)(currentTime->QuadPart - lastTime->QuadPart) / freq->QuadPart;
    }while(deltaTime < 1.0/FPS_TIME);
    return deltaTime;    
}



int main() {
    Console console;

    console_init(&console);

    //時間処理の用意
    LARGE_INTEGER freq, lastTime, currentTime;
    double deltaTime = 0;
    QueryPerformanceFrequency(&freq);//タイマーの周波数
    QueryPerformanceCounter(&lastTime);//基準時間

    //ゴールポータルの設定
    dvec3 portalPos = {5.5, .2, 5.5};   // ポータルの中心座標
    //dvec3 portalPos = {1.5, .2, 1.5};
    dvec3 portalNormal = {1., 0., 0.0}; // ポータルの向き(法線ベクトル)
    vec_normalize3D(&portalNormal);

    //マップ用意
    Map map;
    maze_generate(&map, 3, 3);
    //testMaze(&map);  //test用

    //プレイヤー情報の初期化
    Player player;
    player_init(&player, (dvec3){1.5,0.0,1.5}, -3.14*0.25, 3.14*0., 2.,0.9);
    {//スタートアニメーション
        ScreenBuffer firstGameScreen={0};
        firstGameScreen.width = console.gameScreen.width;
        firstGameScreen.height = console.gameScreen.height;
        console_setScreenBuffer(&firstGameScreen);
        render_setBuffer(&player, &map, &firstGameScreen, portalPos, portalNormal);
        int maxFrame=200;
        for(int frame=0; frame<maxFrame; frame+=4){
            deltaTime = waitFPS(&currentTime, &lastTime, &freq);
            console_checkResizeAndReallocBuffer(&console.gameScreen, console.hGameConsole);
            //バッファ
            render_transAnimation(&console.gameScreen, &console.originalScreen, &firstGameScreen, frame);
            //出力
            console_draw(&console);
            lastTime = currentTime;
        }
        free(firstGameScreen.buffer);
    }

    input_resetState();

    //ゲームループ------
    while(1){
        deltaTime = waitFPS(&currentTime, &lastTime, &freq);
        console_checkResizeAndReallocBuffer(&console.gameScreen, console.hGameConsole);

        {
            portalPos.y=0.2+portalNormal.x*0.05;
            vec_rotate2double(&portalNormal.x, &portalNormal.z, deltaTime);//y軸中心回転
        }

        //入力状態
        input_update();
        const InputState *input = input_getState();

        {//プレイヤー操作
            player_handleInput(&player, input, deltaTime, &map);
        }

        //描画用文字列用意
        {
            render_setBuffer(&player, &map, &console.gameScreen, portalPos, portalNormal);
        }

        {//描画
            console_draw(&console);
        }

        {//ゴールポータル接触判定
            dvec3 relativeCoord;
            vec_sub3D(portalPos,player.pos,&relativeCoord);
            if(vec_length3D(relativeCoord)<0.3){
               break;
            }
        }

        //終了判定
        {
            if (input->isPressed[ACTION_QUIT_GAME])
                break;
        }
        
        lastTime = currentTime;
    }
    //---ゲームループ終了--
    {//エンドアニメーション
        ScreenBuffer lastGameScreen = {0};
        lastGameScreen.width = console.gameScreen.width;
        lastGameScreen.height = console.gameScreen.height;
        console_setScreenBuffer(&lastGameScreen);
        render_setBuffer(&player, &map, &lastGameScreen, portalPos, portalNormal);
        int maxFrame=200;
        for(int frame=0; frame<maxFrame; frame+=4){
            deltaTime = waitFPS(&currentTime, &lastTime, &freq);
            console_checkResizeAndReallocBuffer(&console.gameScreen, console.hGameConsole);
            //バッファ
            render_transAnimation(&console.gameScreen, &lastGameScreen, &console.originalScreen, frame);
            //出力
            console_draw(&console);
            lastTime = currentTime;
        }
        free(lastGameScreen.buffer);
    }

    //終了処理
    console_finish(&console);

    printf("GAME CLEAR\n");
    printf("total walk diatance : %f\n", player.totalWallDist);
    maze_printBinaryMap(&map);
    console_waitKeyUP(ACTION_QUIT_GAME);
    return 0;
}