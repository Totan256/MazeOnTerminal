#include <windows.h>
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
    //dvec3 portalPos = {5.5, .2, 5.5};   // ポータルの中心座標
    dvec3 portalPos = {1.5, .2, 1.5};
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
        int maxFrame=200;
        for(int frame=0; frame<maxFrame; frame+=4){
            deltaTime = waitFPS(&currentTime, &lastTime, &freq);
            console_checkResizeAndReallocBuffer(&console);
            
            {//描画
                for (SHORT y = 0; y < console.windowHeight; ++y) {
                    for (SHORT x = 0; x < console.windowWidth; ++x) {
                        int current_id = x + y * console.windowWidth;

                        // アニメーション判定（斜めに切り替わる）
                        if (x+y < frame) {
                            // ゲーム画面を描画
                            console.screenBuffer[current_id].Char.UnicodeChar = ((x*(4231-y) + y + x*x) % 8 > 4) ? L'0' : L'1';
                            console.screenBuffer[current_id].Attributes = ((x*y + y*y*x) % 8 > 4) ? FOREGROUND_GREEN : FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                        } else {
                            // 元のコンソール画面を描画
                            // ◆ 座標マッピングの核心部分
                            if (x < console.originalWidth && y < console.originalHeight) {
                                // 範囲内なら、元の幅を使ってインデックスを計算
                                int original_id = x + y * console.originalWidth;
                                console.screenBuffer[current_id] = console.originalScreenBuffer[original_id];
                            } else {
                                // 範囲外（リサイズで広くなった部分）
                                console.screenBuffer[current_id].Char.UnicodeChar = L' ';
                                console.screenBuffer[current_id].Attributes = 0; // 黒
                            }
                        }
                    }
                }
            }

            // ◆ 合成したバッファを一括で画面に転送
            SMALL_RECT writeRegion = {0, 0, console.windowWidth - 1, console.windowHeight - 1};
            WriteConsoleOutputW(console.hGameConsole, console.screenBuffer, (COORD){console.windowWidth, console.windowHeight}, (COORD){0, 0}, &writeRegion);

            
            
            lastTime = currentTime;
        }


    }
    //ゲームループ------
    while(1){
        deltaTime = waitFPS(&currentTime, &lastTime, &freq);
        console_checkResizeAndReallocBuffer(&console);

        {
            //portalPos.z=0.6+portalNormal.x*0.05;
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
            for (int y = 0; y < console.windowHeight; y++) {
                for (int x = 0; x < console.windowWidth; x++) {
                    //uv -1~1
                    double uvX = (x*2.-console.windowWidth)/min(console.windowHeight, console.windowWidth);
                    double uvY = (console.windowHeight - y*2.0)/min(console.windowHeight, console.windowWidth);
                    double offSet = 2.0;
                    
                    dvec3 rayDirection = (dvec3){uvX, uvY, offSet};
                    dvec3 rayPosition = (dvec3){player.pos.x,player.pos.y, player.pos.z};
                    vec_normalize3D(&rayDirection);
                    vec_rotate2double(&rayDirection.y,&rayDirection.z, player.dir.y);//Pitch回転 (X軸を中心にYとZを回転)
                    vec_rotate2double(&rayDirection.x,&rayDirection.z, player.dir.x);//Yaw回転 (Y軸を中心にXとZを回転)
                    
                    
                    RaycastResult mapResult = rayCast_map(&map, rayPosition, rayDirection, 0.4, 0.8);
                    
                    WORD col = design_map(mapResult.objectID, mapResult.hitSurface);

                    WCHAR s = L' ';
                    dvec3 encountPos;
                    double portalDist;
                    {
                        //double temp = rayPosition.y;
                        //rayPosition.y = rayPosition.z;
                        //rayPosition.z = temp;

                    }
                    portalDist = rayCast_sprite(rayPosition,rayDirection, portalPos, portalNormal, &encountPos);
                    if(0 <= portalDist && portalDist < mapResult.distance){//壁よりポータルが近い
                        dvec2 portalUV;
                        //s = '@';//test
                        rayCast_calcUV(encountPos, portalPos, portalNormal, &portalUV);
                        //if(portalUV.y < 0.0){
                        if(design_portal(portalUV) != 0){
                            col = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
                        }
                    }

                    int id = y * console.windowWidth + x;
                    console.screenBuffer[id].Char.UnicodeChar = s;
                    console.screenBuffer[id].Attributes = col;
                }
            }
        }

        {//描画
        console_draw(&console);
        }

        {//ゴールポータル接触判定
            dvec3 d;
            vec_sub3D(portalPos,player.pos,&d);
            //if(vec_length3D(d)<1.){
            //    break;
            //}
        }

        //終了判定
        {
            short returnKeyStatus = GetAsyncKeyState(VK_RETURN);
            if (input->isPressed[ACTION_QUIT_GAME])
                break;
        }
        
        lastTime = currentTime;
    }
    //---ゲームループ終了--
    console_finish(&console);

    printf("finish\n");
    console_waitKeyUP(VK_RETURN);
    printf("player:x=%f, y=%f, z=%f\n", player.pos.x, player.pos.y, player.pos.z);
    printf("portal:x=%f, y=%f, z=%f\n", portalPos.x, portalPos.y, portalPos.z);
    dvec3 d;
    vec_sub3D(portalPos,player.pos,&d);
    printf("len=%f\n",vec_length3D(d));
    return 0;
}