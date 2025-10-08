#pragma once
#include "vec.hpp"
#include "maze.hpp"
#include "input.hpp"
#include <math.h>

class Player{
private:
    vec::vec3 pos;
    vec::vec2 dir;
    //dvec2 plane;
    double moveSpeed;
    double rotSpeed;
    double TotalWalkDistance;
public:
    Player();
    Player(vec::vec3 pos, double dirX, double dirY, double moveSpeed, double rotSpeed);
    void handleInput(const InputState* input, double deltaTime, maze::Maze *map);
    vec::vec3 getPos(){return pos;}
    vec::vec2 getDir(){return dir;}
    double getTotalWalkDistance() { return TotalWalkDistance; }
};

Player::Player(){
    pos.x = 1.5;
    pos.y = 0.0;
    pos.z = 1.5;
    dir.x = -3.14 * 0.25;
    dir.y = 0.0;
    moveSpeed = 2.0;
    rotSpeed =0.9;
    TotalWalkDistance = 0;
}

Player::Player(vec::vec3 _pos, double _dirX, double _dirY, double _moveSpeed, double _rotSpeed){
    pos.x = _pos.x;
    pos.y = _pos.y;
    pos.z = _pos.z;
    dir.x = _dirX;
    dir.y = _dirY;
    moveSpeed = _moveSpeed;
    rotSpeed = _rotSpeed;
    TotalWalkDistance = 0;
}


void Player::handleInput(const InputState* input, double deltaTime, maze::Maze *map) {
    double moveStepSize = moveSpeed * deltaTime;

    vec::vec2 moveDir = {0.0, 0.0};
    
    if (input->isDown[static_cast<int>(GameAction::MoveForward)]) { moveDir.y += 1.0; }
    if (input->isDown[static_cast<int>(GameAction::MoveBack)]) { moveDir.y -= 1.0; }
    if (input->isDown[static_cast<int>(GameAction::MoveLeft)]) { moveDir.x -= 1.0; }
    if (input->isDown[static_cast<int>(GameAction::MoveRight)]) { moveDir.x += 1.0; }

    if(moveDir.x != 0.0 || moveDir.y != 0.0){
        moveDir.normalize();
        double stepX = -sin(dir.x)*moveDir.y + cos(dir.x)*moveDir.x;
        stepX*=moveStepSize;
        double stepY = cos(dir.x)*moveDir.y + sin(dir.x)*moveDir.x;
        stepY*=moveStepSize;

        if(map->getNum((int)(pos.x + stepX), (int)(pos.z)) == 0) {
            pos.x += stepX;
            TotalWalkDistance += stepX;
        }
        if(map->getNum((int)(pos.x), (int)(pos.z + stepY)) == 0) {
            pos.z += stepY;
            TotalWalkDistance += stepY;
        }
    }
    
    //回転
    double rotStep = rotSpeed * deltaTime;
    dir.x -= input->deltaMouseX*rotStep;
    dir.y += input->deltaMouseY*rotStep;
    
}