#pragma once

#include "testCommand/shellGame.hpp"
#include "input.hpp"
#include <vector>
#include <string>


class shellTextEditer{
public:
    //std::vector<std::string> historyLog;
    std::string currentCommand;
    bool enterPressed=false;
    int cursorPos;
    shellTextEditer(ShellGame& sgame):sgame(&sgame){
        reset();
    }

    void update() {
        input = inputManager.update();

        // 文字入力
        if (!input.typedChars.empty()) {
            currentCommand.insert(cursorPos, input.typedChars);
            cursorPos += input.typedChars.length();
        }

        // BackSpace
        if (input.backspacePressed) {
            if (cursorPos > 0) {
                currentCommand.erase(cursorPos - 1, 1);
                cursorPos--;
            }
        }

        // 矢印キー (パスワード入力中は無視)
        if (sgame->currentState != ShellState::WAITING_PASSWORD) {
            if (input.arrowLeftPressed) {
                if (cursorPos > 0) {
                    cursorPos--;
                }
            }
            if (input.arrowRightPressed) {
                if (cursorPos < currentCommand.length()) {
                    cursorPos++;
                }
            }
            // (input.arrowUpPressed などはコマンド履歴の実装で使用)
        }
        enterPressed = input.enterPressed;
    }
    void reset(){
        currentCommand = "";
        cursorPos = 0;
        enterPressed = false;
    }
private:
    ShellGame* sgame;
    TextInputManager inputManager;
    TextInputResult input;
};

