// main.cpp
#include "shellGame.hpp"
#include "../input.hpp" 
#include <iostream>
#include <vector>
#include <string>

// Render関数も cursorPos を受け取るように変更
void Render(ShellGame& game, const std::string& command, int cursorPos, const std::vector<std::string>& history) {
    system("cls"); 
    
    for (const auto& entry : history) {
        std::cout << entry << std::endl; 
    }

    std::string prompt;
    std::string renderedCommand;

    if (game.currentState == ShellState::PROMPT) {
        prompt = game.getCurrentDirectory()->name + "> ";
        renderedCommand = command;
    } else if (game.currentState == ShellState::WAITING_PASSWORD) {
        prompt = "[sudo] password: ";
        renderedCommand = std::string(command.length(), '*');
    }

    std::cout << prompt;

    // パスワード入力中はカーソルをシミュレートしない
    if (game.currentState == ShellState::WAITING_PASSWORD) {
        std::cout << renderedCommand;
    } else {
        // カーソル位置をシミュレート（簡易版）
        // （注：system("cls")ではチラつきや制御に限界があります）
        std::cout << renderedCommand.substr(0, cursorPos);
        std::cout << "|"; // カーソルを | で表現
        std::cout << renderedCommand.substr(cursorPos);
    }
}

int main() {
    ShellGame game;
    TextInputManager inputManager;
    std::vector<std::string> historyLog;

    // ★状態を main で管理★
    std::string currentCommand = "";
    int cursorPos = 0;

    // 最初の描画
    Render(game, currentCommand, cursorPos, historyLog);

    while (1) {
        // 1. 入力
        TextInputResult input = inputManager.update();

        // 2. 状態更新
        bool needsRender = false;

        // 2a. 文字入力
        if (!input.typedChars.empty()) {
            // パスワード入力中も文字は受け付ける
            currentCommand.insert(cursorPos, input.typedChars);
            cursorPos += input.typedChars.length();
            needsRender = true;
        }

        // 2b. BackSpace
        if (input.backspacePressed) {
            if (cursorPos > 0) {
                currentCommand.erase(cursorPos - 1, 1);
                cursorPos--;
                needsRender = true;
            }
        }

        // 2c. 矢印キー (パスワード入力中は無視)
        if (game.currentState != ShellState::WAITING_PASSWORD) {
            if (input.arrowLeftPressed) {
                if (cursorPos > 0) {
                    cursorPos--;
                    needsRender = true;
                }
            }
            if (input.arrowRightPressed) {
                if (cursorPos < currentCommand.length()) {
                    cursorPos++;
                    needsRender = true;
                }
            }
            // (input.arrowUpPressed などはコマンド履歴の実装で使用)
        }

        // 2d. Enter
        if (input.enterPressed) {
            if (game.currentState == ShellState::PROMPT && currentCommand == "exit") {
                system("cls"); // 終了前に画面をクリア
                std::cout << "Exiting Command Maze." << std::endl;
                break;
            }

            historyLog = game.update(currentCommand);
            currentCommand = ""; // コマンド実行後にバッファをクリア
            cursorPos = 0;       // カーソル位置をリセット
            needsRender = true;
        }

        // 3. 描画
        // 状態が変更された場合のみ再描画
        if (needsRender) {
            Render(game, currentCommand, cursorPos, historyLog);
        }

        Sleep(16); // 60FPS相当の待機
    }
    
    return 0;
}