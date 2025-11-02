#pragma once
#include <stdlib.h>
#include <windows.h>
#include <array>
#include <string>

// enumはenum classにすると、より型安全になる
enum class GameAction {
    MoveForward,
    MoveBack,
    MoveLeft,
    MoveRight,
    Jump,
    Interact,
    QuitGame,
    // アクションの総数を保持するマーカー
    Count
};

struct InputState {
    std::array<bool, static_cast<size_t>(GameAction::Count)> isDown;
    std::array<bool, static_cast<size_t>(GameAction::Count)> isPressed;
    long deltaMouseX = 0;
    long deltaMouseY = 0;
};

class InputManager {
public:
    InputManager();

    void update();
    void reset();
    const InputState& getState() const { return currentState; }
    
    static void waitKeyUp(GameAction action);

private:
    InputState currentState;
    std::array<bool, static_cast<size_t>(GameAction::Count)> previousDownStates;
    POINT previousMousePos;

    // キーマップもクラスの静的定数メンバとしてカプセル化
    static const std::array<int, static_cast<size_t>(GameAction::Count)> keyMap;
};



// シェル入力の結果を格納する構造体
struct TextInputResult {
    std::string typedChars;
    bool backspacePressed = false;
    bool enterPressed = false;
    bool arrowLeftPressed = false;
    bool arrowRightPressed = false;
    bool arrowUpPressed = false;   // (コマンド履歴用)
    bool arrowDownPressed = false; // (コマンド履歴用)
};

// シェル（テキスト入力）専用の新しい入力クラス
// InputManager とは「同居」する
class TextInputManager {
public:
    TextInputManager();
    // 毎フレーム呼び出され、入力イベントを処理する
    TextInputResult update();
private:
    HANDLE hConsoleInput;
};