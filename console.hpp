#pragma once
#include <windows.h>
#include <stdio.h>

#include <vector> // 可変長配列。CHAR_INFOの管理に便利

// ScreenBufferはConsoleクラスで使う部品なので、このファイルに一緒に定義すると便利
struct ScreenBuffer {
    std::vector<CHAR_INFO> buffer;
    int width = 0;
    int height = 0;

    void reallocate(int newWidth, int newHeight) {
        width = newWidth;
        height = newHeight;
        buffer.assign(width * height, {}); // 指定サイズでバッファを確保し、ゼロで初期化
    }
};

class Console {
public:
    // コンストラクタで console_init の処理を行う
    Console(){
        //console_waitKeyUP(ACTION_QUIT_GAME);
        SetConsoleOutputCP(CP_UTF8);
        hOriginalConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        
        GetConsoleMode(hOriginalConsole, &originalMode);
        SetConsoleMode(hOriginalConsole, originalMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

        // 1. 元の画面をキャプチャ
        CONSOLE_SCREEN_BUFFER_INFO originalCsbi;
        GetConsoleScreenBufferInfo(hOriginalConsole, &originalCsbi);
        originalScreen.width = originalCsbi.srWindow.Right - originalCsbi.srWindow.Left + 1;
        originalScreen.height = originalCsbi.srWindow.Bottom - originalCsbi.srWindow.Top + 1;
        SMALL_RECT readRegion = originalCsbi.srWindow;
        
        //originalScreen.buffer = (CHAR_INFO*)calloc(originalScreen.width * originalScreen.height, sizeof(CHAR_INFO));
        originalScreen.reallocate(originalScreen.width, originalScreen.height);
        COORD bufferSize = { (SHORT)originalScreen.width, (SHORT)originalScreen.height };
        const COORD bufferCoord = { 0, 0 };
        ReadConsoleOutputW(hOriginalConsole, originalScreen.buffer.data(), bufferSize, bufferCoord, &readRegion);

        // 2. 代替バッファへ移行
        printf("\x1b[?1049h\x1b[?25l");//VT100シーケンス
        hGameConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        getWindowSize(gameScreen.width, gameScreen.height, hGameConsole);
        gameScreen.reallocate(gameScreen.width, gameScreen.height);
    }

    // デストラクタで console_finish の処理を行う
    ~Console(){
        
    }

    // リソースを管理するクラスはコピーできないようにするのが定石
    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    void checkResizeAndReallocBuffer() {
        int currentWidth, currentHeight;
        getWindowSize(currentWidth, currentHeight, hGameConsole);
        
        // 自分自身のメンバであるgameScreenを直接チェック・操作する
        if (gameScreen.width != currentWidth || gameScreen.height != currentHeight) {
            gameScreen.reallocate(currentWidth, currentHeight);
        }
    }
    void draw(const ScreenBuffer& screenToDraw){
        COORD bufferSize = { (SHORT)screenToDraw.width, (SHORT)screenToDraw.height };
        const COORD bufferCoord = { 0, 0 };
        SMALL_RECT writeRegion = { 0, 0, (SHORT)(screenToDraw.width - 1), (SHORT)(screenToDraw.height - 1) };
        WriteConsoleOutputW(
            hGameConsole, // 書き込み先ハンドル
            screenToDraw.buffer.data(),   // 書き込むデータ (CHAR_INFO 配列)
            bufferSize,     // データのサイズ (幅, 高さ)
            bufferCoord,    // データの読み取り開始位置 (0, 0)
            &writeRegion    // コンソールへの書き込み領域
        );
    }
    
    ScreenBuffer& getGameScreenBuffer() { return gameScreen; }
    const ScreenBuffer& getOriginalScreen() const { return originalScreen; }
    const HANDLE getGameHandle() { return hGameConsole;}

    void restore(){
        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
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
        SetConsoleMode(hOriginalConsole, originalMode);
    }

private:
    void getWindowSize(int& w, int& h, HANDLE handle){
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(handle, &csbi)) {
            w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        }
    }

    HANDLE hOriginalConsole;
    HANDLE hGameConsole;
    DWORD originalMode;

    ScreenBuffer originalScreen;
    ScreenBuffer gameScreen;
};

