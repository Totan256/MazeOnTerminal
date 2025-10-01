#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "maze.h"
#include "vec.h"
#include "player.h"
#include "rayCast.h"
#include "console.h"
#define FPS_TIME 60.0


int main() {
    Console console;

    console_init(&console);

    //時間処理の用意
    LARGE_INTEGER freq, lastTime, currentTime;
    double deltaTime = 0;
    QueryPerformanceFrequency(&freq);//タイマーの周波数
    QueryPerformanceCounter(&lastTime);//基準時間

    //マップ用意
    Map map;
    maze_ganarate(&map, 3, 3);
    //testMaze(&map);  //test用

    //プレイヤー情報の初期化
    Player player;
    player_init(&player, 2., 2., 0., 0., 2.,0.5);

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

        {//プレイヤー入力
            player_handleInput(&player, deltaTime, &map);
        }

        //描画用文字列用意
        {
            WCHAR s = L' ';
            for (int y = 0; y < console.windowHeight; y++) {
                for (int x = 0; x < console.windowWidth; x++) {
                    //uv -1~1
                    double uvX = (x*2.-console.windowWidth)/min(console.windowHeight, console.windowWidth);
                    double uvY = (y*2.-console.windowHeight)/min(console.windowHeight, console.windowWidth);
                    double offSet = 2.0;
                    double dirX, dirY, dirZ;
                    {//normalize
                        double length = sqrt(uvX*uvX+uvY*uvY +offSet*offSet);
                        dirX = uvX/length;
                        dirY = offSet/length;
                        dirZ = uvY/length;
                    }
                    vec_rotate2double(&dirY,&dirZ, player.dir.y);
                    vec_rotate2double(&dirX,&dirY, player.dir.x);
                    int sideFlag=1, numFlag=1;
                    rayCast(&map, player.pos.x,player.pos.y, 10.0, 
                        dirX, dirY, dirZ, &sideFlag, &numFlag, 0.8, 0.4);
                    WORD col;
                    {
                        switch (numFlag) {
                            case -1:
                                col = BACKGROUND_RED | BACKGROUND_BLUE;
                                break;
                            case 1:
                            case 2:
                            case 3:
                            case 4:
                            case 5:
                                if(sideFlag==1)
                                    col = col = BACKGROUND_BLUE | BACKGROUND_INTENSITY;
                                else
                                    col = col = BACKGROUND_BLUE;
                                break;

                            default:
                                //デバッグ用
                                col = BACKGROUND_RED;
                                break;
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