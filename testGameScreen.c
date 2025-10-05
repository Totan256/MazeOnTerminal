#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// VT100エスケープシーケンスを有効にする関数
void enableVirtualTerminalProcessing() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}

int main() {
    enableVirtualTerminalProcessing();

    // =================================================================
    // ステップ1: 実行前のコンソール画面をキャプチャ
    // =================================================================
    HANDLE hOriginalConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO originalCsbi;
    GetConsoleScreenBufferInfo(hOriginalConsole, &originalCsbi);

    // ウィンドウのサイズと表示領域を保存
    COORD originalSize = {
        (SHORT)(originalCsbi.srWindow.Right - originalCsbi.srWindow.Left + 1),
        (SHORT)(originalCsbi.srWindow.Bottom - originalCsbi.srWindow.Top + 1)
    };
    SMALL_RECT readRegion = originalCsbi.srWindow;
    
    // 表示領域を保持するバッファを確保
    CHAR_INFO* originalScreenBuffer = (CHAR_INFO*)calloc(originalSize.X * originalSize.Y, sizeof(CHAR_INFO));
    if (originalScreenBuffer == NULL) return 1;

    // 表示領域の内容をバッファに読み込む
    ReadConsoleOutputW(
        hOriginalConsole,
        originalScreenBuffer,
        originalSize,
        (COORD){0, 0},
        &readRegion
    );

    // =================================================================
    // ステップ2: 代替スクリーンバッファへ移行
    // =================================================================
    printf("\x1b[?1049h"); // 代替バッファを有効化
    printf("\x1b[?25l");  // カーソルを非表示に

    HANDLE hAlternateConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // 代替バッファに、キャプチャした元の画面を描画（アニメーションの開始点）
    WriteConsoleOutputW(hAlternateConsole, originalScreenBuffer, originalSize, (COORD){0, 0}, &readRegion);


    // =================================================================
    // ゲーム画面の準備
    // =================================================================
    CHAR_INFO* gameBuffer = (CHAR_INFO*)calloc(originalSize.X * originalSize.Y, sizeof(CHAR_INFO));
    if (gameBuffer == NULL) { /* エラー処理... */ free(originalScreenBuffer); return 1; }

    for (int y = 0; y < originalSize.Y; ++y) {
        for (int x = 0; x < originalSize.X; ++x) {
            int index = y * originalSize.X + x;
            gameBuffer[index].Char.UnicodeChar = L' ';
            if ((x + y) % 2 == 0) {
                gameBuffer[index].Attributes = BACKGROUND_RED | BACKGROUND_INTENSITY;
            } else {
                gameBuffer[index].Attributes = BACKGROUND_BLUE | BACKGROUND_INTENSITY;
            }
        }
    }


    // =================================================================
    // ステップ3: イントロアニメーション（右からワイプイン）
    // =================================================================
    for (SHORT x = originalSize.X - 1; x >= 0; --x) {
        SMALL_RECT wipeRegion = {x, 0, originalSize.X - 1, originalSize.Y - 1};
        COORD wipeCoord = {x, 0};
        WriteConsoleOutputW(hAlternateConsole, gameBuffer, originalSize, wipeCoord, &wipeRegion);
        
        // リサイズ検知
        CONSOLE_SCREEN_BUFFER_INFO currentCsbi;
        GetConsoleScreenBufferInfo(hAlternateConsole, &currentCsbi);
        if (currentCsbi.srWindow.Right != originalCsbi.srWindow.Right || currentCsbi.srWindow.Bottom != originalCsbi.srWindow.Bottom) {
            WriteConsoleOutputW(hAlternateConsole, gameBuffer, originalSize, (COORD){0,0}, &readRegion); // 即座に最終状態を描画
            break;
        }
        Sleep(8);
    }
    
    // ゲーム画面の表示
    Sleep(2000);


    // =================================================================
    // ステップ5: アウトロアニメーション（左からワイプアウト）
    // =================================================================
    for (SHORT x = 0; x < originalSize.X; ++x) {
        SMALL_RECT wipeRegion = {0, 0, x, originalSize.Y - 1};
        COORD wipeCoord = {0, 0};
        WriteConsoleOutputW(hAlternateConsole, originalScreenBuffer, originalSize, wipeCoord, &wipeRegion);
        
        // リサイズ検知
        CONSOLE_SCREEN_BUFFER_INFO currentCsbi;
        GetConsoleScreenBufferInfo(hAlternateConsole, &currentCsbi);
        if (currentCsbi.srWindow.Right != originalCsbi.srWindow.Right || currentCsbi.srWindow.Bottom != originalCsbi.srWindow.Bottom) {
            WriteConsoleOutputW(hAlternateConsole, originalScreenBuffer, originalSize, (COORD){0,0}, &readRegion); // 即座に最終状態を描画
            break;
        }
        Sleep(8);
    }
    Sleep(500); // 最後の余韻

    // =================================================================
    // ステップ6: 代替バッファを終了し、後片付け
    // =================================================================
    free(originalScreenBuffer);
    free(gameBuffer);

    printf("\x1b[?25h");  // カーソルを再表示
    printf("\x1b[?1049l"); // 元のバッファに戻す

    return 0;
}