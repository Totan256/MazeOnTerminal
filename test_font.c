#include <windows.h>
#include <stdio.h>
#include <string.h> // wcslen

// 指定した座標にサンプルテキストを描画するヘルパー関数
void drawSampleText(HANDLE hConsole, const wchar_t* text, COORD position) {
    DWORD charsWritten;
    // 画面をクリア (空白文字で埋める)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    DWORD consoleSize = csbi.dwSize.X * csbi.dwSize.Y;
     // 指定した位置にテキストを書き込む
    FillConsoleOutputCharacterW(hConsole, (WCHAR)' ', consoleSize, (COORD){0, 0}, &charsWritten);
    
    // 指定した位置にテキストを書き込む
    WriteConsoleOutputCharacterW(hConsole, text, wcslen(text), position, &charsWritten);
}

int main() {
    // --- Setup Alternate Screen Buffer ---
    HANDLE hOriginalConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hGameConsole = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    if (hGameConsole == INVALID_HANDLE_VALUE) {
        printf("Error: Could not create alternate screen buffer.\n");
        return 1;
    }
    SetConsoleActiveScreenBuffer(hGameConsole);
    printf("Switched to alternate buffer. Please wait...\n");
    Sleep(500); // Wait a moment for the buffer switch to settle

    // --- Wait for the window to become active ---
    int attempts = 0;
    while (GetForegroundWindow() != GetConsoleWindow() && attempts < 30) {
        Sleep(100);
        attempts++;
    }
    if (attempts >= 30) {
         printf("Warning: Window did not become active.\n");
    }

    // --- 1. Get and Display Original Font ---
    CONSOLE_FONT_INFOEX originalFont;
    originalFont.cbSize = sizeof(originalFont);
    if (!GetCurrentConsoleFontEx(hGameConsole, FALSE, &originalFont)) {
        printf("Error: Failed to get original font info. Code: %lu\n", GetLastError());
        SetConsoleActiveScreenBuffer(hOriginalConsole);
        CloseHandle(hGameConsole);
        return 1;
    }
    wchar_t originalSizeText[64];
    swprintf(originalSizeText, 64, L"Font Size: %d", originalFont.dwFontSize.Y);
    drawSampleText(hGameConsole, originalSizeText, (COORD){5, 5});
    Sleep(3000); // 3 seconds to observe

    // --- 2. Try to Set New Font ---
    CONSOLE_FONT_INFOEX newFont = originalFont;
    newFont.dwFontSize.Y = 514; // Change height to 32
    newFont.dwFontSize.X = 0;  // Let the system choose the width

    if (!SetCurrentConsoleFontEx(hGameConsole, FALSE, &newFont)) {
        drawSampleText(hGameConsole, L"Error: SetCurrentConsoleFontEx failed.", (COORD){5, 5});
    } else {
        wchar_t newSizeText[64];
        swprintf(newSizeText, 64, L"Font size: %d", newFont.dwFontSize.Y);
        drawSampleText(hGameConsole, newSizeText, (COORD){5, 5});
    }
    Sleep(5000); // 5 seconds to observe the change (or lack thereof)

    // --- 3. Try to Restore Original Font ---
    if (!SetCurrentConsoleFontEx(hGameConsole, FALSE, &originalFont)) {
        drawSampleText(hGameConsole, L"Error: Failed to restore font.", (COORD){5, 5});
    } else {
        drawSampleText(hGameConsole, L"Restored original font.", (COORD){5, 5});
    }
    Sleep(3000); // 3 seconds to observe the restoration

    // --- Cleanup ---
    SetConsoleActiveScreenBuffer(hOriginalConsole);
    CloseHandle(hGameConsole);
    printf("Test finished. Returned to original console.\n");

    return 0;
}