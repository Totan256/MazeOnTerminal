#pragma once
#include <math.h>
#include "vec.h"
#include "maze.h"

//平面との距離と交点座標計算，返り値がマイナスなら非接触
double rayCast_sprite(dvec3 rayPos, dvec3 rayDir, dvec3 planePos, dvec3 planeNormal, dvec3 *encountPos){
    double denominator = vec_dot3D(rayDir, planeNormal);

    //内積がほぼ0の場合で平行
    if (fabs(denominator) < 1e-6) {
        return false;
    }

    dvec3 playerToPlane = vec_sub3D(planePos, rayPos);

    double t = vec_dot3D(playerToPlane, planeNormal) / denominator;

    // 距離tが負の場合、交点はプレイヤーの後ろ側
    if (t >= 0) {
        encountPos->x = rayPos.x + rayDir.x * t;
        encountPos->y = rayPos.y + rayDir.y * t;
        encountPos->z = rayPos.z + rayDir.z * t;
    }
    
    return t;
}


//参考記事https://lodev.org/cgtutor/raycasting.html
//２次元配列のマップに対して壁との距離と，X・Y平面のどちらにあたったかと，壁のナンバーを計算
double rayCast(Map *map, 
                double playerX, double playerY, double playerZ,
                double rayDirX, double rayDirY, double rayDirZ,
                int *sideFlag, int *hitNumFlag,
                double heightFloor, double heightCelling){
    int mapX = (int)playerX;
    int mapY = (int)playerY;
    
    double deltaDistX = (rayDirX == 0) ? 1e30 : fabs(1 / rayDirX);
    double deltaDistY = (rayDirY == 0) ? 1e30 : fabs(1 / rayDirY);

    double sideDistX, sideDistY;
    int stepX, stepY;

    if (rayDirX < 0) {
        stepX = -1;
        sideDistX = (playerX - mapX) * deltaDistX;
    } else {
        stepX = 1;
        sideDistX = (mapX + 1.0 - playerX) * deltaDistX;
    }
    if (rayDirY < 0) {
        stepY = -1;
        sideDistY = (playerY - mapY) * deltaDistY;
    } else {
        stepY = 1;
        sideDistY = (mapY + 1.0 - playerY) * deltaDistY;
    }

    int hit = 0;//当たり判定フラグ
    int side;//X面(0)Y面(1)　どちらに当たったかのフラグ

    const int wallFlagNum = -1;

    while (hit == 0) {
        if (sideDistX < sideDistY) {//Xグリッドに当たった
            sideDistX += deltaDistX;
            mapX += stepX;
            side = 0;
        } else {//Yグリッドに当たった
            sideDistY += deltaDistY;
            mapY += stepY;
            side = 1;
        }
        
        //マスが壁かチェック
        int num = maze_getNum(map, mapX, mapY);
        if (num > 0) {
            hit = 1;
            *hitNumFlag = num;
        }
    }

    *sideFlag = side;
    double perpWallDist;//壁までの垂直距離　魚眼効果補正
    if (side == 0) {//X
        //1ステップ分戻す
        perpWallDist = (sideDistX - deltaDistX);
    } else {//Y
        perpWallDist = (sideDistY - deltaDistY);
    }

    //床天井との当たり判定
    double rayDirLength2D = sqrt(rayDirX * rayDirX + rayDirY * rayDirY);
    double true3DWallDist = (rayDirLength2D > 1e-6) ? (perpWallDist / rayDirLength2D) : 1e30;
    if(rayDirZ > 0){
        double distCelling;
        distCelling = (1.0 / rayDirZ) * heightCelling;
        if(distCelling < true3DWallDist){
            //perpWallDist = dist
            *hitNumFlag = wallFlagNum; // 天井
        }
    }else if(rayDirZ < 0){
        double distFloor;
        distFloor = -(1.0 / rayDirZ) * heightFloor;
        if(distFloor < true3DWallDist){
            //perpWallDist = distFloor;
            *hitNumFlag = wallFlagNum; // 床
        }
    }
    
    return perpWallDist;
}
