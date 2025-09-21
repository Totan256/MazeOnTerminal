#pragma once
#include <windows.h>
#include <stdio.h>

typedef struct{
    HANDLE hOriginalConsole, hGameConsole;

    DWORD dwBytesWritten;//正直良くわからん
    POINT mousePosition;


    int windowWidth, windowHeight;//ウィンドウサイズ
    CHAR_INFO *screenBuffer;//描画用の文字列バッファ

}Console;

void console_getWindowSize(int *windowWidth, int *windowHeight, HANDLE handle ){
    //ウィンドウサイズ取得
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(handle, &csbi)) {
        *windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        *windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
}

void console_setScreenBuffer(Console *c){
    free(c->screenBuffer);
    c->screenBuffer = (CHAR_INFO*)calloc(c->windowWidth * c->windowHeight, sizeof(CHAR_INFO));
}

BOOL console_init(Console *c, double fps){
    //現在の標準出力ハンドル（元のバッファ）
    c->hOriginalConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    //新しいスクリーンバッファ（代替バッファ）を作成
    c->hGameConsole = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE, //読み書き両方
        0,//共有しない(?)
        NULL,
        CONSOLE_TEXTMODE_BUFFER,//テキストモード
        NULL
    );
    if(c->hOriginalConsole == INVALID_HANDLE_VALUE || c->hGameConsole == INVALID_HANDLE_VALUE) {
        printf("コンソールバッファの作成に失敗しました。\n");
        exit(0);
    }
    //代替バッファをアクティブにする（画面切り替え）
    SetConsoleActiveScreenBuffer(c->hGameConsole);

    c->dwBytesWritten=0;

    console_getWindowSize(&c->windowWidth, &c->windowHeight, c->hGameConsole);
    c->screenBuffer = (CHAR_INFO*)calloc(c->windowWidth * c->windowHeight, sizeof(CHAR_INFO));
    WriteConsoleW(c->hGameConsole, L"Game Start!", 11, &c->dwBytesWritten, NULL);
    Sleep(2000);
}

BOOL console_windowSizeIsChanged(Console *c){
    int currentWidth, currentHeight;
    console_getWindowSize(&currentWidth, &currentHeight, c->hGameConsole);
    
    if (c->windowWidth != currentWidth || c->windowHeight != currentHeight) {
        // サイズが違ったら値更新
        c->windowWidth = currentWidth;
        c->windowHeight = currentHeight;
        return TRUE;
    }
    return FALSE;
}


void console_draw(Console *c){
    COORD bufferSize = { (SHORT)c->windowWidth, (SHORT)c->windowHeight };
    const COORD bufferCoord = { 0, 0 };
    SMALL_RECT writeRegion = { 0, 0, (SHORT)(c->windowWidth - 1), (SHORT)(c->windowHeight - 1) };
    WriteConsoleOutputW(
        c->hGameConsole, // 書き込み先ハンドル
        c->screenBuffer,   // 書き込むデータ (CHAR_INFO 配列)
        bufferSize,     // データのサイズ (幅, 高さ)
        bufferCoord,    // データの読み取り開始位置 (0, 0)
        &writeRegion    // コンソールへの書き込み領域
    );
}
void console_finish(Console *c){
    //ゲーム中に溜まったキー入力破棄
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    free(c->screenBuffer);
    //元の標準バッファに戻す
    SetConsoleActiveScreenBuffer(c->hOriginalConsole);

    //代替バッファを閉じる
    CloseHandle(c->hGameConsole);

    system("cls");
}
