#pragma once
#include <stdlib.h>
#include <windows.h>

typedef enum {
    // 移動入力
    ACTION_MOVE_FORWARD,
    ACTION_MOVE_BACK,
    ACTION_MOVE_LEFT,
    ACTION_MOVE_RIGHT,
    // ジャンプと設定キーとか　あとでやる
    ACTION_JUMP,
    ACTION_INTERACT,
    ACTION_QUIT_GAME, // ゲーム終了などのシステム操作

    // アクションの総数を保持するマーカー
    ACTION_COUNT
} GameAction;

typedef struct {
    // 各アクションが現在「押され続けているか」 (isDown)
    bool isDown[ACTION_COUNT];
    
    // 各アクションがこのフレームで「押された瞬間か」 (isPressed)
    bool isPressed[ACTION_COUNT];

    // 前回のフレームからのマウスの相対移動量
    long deltaMouseX;
    long deltaMouseY;
} InputState;

// 最新の入力状態を保持する
static InputState g_currentState = {0};

// 1フレーム前のキー状態を保持する (isPressed判定用)
static bool g_previousDownStates[ACTION_COUNT] = {0};

// 1フレーム前のマウスカーソル位置
static POINT g_previousMousePos = {0};


// --- キーマッピング定義 ---
// ゲームのアクションと物理キーを対応付ける配列
// 将来的にキーコンフィグ機能を作る場合、この配列を外部ファイルから読み込むように変更する
static const int g_keyMap[ACTION_COUNT] = {
    'W',            //ACTION_MOVE_FORWARD
    'S',            //ACTION_MOVE_BACK
    'A',            //ACTION_MOVE_LEFT
    'D',            //ACTION_MOVE_RIGHT
    VK_SPACE,       //ACTION_JUMP
    'E',            //ACTION_INTERACT
    VK_RETURN       //ACTION_QUIT_GAME// Enterキー
};



void input_init(void) {
    GetCursorPos(&g_previousMousePos);
}

void input_update(void) {
    // --- 1. キーボード入力の更新 ---
    for (int i = 0; i < ACTION_COUNT; ++i) {
        int vKey = g_keyMap[i];
        if (vKey == 0) continue; // 未割り当てのアクションはスキップ

        // 現在のキーが押されているかを取得
        // GetAsyncKeyStateの最上位ビットが1なら押されている
        bool currentDown = (GetAsyncKeyState(vKey) & 0x8000) != 0;

        // isDown: 現在押されているか
        g_currentState.isDown[i] = currentDown;

        // isPressed: 今回押されていて、前回は押されていなかったか
        g_currentState.isPressed[i] = currentDown && !g_previousDownStates[i];

        // 次のフレームのために、今回の状態を保存
        g_previousDownStates[i] = currentDown;
    }
    
    // --- 2. マウス入力の更新 ---
    POINT currentMousePos;
    GetCursorPos(&currentMousePos);

    // 前回からの差分を計算
    g_currentState.deltaMouseX = currentMousePos.x - g_previousMousePos.x;
    g_currentState.deltaMouseY = currentMousePos.y - g_previousMousePos.y;

    // 次のフレームのために、今回の位置を保存
    g_previousMousePos = currentMousePos;
}

const InputState* input_getState(void) {
    // 外部からは内部状態を直接書き換えられないように、constポインタを返す
    return &g_currentState;
}