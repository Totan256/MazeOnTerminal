#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "maze.h"
#include "vec.h"
#include "player.h"
#include "rayCast.h"
#define FPS_TIME 60.0

void getWindowSize(int *windowWidth, int *windowHeight, HANDLE handle ){
    //ウィンドウサイズ取得
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(handle, &csbi)) {
        *windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        *windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
}




int main() {
    HANDLE hOriginalConsole, hGameConsole;
    

    {//画面切り替え処理
        //現在の標準出力ハンドル（元のバッファ）
        hOriginalConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        
        //新しいスクリーンバッファ（代替バッファ）を作成
        hGameConsole = CreateConsoleScreenBuffer(
            GENERIC_READ | GENERIC_WRITE, //読み書き両方
            0,//共有しない(?)
            NULL,
            CONSOLE_TEXTMODE_BUFFER,//テキストモード
            NULL
        );
        if(hOriginalConsole == INVALID_HANDLE_VALUE || hGameConsole == INVALID_HANDLE_VALUE) {
            printf("コンソールバッファの作成に失敗しました。\n");
            return 1;
        }
        //代替バッファをアクティブにする（画面切り替え）
        SetConsoleActiveScreenBuffer(hGameConsole);
    }

    //時間処理の用意
    LARGE_INTEGER freq, lastTime, currentTime;
    double deltaTime = 0;
    QueryPerformanceFrequency(&freq);//タイマーの周波数
    QueryPerformanceCounter(&lastTime);//基準時間

    //入出力用変数
    DWORD dwBytesWritten = 0;//正直良くわからん
    POINT mousePosition;

    //マップ用意
    int map[24][24];
    testMaze(map);
    double floorHeight, ceilingHeight;//to do

    //ゲーム開始
    WriteConsoleW(hGameConsole, L"Game Start!", 11, &dwBytesWritten, NULL);
    Sleep(2000);

    //プレイヤー情報の初期化
    Player player;
    initPlayer(&player, 12., 12., 0., 0., 2.,0.5);

    //ゲームループ------
    int windowWidth, windowHeight;//ウィンドウサイズ
    CHAR_INFO *screenBuffer;//描画用の文字列バッファ
    getWindowSize(&windowWidth, &windowHeight, hGameConsole);
    screenBuffer = (CHAR_INFO*)calloc(windowWidth * windowHeight, sizeof(CHAR_INFO));
    while(1){
        
        {//時間処理
            QueryPerformanceCounter(&currentTime);
            deltaTime = (double)(currentTime.QuadPart - lastTime.QuadPart) / freq.QuadPart;
            if(deltaTime < 1.0/FPS_TIME)
                continue;//FPS保持のため停止
        }
        
        {//ウィンドウサイズ変更時にバッファを再設定して解像度を変更
            int prevWidth, prevHeight;
            getWindowSize(&prevWidth, &prevHeight, hGameConsole);
            if(windowWidth!=prevWidth || windowHeight!=prevHeight){//ウィンドウサイズ変更検知
                //描画バッファの再設定
                free(screenBuffer);
                screenBuffer = (CHAR_INFO*)calloc(prevWidth * prevHeight, sizeof(CHAR_INFO));
                windowWidth = prevWidth;
                windowHeight = prevHeight;
            }
        }

        {//プレイヤー入力
            GetCursorPos(&mousePosition);
            handleInputPlayer(&player, deltaTime, map);
        }

        //描画用文字列用意
        {
            WCHAR s = L' ';
            for (int y = 0; y < windowHeight; y++) {
                for (int x = 0; x < windowWidth; x++) {
                    //uv -1~1
                    double uvX = (x*2.-windowWidth)/min(windowHeight, windowWidth);
                    double uvY = (y*2.-windowHeight)/min(windowHeight, windowWidth);
                    double offSet = 5.0;
                    double dirX, dirY, dirZ;
                    {//normalize
                        double length = sqrt(uvX*uvX+uvY*uvY +offSet*offSet);
                        dirX = uvX/length;
                        dirY = offSet/length;
                        dirZ = uvY/length;
                    }
                    rotate2double(&dirY,&dirZ, player.dir.y);
                    rotate2double(&dirX,&dirY, player.dir.x);
                    int sideFlag=1, numFlag=1;
                    rayCast(map, 24, 24, player.pos.x,player.pos.y, 10.0, 
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
                    int id = y * windowWidth + x;
                    screenBuffer[id].Char.UnicodeChar = s;
                    screenBuffer[id].Attributes = col;
                }
            }
        }

        //描画
        {
            COORD bufferSize = { (SHORT)windowWidth, (SHORT)windowHeight };
            COORD bufferCoord = { 0, 0 };
            SMALL_RECT writeRegion = { 0, 0, (SHORT)(windowWidth - 1), (SHORT)(windowHeight - 1) };
            WriteConsoleOutputW(
                hGameConsole, // 書き込み先ハンドル
                screenBuffer,   // 書き込むデータ (CHAR_INFO 配列)
                bufferSize,     // データのサイズ (幅, 高さ)
                bufferCoord,    // データの読み取り開始位置 (0, 0)
                &writeRegion    // コンソールへの書き込み領域
            );
        }

        //終了判定
        {
            short returnKeyStatus = GetAsyncKeyState(VK_RETURN);
            if ((returnKeyStatus & 0x8000) != 0)
                break;
        }
        
        lastTime = currentTime;
    }
    //---ゲームループ終了--
    //ゲーム中に溜まったキー入力破棄
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    free(screenBuffer);
    //元の標準バッファに戻す
    SetConsoleActiveScreenBuffer(hOriginalConsole);

    //代替バッファを閉じる
    CloseHandle(hGameConsole);

    system("cls");

    printf("finish\n");
    return 0;
}