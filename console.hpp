#pragma once
#include <windows.h>
#include <stdio.h>

#include <vector> // 可変長配列。CHAR_INFOの管理に便利

namespace col{
    enum HUE : unsigned short {
        BLACK   = 0,
        BLUE    = 1,
        GREEN   = 2,
        CYAN    = 3, // BLUE | GREEN
        RED     = 4,
        MAGENTA = 5, // RED | BLUE
        YELLOW  = 6, // RED | GREEN
        WHITE   = 7  // RED | GREEN | BLUE
    };
    struct COL_INF{
        HUE hue = WHITE;
        bool isIntensity = false;
        COL_INF():hue(WHITE), isIntensity(false){};
        COL_INF(HUE n, bool i):hue(n),isIntensity(i){};
    };
    struct CHAR_INF{
        wchar_t charactor;
        COL_INF fore;
        COL_INF back;
        CHAR_INF() 
            : charactor(L' '), fore(WHITE, false), back(BLACK, false) {}
        CHAR_INF(wchar_t c, COL_INF f, COL_INF b)
            : charactor(c), fore(f), back(b) {}
    };
}

// ScreenBufferはConsoleクラスで使う部品なので、このファイルに一緒に定義すると便利
struct ScreenBuffer {
    std::vector<col::CHAR_INF> buffer;
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
        std::vector<CHAR_INFO> temp;
        temp.assign(originalScreen.width*originalScreen.height, {});
        ReadConsoleOutputW(hOriginalConsole, temp.data(), bufferSize, bufferCoord, &readRegion);
        this->ConvertBufferFromPlatform(originalScreen, temp);

        // 2. 代替バッファへ移行
        printf("\x1b[?1049h\x1b[?25l");//VT100シーケンス
        hGameConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        getWindowSize(gameScreen.width, gameScreen.height, hGameConsole);
        gameScreen.reallocate(gameScreen.width, gameScreen.height);
        gameScreen_comberted.assign(gameScreen.width*gameScreen.height, {});
    }


    // デストラクタで console_finish の処理を行う
    ~Console(){}
    
    void ConvertBufferToPlatform(std::vector<CHAR_INFO>& to, const ScreenBuffer& from){
        if (to.size() != from.buffer.size()) {
            to.resize(from.buffer.size());
        }
        for(int i=0; i<from.buffer.size(); i++){
            WORD attributes = 0;
            attributes |= from.buffer[i].fore.hue;
            attributes |= (from.buffer[i].back.hue << 4);
            if(from.buffer[i].fore.isIntensity){
                attributes |= FOREGROUND_INTENSITY;
            }
            if(from.buffer[i].back.isIntensity){
                attributes |= BACKGROUND_INTENSITY;
            }
            to[i].Char.UnicodeChar = from.buffer[i].charactor;
            to[i].Attributes = attributes;
        }
    }
    void ConvertBufferFromPlatform(ScreenBuffer& to, std::vector<CHAR_INFO>& from){
        for(int i=0; i<from.size(); i++){
            to.buffer[i].charactor = from.at(i).Char.UnicodeChar;
            WORD attr = from.at(i).Attributes;
            to.buffer[i].back.hue = static_cast<col::HUE>((attr & 0x00F0) >> 4);
            to.buffer[i].fore.hue = static_cast<col::HUE>(attr & 0x000F);
            to.buffer[i].back.isIntensity = (attr & BACKGROUND_INTENSITY) != 0;
            to.buffer[i].fore.isIntensity = (attr & FOREGROUND_INTENSITY) != 0;
        }
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
            gameScreen_comberted.assign(gameScreen.width*gameScreen.height, {});
        }
    }
    void draw(const ScreenBuffer& screenToDraw){
        COORD bufferSize = { (SHORT)screenToDraw.width, (SHORT)screenToDraw.height };
        const COORD bufferCoord = { 0, 0 };
        SMALL_RECT writeRegion = { 0, 0, (SHORT)(screenToDraw.width - 1), (SHORT)(screenToDraw.height - 1) };
        
        ConvertBufferToPlatform(gameScreen_comberted, gameScreen);
        WriteConsoleOutputW(
            hGameConsole, // 書き込み先ハンドル
            gameScreen_comberted.data(),   // 書き込むデータ (CHAR_INFO 配列)
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

    struct PLATFORM_INF;
    PLATFORM_INF* platformInf;
    HANDLE hOriginalConsole;
    HANDLE hGameConsole;
    DWORD originalMode;

    ScreenBuffer originalScreen;
    ScreenBuffer gameScreen;
    std::vector<CHAR_INFO> gameScreen_comberted;
};

