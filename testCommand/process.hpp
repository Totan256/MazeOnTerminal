#pragma once

#include <string>
class Game;

class Process{
public:
    Game* game;
    Process(Game& _game, int _id){
        id = _id;
        game = &_game;
        isOperating = true;
    }
    virtual void update() = 0;
    int id;
    bool isOperating;
    std::string name;
};

class MazeMaster : public Process{
public:
    MazeMaster(Game& game, int id) : Process(game, id) {
        name = "MazeMaster";
    }
    void update() override{

    }
};

class Trader : public Process{
    public:
    Trader(Game& game, int id) : Process(game, id) {
        name = "trader";
    }
    void update() override{

    }
};