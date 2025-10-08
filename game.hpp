#pragma once
#include "vec.hpp"
#include "input.hpp"
#include "console.hpp"
#include "player.hpp"
#include <time.h>
#include "maze.hpp"
#include "render.hpp"

typedef enum {
    GAME_STATE_START_ANIM, // スタートアニメーション中
    GAME_STATE_PLAYING,    // ゲームプレイ中
    GAME_STATE_END_ANIM,   // 終了アニメーション中
    GAME_STATE_EXIT        // 終了
} GameState;

class Game{
private:
    // システム関連
    Console console;
    InputManager inputManager;
    LARGE_INTEGER freq, lastTime, currentTime;
    double deltaTime;
    double FPS;

    // ゲームオブジェクト
    Player player;
    maze::Maze map;
    vec::vec3 portalPos;
    vec::vec3 portalNormal;

    // ゲームの状態
    GameState currentState;
    double animationFrame; // アニメーションのフレーム管理用
    double animationSpeed;

    void waitFPS(){//時間処理
        do{
            QueryPerformanceCounter(&currentTime);
            deltaTime = (double)(currentTime.QuadPart - lastTime.QuadPart) / freq.QuadPart;
        }while(deltaTime < 1.0/FPS);
    }
public:
    Game(){
        //初期数値
        const double defaultFPS = 60.0;
        const int mapSizeX = 3;
        const int mapSizeY = 3;
        const vec::vec3 portalStartNormal = {1., 0., 0.0};
        const vec::vec3 portaldefaultPos = {5.5,0.2,5.5};
        const vec::vec3 playerStartPos = {1.5,0.0,1.5};
        const double playerStartDirX = -3.14*0.25;
        const double playerStartDirY = 3.14*0.0;
        const double playerMoveSpeed = 2.;
        const double playerRotSpeed = 0.9;
        const double animationDefaultSpeed = 4.0/200.;

        //console.init(&console);
        

        //時間処理の用意
        FPS = defaultFPS;
        deltaTime = 0;
        QueryPerformanceFrequency(&freq);//タイマーの周波数
        QueryPerformanceCounter(&lastTime);//基準時間

        //ゴールポータルの設定
        portalPos = portaldefaultPos;   // ポータルの中心座標
        portalNormal = portalStartNormal; // ポータルの向き(法線ベクトル)
        portalNormal.normalize();

        //マップ用意
        map.generate(mapSizeX, mapSizeY);
        //testMaze(&map);  //test用

        //プレイヤー情報の初期化
        //player = Player(playerStartPos, playerStartDirX, playerStartDirY, playerMoveSpeed, playerRotSpeed);
        
        //アニメーション設定
        animationSpeed = animationDefaultSpeed;
        
        currentState = GAME_STATE_START_ANIM;
        animationFrame = 0;

    }
    void mainLoop() {
        // --- 共通処理 ---
        waitFPS();
        console.checkResizeAndReallocBuffer();
        inputManager.update(); // メンバ変数のメソッドを呼ぶ
        const InputState& input = inputManager.getState();

        // --- 現在のシーンに応じた処理 ---
        switch (currentState) {
            case GAME_STATE_START_ANIM: {
                ScreenBuffer firstGameScreen;
                firstGameScreen.reallocate(console.getGameScreenBuffer().width, console.getGameScreenBuffer().height);
                render::setBuffer(&player, &map, &firstGameScreen, portalPos, portalNormal);
            
                render::transAnimation(&console.getGameScreenBuffer(), &console.getOriginalScreen(), &firstGameScreen, animationFrame);
                animationFrame += animationSpeed;
                if (animationFrame >= 1.0) {
                    currentState = GAME_STATE_PLAYING; // 次のシーンへ
                }
                break;
            }

            case GAME_STATE_PLAYING: {
                {//オブジェクト動作
                    portalPos.y=0.2+portalNormal.x*0.05;
                    vec::rotate(portalNormal.x, portalNormal.z, deltaTime);//y軸中心回転
                }
                //プレイヤー操作
                player.handleInput(&input, deltaTime, &map);
                render::setBuffer(&player, &map, &console.getGameScreenBuffer(), portalPos, portalNormal);

                //ゴールポータル接触判定
                vec::vec3 relativeCoord = portalPos-player.getPos();
                if(relativeCoord.length()<0.3){
                    currentState = GAME_STATE_END_ANIM; // 次のシーンへ
                    animationFrame = 0; // アニメーションフレームをリセット
                    break;
                }

                //強制終了判定
                if (input.isPressed[static_cast<int>(GameAction::QuitGame)])
                    currentState = GAME_STATE_EXIT;
                break;
            }

            case GAME_STATE_END_ANIM: {
                ScreenBuffer lastGameScreen;
                lastGameScreen.reallocate(console.getGameScreenBuffer().width, console.getGameScreenBuffer().height);
                render::setBuffer(&player, &map, &lastGameScreen, 
                    portalPos, portalNormal);
            
                render::transAnimation(&console.getGameScreenBuffer(), &lastGameScreen, &console.getOriginalScreen(), animationFrame);
                animationFrame += animationSpeed;
                if (animationFrame >= 1.0) {
                    currentState = GAME_STATE_EXIT; // 終了へ
                }
                break;
            }

            case GAME_STATE_EXIT:
                // 何もしない
                break;
        }

        // --- 共通の描画と時間更新 ---
        console.draw(console.getGameScreenBuffer());
        lastTime = currentTime;
    }

    ~Game(){
        console.restore();//代替バッファから元のバッファへ切り替え

        printf("GAME CLEAR\n");
        printf("total walk diatance : %f\n", player.getTotalWalkDistance());
        map.print();
        //console_waitKeyUP(ACTION_QUIT_GAME);
        inputManager.waitKeyUp(GameAction::QuitGame);
    }

    void run(){
        while (currentState != GAME_STATE_EXIT) {
            mainLoop();
        }
    }
};






