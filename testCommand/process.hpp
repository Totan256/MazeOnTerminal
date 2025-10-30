#pragma once

#include <string>
class ShellGame;

class Process{
public:
    ShellGame* game;
    Process(ShellGame& _game, int _id){
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
    MazeMaster(ShellGame& game, int id) : Process(game, id) {
        name = "MazeMaster";
    }
    void update() override{

    }
};

class Trader : public Process{
    public:
    Trader(ShellGame& game, int id) : Process(game, id) {
        name = "trader";
    }
    void update() override{

    }
};