#include "input.hpp"
const std::array<int, static_cast<size_t>(GameAction::Count)> InputManager::keyMap = {
        'W',            //ACTION_MOVE_FORWARD
        'S',            //ACTION_MOVE_BACK
        'A',            //ACTION_MOVE_LEFT
        'D',            //ACTION_MOVE_RIGHT
        VK_SPACE,       //ACTION_JUMP
        'E',            //ACTION_INTERACT
        VK_RETURN       //ACTION_QUIT_GAME// Enterキー
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