#pragma once
#include <windows.h>
#include <stdio.h>

typedef struct{
    HANDLE hOriginalConsole, hGameConsole;
    DWORD originalMode;//元のコンソールの設定を記録

    DWORD dwBytesWritten;//正直良くわからん
    POINT mousePosition;


    int windowWidth, windowHeight;//ウィンドウサイズ
    CHAR_INFO *screenBuffer;//描画用の文字列バッファ
    int originalWidth, originalHeight;//履歴ウィンドウサイズ
    CHAR_INFO *originalScreenBuffer;//履歴文字列バッファ

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

void console_waitKeyUP(int vKey){
    while((GetAsyncKeyState(vKey) & 0x8000) != 0){}
}

BOOL console_init(Console *c){
    console_waitKeyUP(VK_RETURN);
    SetConsoleOutputCP(CP_UTF8);
    c->hOriginalConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    GetConsoleMode(c->hOriginalConsole, &c->originalMode);
    SetConsoleMode(c->hOriginalConsole, c->originalMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    // 1. 元の画面をキャプチャ
    CONSOLE_SCREEN_BUFFER_INFO originalCsbi;
    GetConsoleScreenBufferInfo(c->hOriginalConsole, &originalCsbi);
    c->originalWidth = originalCsbi.srWindow.Right - originalCsbi.srWindow.Left + 1;
    c->originalHeight = originalCsbi.srWindow.Bottom - originalCsbi.srWindow.Top + 1;
    SMALL_RECT readRegion = originalCsbi.srWindow;
    
    c->originalScreenBuffer = (CHAR_INFO*)calloc(c->originalWidth * c->originalHeight, sizeof(CHAR_INFO));
    ReadConsoleOutputW(c->hOriginalConsole, c->originalScreenBuffer, (COORD){c->originalWidth, c->originalHeight}, (COORD){0, 0}, &readRegion);

    // 2. 代替バッファへ移行
    printf("\x1b[?1049h\x1b[?25l");//VT100シーケンス
    c->hGameConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    console_getWindowSize(&c->windowWidth, &c->windowHeight, c->hGameConsole);
    c->screenBuffer = (CHAR_INFO*)calloc(c->windowWidth * c->windowHeight, sizeof(CHAR_INFO));
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

void console_checkResizeAndReallocBuffer(Console *c){
    if(console_windowSizeIsChanged(c)){//ウィンドウサイズ変更検知
        //描画バッファの再設定
        console_setScreenBuffer(c);
    }
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
    free(c->originalScreenBuffer);
    free(c->screenBuffer);
    // 元のバッファに戻す
    printf("\x1b[?1049l");

    // 【対策】復元後、カーソル位置を整える
    // 現在のウィンドウの高さを取得
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(c->hOriginalConsole, &csbi);
    SHORT finalHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    
    // カーソルをウィンドウの左下に移動させてから改行し、プロンプトがきれいな行から始まるようにする
    printf("\x1b[%d;1H\n", finalHeight);

    // コンソールモードも元に戻す
    SetConsoleMode(c->hOriginalConsole, c->originalMode); 
}
