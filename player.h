#pragma once
#include "vec.h"
#include "maze.h"
#include "input.h"
#include <math.h>
#include <windows.h>

typedef struct {
    dvec3 pos;
    dvec2 dir;
    //dvec2 plane;
    double moveSpeed;
    double rotSpeed;
    POINT lastMousePos;
    POINT prevMousePos;
} Player;

void player_init(Player *p, dvec3 pos, double dirX, double dirY, double moveSpeed, double rotSpeed){
    p->pos.x = pos.x;
    p->pos.y = pos.y;
    p->pos.z = pos.z;
    p->dir.x = dirX;
    p->dir.y = dirY;
    p->moveSpeed = moveSpeed;
    p->rotSpeed = rotSpeed;
    //カーソルをウィンドウ中央に
    //うまくいかない．．．どうやらwindowsコンソールでは無理そう
    /*
    RECT windowRect;
    HWND consoleWindow = GetConsoleWindow();
    GetWindowRect(consoleWindow, &windowRect);
    int centerX = (windowRect.left + windowRect.right) / 2;
    int centerY = (windowRect.top + windowRect.bottom) / 2;
    SetCursorPos(centerX, centerY);
    
    //カーソル非表示
    ShowCursor(FALSE);
    */
    GetCursorPos(&p->lastMousePos);
}

void player_handleInput(Player *p, const InputState* input, double deltaTime, Map *map) {
    double moveStepSize = p->moveSpeed * deltaTime;

    dvec2 moveDir = {0.0, 0.0};
    
    if (input->isDown[ACTION_MOVE_FORWARD]) { moveDir.y += 1.0; }
    if (input->isDown[ACTION_MOVE_BACK]) { moveDir.y -= 1.0; }
    if (input->isDown[ACTION_MOVE_LEFT]) { moveDir.x -= 1.0; }
    if (input->isDown[ACTION_MOVE_RIGHT]) { moveDir.x += 1.0; }

    if(moveDir.x != 0.0 || moveDir.y != 0.0){
        vec_normalize2D(&moveDir);
        double stepX = -sin(p->dir.x)*moveDir.y + cos(p->dir.x)*moveDir.x;
        stepX*=moveStepSize;
        double stepY = cos(p->dir.x)*moveDir.y + sin(p->dir.x)*moveDir.x;
        stepY*=moveStepSize;

        if(maze_getNum(map, (int)(p->pos.x + stepX), (int)(p->pos.z)) == 0) {
            p->pos.x += stepX;
        }
        if(maze_getNum(map, (int)(p->pos.x), (int)(p->pos.z + stepY)) == 0) {
            p->pos.z += stepY;
        }
    }
    
    // //移動
    // if(GetAsyncKeyState('W') & 0x8000) {
    //     float stepX = -sin(p->dir.x)*moveStepSize;
    //     float stepY = cos(p->dir.x)*moveStepSize;
        
    //     if(maze_getNum(map, (int)(p->pos.x+stepX), (int)(p->pos.z)) == 0) {
    //         p->pos.x += stepX;
    //     }
    //     if(maze_getNum(map, (int)(p->pos.x), (int)(p->pos.z + stepY)) == 0) {
    //         p->pos.z += stepY;
    //     }
    // }
    // if(GetAsyncKeyState('S') & 0x8000) {
    //     float stepX = -sin(p->dir.x)*moveStepSize;
    //     float stepY = cos(p->dir.x)*moveStepSize;
        
    //     if(maze_getNum(map, (int)(p->pos.x - stepX), (int)(p->pos.z)) == 0) {
    //         p->pos.x -= stepX;
    //     }
    //     if(maze_getNum(map, (int)(p->pos.x), (int)(p->pos.z - stepY)) == 0) {
    //         p->pos.z -= stepY;
    //     }
    // }
    // if(GetAsyncKeyState('D') & 0x8000){
    //     float stepX = cos(p->dir.x)*moveStepSize;
    //     float stepY = sin(p->dir.x)*moveStepSize;
    //     if(maze_getNum(map, (int)(p->pos.x + stepX), (int)(p->pos.z)) == 0) {
    //         p->pos.x += stepX;
    //     }
    //     if(maze_getNum(map, (int)(p->pos.x), (int)(p->pos.z + stepY)) == 0) {
    //         p->pos.z += stepY;
    //     }
    // }
    // if(GetAsyncKeyState('A') & 0x8000){
    //     float stepX = cos(p->dir.x)*moveStepSize;
    //     float stepY = sin(p->dir.x)*moveStepSize;
    //     if(maze_getNum(map, (int)(p->pos.x - stepX), (int)(p->pos.z)) == 0) {
    //         p->pos.x -= stepX;
    //     }
    //     if(maze_getNum(map, (int)(p->pos.x), (int)(p->pos.z - stepY)) == 0) {
    //         p->pos.z -= stepY;
    //     }
    // }
    
    //回転
    double rotStep = p->rotSpeed * deltaTime;
    GetCursorPos(&p->prevMousePos);
    float deltaMousePosX = p->prevMousePos.x - p->lastMousePos.x;
    float deltaMousePosY = p->prevMousePos.y - p->lastMousePos.y;
    p->lastMousePos = (POINT){p->prevMousePos.x, p->prevMousePos.y};
    p->dir.x -= deltaMousePosX*rotStep;
    p->dir.y += deltaMousePosY*rotStep;
    
}