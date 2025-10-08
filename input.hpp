#pragma once
#include <stdlib.h>
#include <windows.h>

#pragma once
#include <windows.h>
#include <array>

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
