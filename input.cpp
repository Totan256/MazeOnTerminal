#include "input.hpp"
#include <vector>

const std::array<int, static_cast<size_t>(GameAction::Count)> InputManager::keyMap = {
        'W',            //ACTION_MOVE_FORWARD
        'S',            //ACTION_MOVE_BACK
        'A',            //ACTION_MOVE_LEFT
        'D',            //ACTION_MOVE_RIGHT
        VK_SPACE,       //ACTION_JUMP
        'E',            //ACTION_INTERACT
        VK_ESCAPE       //ACTION_QUIT_GAME// Escapeキー
};

void InputManager::waitKeyUp(GameAction action){
    while((GetAsyncKeyState(keyMap.at(static_cast<int>(action))) & 0x8000) != 0){}
}

InputManager::InputManager(){
    GetCursorPos(&previousMousePos);
}

void InputManager::update(){
    // --- 1. キーボード入力の更新 ---
    for (int i = 0; i < static_cast<int>(GameAction::Count); ++i) {
        int vKey = keyMap[i];
        if (vKey == 0) continue; // 未割り当てのアクションはスキップ

        // 現在のキーが押されているかを取得
        // GetAsyncKeyStateの最上位ビットが1なら押されている
        bool currentDown = (GetAsyncKeyState(vKey) & 0x8000) != 0;

        // isDown: 現在押されているか
        currentState.isDown[i] = currentDown;

        // isPressed: 今回押されていて、前回は押されていなかったか
        currentState.isPressed[i] = currentDown && !previousDownStates[i];

        // 次のフレームのために、今回の状態を保存
        previousDownStates[i] = currentDown;
    }

    // --- 2. マウス入力の更新 ---
    POINT currentMousePos;
    GetCursorPos(&currentMousePos);

    // 前回からの差分を計算
    currentState.deltaMouseX = currentMousePos.x - previousMousePos.x;
    currentState.deltaMouseY = currentMousePos.y - previousMousePos.y;

    // 次のフレームのために、今回の位置を保存
    previousMousePos = currentMousePos;
}

void InputManager::reset(){
    GetCursorPos(&previousMousePos);
    currentState.deltaMouseX=0;
    currentState.deltaMouseY=0;
    for (int i = 0; i < static_cast<int>(GameAction::Count); ++i) {
        currentState.isDown[i] = false;
        currentState.isPressed[i] = false;
        previousDownStates[i] = false;
    }
}


TextInputManager::TextInputManager() {
    hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
}

TextInputResult TextInputManager::update() {
    TextInputResult result;
    DWORD numEvents = 0;
    GetNumberOfConsoleInputEvents(hConsoleInput, &numEvents);

    if (numEvents == 0) return result; // 入力がなければ即終了

    std::vector<INPUT_RECORD> inputBuffer(numEvents);
    DWORD numEventsRead = 0;
    ReadConsoleInput(hConsoleInput, inputBuffer.data(), numEvents, &numEventsRead);

    for (DWORD i = 0; i < numEventsRead; ++i) {
        if (inputBuffer[i].EventType == KEY_EVENT &&
            inputBuffer[i].Event.KeyEvent.bKeyDown)
        {
            // キーが押されたイベントのみ処理
            WORD keyCode = inputBuffer[i].Event.KeyEvent.wVirtualKeyCode;
            WCHAR unicodeChar = inputBuffer[i].Event.KeyEvent.uChar.UnicodeChar;

            if (keyCode == VK_RETURN) {
                result.enterPressed = true;
            } else if (keyCode == VK_BACK) {
                result.backspacePressed = true;
            } else if (keyCode == VK_LEFT) {
                result.arrowLeftPressed = true;
            } else if (keyCode == VK_RIGHT) {
                result.arrowRightPressed = true;
            } else if (keyCode == VK_UP) {
                result.arrowUpPressed = true;
            } else if (keyCode == VK_DOWN) {
                result.arrowDownPressed = true;
            } else if (unicodeChar >= 32) {
                char c = static_cast<char>(unicodeChar); 
                result.typedChars += c;
            }
        }
    }
    return result;
}