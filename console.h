#pragma once
#include <windows.h>
#include <stdio.h>

typedef struct {
    CHAR_INFO* buffer;
    int width;
    int height;
} ScreenBuffer;
typedef struct{
    HANDLE hOriginalConsole, hGameConsole;
    DWORD originalMode;//元のコンソールの設定を記録

    DWORD dwBytesWritten;//正直良くわからん
    POINT mousePosition;


    // int windowWidth, windowHeight;//ウィンドウサイズ
    // CHAR_INFO *screenBuffer;//描画用の文字列バッファ
    // int originalWidth, originalHeight;//履歴ウィンドウサイズ
    // CHAR_INFO *originalScreenBuffer;//履歴文字列バッファ
    ScreenBuffer originalScreen;
    ScreenBuffer gameScreen;

}Console;


void console_getWindowSize(int *windowWidth, int *windowHeight, HANDLE handle ){
    //ウィンドウサイズ取得
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(handle, &csbi)) {
        *windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        *windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
}

void console_setScreenBuffer(ScreenBuffer *sb){
    if(sb->buffer){
        free(sb->buffer);
    }
    sb->buffer = (CHAR_INFO*)calloc(sb->width * sb->height, sizeof(CHAR_INFO));
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
    c->originalScreen.width = originalCsbi.srWindow.Right - originalCsbi.srWindow.Left + 1;
    c->originalScreen.height = originalCsbi.srWindow.Bottom - originalCsbi.srWindow.Top + 1;
    SMALL_RECT readRegion = originalCsbi.srWindow;
    
    c->originalScreen.buffer = (CHAR_INFO*)calloc(c->originalScreen.width * c->originalScreen.height, sizeof(CHAR_INFO));
    ReadConsoleOutputW(c->hOriginalConsole, c->originalScreen.buffer, (COORD){c->originalScreen.width, c->originalScreen.height}, (COORD){0, 0}, &readRegion);

    // 2. 代替バッファへ移行
    printf("\x1b[?1049h\x1b[?25l");//VT100シーケンス
    c->hGameConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    console_getWindowSize(&c->gameScreen.width, &c->gameScreen.height, c->hGameConsole);
    c->gameScreen.buffer = (CHAR_INFO*)calloc(c->gameScreen.width * c->gameScreen.height, sizeof(CHAR_INFO));
}

BOOL console_windowSizeIsChanged(ScreenBuffer *sb, HANDLE handle){
    int currentWidth, currentHeight;
    console_getWindowSize(&currentWidth, &currentHeight, handle);
    
    if (sb->width != currentWidth || sb->height != currentHeight) {
        // サイズが違ったら値更新
        sb->width = currentWidth;
        sb->height = currentHeight;
        return TRUE;
    }
    return FALSE;
}

void console_checkResizeAndReallocBuffer(ScreenBuffer *sb, HANDLE handle){
    if(console_windowSizeIsChanged(sb, handle)){//ウィンドウサイズ変更検知
        //描画バッファの再設定
        console_setScreenBuffer(sb);
    }
}

void console_draw(Console *c){
    COORD bufferSize = { (SHORT)c->gameScreen.width, (SHORT)c->gameScreen.height };
    const COORD bufferCoord = { 0, 0 };
    SMALL_RECT writeRegion = { 0, 0, (SHORT)(c->gameScreen.width - 1), (SHORT)(c->gameScreen.height - 1) };
    WriteConsoleOutputW(
        c->hGameConsole, // 書き込み先ハンドル
        c->gameScreen.buffer,   // 書き込むデータ (CHAR_INFO 配列)
        bufferSize,     // データのサイズ (幅, 高さ)
        bufferCoord,    // データの読み取り開始位置 (0, 0)
        &writeRegion    // コンソールへの書き込み領域
    );
}
void console_finish(Console *c){
    //ゲーム中に溜まったキー入力破棄
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    free(c->originalScreen.buffer);
    free(c->gameScreen.buffer);
    // 元のバッファに戻す
    printf("\x1b[?1049l");

    // 【対策】復元後、カーソル位置を整える
    // 現在のウィンドウの高さを取得
    //CONSOLE_SCREEN_BUFFER_INFO csbi;
    //GetConsoleScreenBufferInfo(c->hOriginalConsole, &csbi);
    //SHORT finalHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    
    // カーソルをウィンドウの左下に移動させてから改行し、プロンプトがきれいな行から始まるようにする
    //printf("\x1b[%d;1H\n", finalHeight);

    // コンソールモードも元に戻す
    SetConsoleMode(c->hOriginalConsole, c->originalMode); 
}
