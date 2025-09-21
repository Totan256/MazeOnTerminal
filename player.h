#pragma once
#include "vec.h"
#include <math.h>
#include <windows.h>

typedef struct {
    dvec2 pos;
    dvec2 dir;
    dvec2 plane;
    double moveSpeed;
    double rotSpeed;
    POINT lastMousePos;
    POINT prevMousePos;
} Player;

void player_init(Player *p, double x, double y, double dirX, double dirY, double moveSpeed, double rotSpeed);
void player_handleInput(Player *p, double deltaTime, int map[][24]);

void player_init(Player *p, double x, double y, double dirX, double dirY, double moveSpeed, double rotSpeed){
    p->pos.x = x;
    p->pos.y = y;
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

void player_handleInput(Player *p, double deltaTime, int map[][24]) {
    double moveStep = p->moveSpeed * deltaTime;
    
    //移動
    if (GetAsyncKeyState('W') & 0x8000) {
        float stepX = -sin(p->dir.x)*moveStep;
        float stepY = cos(p->dir.x)*moveStep;
        if (map[(int)(p->pos.x + stepX)][(int)(p->pos.y)] == 0) {
            p->pos.x += stepX;
        }
        if (map[(int)(p->pos.x)][(int)(p->pos.y + stepY)] == 0) {
            p->pos.y += stepY;
        }
    }
    if (GetAsyncKeyState('S') & 0x8000) {
        float stepX = -sin(p->dir.x)*moveStep;
        float stepY = cos(p->dir.x)*moveStep;
        if (map[(int)(p->pos.x - stepX)][(int)(p->pos.y)] == 0) {
            p->pos.x -= stepX;
        }
        if (map[(int)(p->pos.x)][(int)(p->pos.y - stepY)] == 0) {
            p->pos.y -= stepY;
        }
    }
    if (GetAsyncKeyState('D') & 0x8000) {
        float stepX = cos(p->dir.x)*moveStep;
        float stepY = sin(p->dir.x)*moveStep;
        if (map[(int)(p->pos.x + stepX)][(int)(p->pos.y)] == 0) {
            p->pos.x += stepX;
        }
        if (map[(int)(p->pos.x)][(int)(p->pos.y + stepY)] == 0) {
            p->pos.y += stepY;
        }
    }
    if (GetAsyncKeyState('A') & 0x8000) {
        float stepX = cos(p->dir.x)*moveStep;
        float stepY = sin(p->dir.x)*moveStep;
        if (map[(int)(p->pos.x - stepX)][(int)(p->pos.y)] == 0) {
            p->pos.x -= stepX;
        }
        if (map[(int)(p->pos.x)][(int)(p->pos.y - stepY)] == 0) {
            p->pos.y -= stepY;
        }
    }
    
    //回転
    double rotStep = p->rotSpeed * deltaTime;
    GetCursorPos(&p->prevMousePos);
    float deltaMousePosX = p->prevMousePos.x - p->lastMousePos.x;
    float deltaMousePosY = p->prevMousePos.y - p->lastMousePos.y;
    p->lastMousePos = (POINT){p->prevMousePos.x, p->prevMousePos.y};
    p->dir.x -= deltaMousePosX*rotStep;
    p->dir.y += deltaMousePosY*rotStep;
    
}