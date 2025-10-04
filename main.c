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
#define FPS_TIME 60.0


int main() {
    Console console;

    console_init(&console);

    //時間処理の用意
    LARGE_INTEGER freq, lastTime, currentTime;
    double deltaTime = 0;
    QueryPerformanceFrequency(&freq);//タイマーの周波数
    QueryPerformanceCounter(&lastTime);//基準時間

    //ゴールポータルの設定
    dvec3 portalPos = {1.5, 1.5, 0.1};   // ポータルの中心座標
    dvec3 portalNormal = {1., 0., 0.0}; // ポータルの向き(法線ベクトル)
    vec_normalize3D(&portalNormal);

    //マップ用意
    Map map;
    maze_ganarate(&map, 3, 3);
    //testMaze(&map);  //test用

    //プレイヤー情報の初期化
    Player player;
    player_init(&player, (dvec3){1.5,1.5,0.0}, 0., 0., 2.,0.5);

    //ゲームループ------
    while(1){
        
        {//時間処理
            QueryPerformanceCounter(&currentTime);
            deltaTime = (double)(currentTime.QuadPart - lastTime.QuadPart) / freq.QuadPart;
            if(deltaTime < 1.0/FPS_TIME)
                continue;//FPS保持のため停止
        }
        
        {//ウィンドウサイズ変更時にバッファを再設定して解像度を変更
            if(console_windowSizeIsChanged(&console)){//ウィンドウサイズ変更検知
                //描画バッファの再設定
                console_setScreenBuffer(&console);
            }
        }

        {
            //portalPos.z-=0.01;
        }

        {//プレイヤー入力
            player_handleInput(&player, deltaTime, &map);
        }

        //描画用文字列用意
        {
            for (int y = 0; y < console.windowHeight; y++) {
                for (int x = 0; x < console.windowWidth; x++) {
                    //uv -1~1
                    double uvX = (x*2.-console.windowWidth)/min(console.windowHeight, console.windowWidth);
                    double uvY = (console.windowHeight - y*2.0)/min(console.windowHeight, console.windowWidth);
                    double offSet = 2.0;
                    
                    dvec3 rayDirection = (dvec3){uvX, offSet, uvY};
                    dvec3 rayPosition = (dvec3){player.pos.x,player.pos.y, 0.0};
                    vec_normalize3D(&rayDirection);
                    vec_rotate2double(&rayDirection.y,&rayDirection.z, player.dir.y);
                    vec_rotate2double(&rayDirection.x,&rayDirection.y, player.dir.x);
                    
                    int sideFlag=1, numFlag=1;
                    double wallDist;
                    wallDist = rayCast_map(&map, rayPosition, rayDirection, &sideFlag, &numFlag, 0.4, 0.8);
                        
                    WORD col = design_map(numFlag, sideFlag);

                    WCHAR s = L' ';
                    dvec3 encountPos;
                    double portalDist;
                    portalDist = rayCast_sprite(rayPosition,rayDirection, portalPos, portalNormal, &encountPos);
                    if(0 <= portalDist && portalDist < wallDist){//壁よりポータルが近い
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

        //描画
        console_draw(&console);

        //終了判定
        {
            short returnKeyStatus = GetAsyncKeyState(VK_RETURN);
            if ((returnKeyStatus & 0x8000) != 0)
                break;
        }
        
        lastTime = currentTime;
    }
    //---ゲームループ終了--
    console_finish(&console);

    printf("finish\n");
    return 0;
}