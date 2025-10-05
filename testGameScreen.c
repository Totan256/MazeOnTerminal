#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// グローバルで管理するコンソール状態
HANDLE g_hConsole;
CHAR_INFO* g_backBuffer = NULL;
SHORT g_width = 0, g_height = 0;

// リサイズをチェックし、必要ならバックバッファを再確保する関数
BOOL checkResizeAndReallocBuffers() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(g_hConsole, &csbi);
    SHORT newWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    SHORT newHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    if (newWidth != g_width || newHeight != g_height) {
        g_width = newWidth;
        g_height = newHeight;
        
        // reallocでバッファをリサイズ（初回はmallocと同じ）
        CHAR_INFO* newBuffer = (CHAR_INFO*)realloc(g_backBuffer, g_width * g_height * sizeof(CHAR_INFO));
        if (newBuffer == NULL) {
            fprintf(stderr, "Failed to reallocate buffer\n");
            return FALSE;
        }
        g_backBuffer = newBuffer;
    }
    return TRUE;
}


int main() {
    // 初期設定
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOriginalConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD originalMode;
    GetConsoleMode(hOriginalConsole, &originalMode);
    SetConsoleMode(hOriginalConsole, originalMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    // 1. 元の画面をキャプチャ
    CONSOLE_SCREEN_BUFFER_INFO originalCsbi;
    GetConsoleScreenBufferInfo(hOriginalConsole, &originalCsbi);
    SHORT originalWidth = originalCsbi.srWindow.Right - originalCsbi.srWindow.Left + 1;
    SHORT originalHeight = originalCsbi.srWindow.Bottom - originalCsbi.srWindow.Top + 1;
    SMALL_RECT readRegion = originalCsbi.srWindow;
    
    CHAR_INFO* originalScreenBuffer = (CHAR_INFO*)calloc(originalWidth * originalHeight, sizeof(CHAR_INFO));
    ReadConsoleOutputW(hOriginalConsole, originalScreenBuffer, (COORD){originalWidth, originalHeight}, (COORD){0, 0}, &readRegion);

    // 2. 代替バッファへ移行
    printf("\x1b[?1049h\x1b[?25l");
    g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // =================================================================
    // 3. メインアニメーションループ
    // =================================================================
    int max_frames = (originalWidth + originalHeight) * 2; // アニメーションの総フレーム数
    for (int frame = 0; frame < max_frames; ++frame) {
        
        // ◆ リサイズチェックとバッファの再確保
        if (!checkResizeAndReallocBuffers()) break;

        // ◆ フレームごとのバッファ合成（Composition）
        for (SHORT y = 0; y < g_height; ++y) {
            for (SHORT x = 0; x < g_width; ++x) {
                int current_id = x + y * g_width;

                // アニメーション判定（斜めに切り替わる）
                if (x + y < frame) {
                    // ゲーム画面を描画
                    g_backBuffer[current_id].Char.UnicodeChar = L' ';
                    g_backBuffer[current_id].Attributes = ((x + y) % 2 == 0) ? (BACKGROUND_RED | BACKGROUND_INTENSITY) : (BACKGROUND_BLUE | BACKGROUND_INTENSITY);
                } else {
                    // 元のコンソール画面を描画
                    // ◆ 座標マッピングの核心部分
                    if (x < originalWidth && y < originalHeight) {
                        // 範囲内なら、元の幅を使ってインデックスを計算
                        int original_id = x + y * originalWidth;
                        g_backBuffer[current_id] = originalScreenBuffer[original_id];
                    } else {
                        // 範囲外（リサイズで広くなった部分）
                        g_backBuffer[current_id].Char.UnicodeChar = L' ';
                        g_backBuffer[current_id].Attributes = 0; // 黒
                    }
                }
            }
        }

        // ◆ 合成したバッファを一括で画面に転送
        SMALL_RECT writeRegion = {0, 0, g_width - 1, g_height - 1};
        WriteConsoleOutputW(g_hConsole, g_backBuffer, (COORD){g_width, g_height}, (COORD){0, 0}, &writeRegion);

        Sleep(8);
    }
    
    // アニメーション終了後、ゲーム画面として少し待機
    Sleep(2000);

    // 4. 後片付け
    free(originalScreenBuffer);
    free(g_backBuffer);
    // 元のバッファに戻す
    printf("\x1b[?1049l");

    // 【対策】復元後、カーソル位置を整える
    // 現在のウィンドウの高さを取得
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOriginalConsole, &csbi);
    SHORT finalHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    
    // カーソルをウィンドウの左下に移動させてから改行し、プロンプトがきれいな行から始まるようにする
    printf("\x1b[%d;1H\n", finalHeight);

    // コンソールモードも元に戻す
    SetConsoleMode(hOriginalConsole, originalMode); 

    return 0;
}