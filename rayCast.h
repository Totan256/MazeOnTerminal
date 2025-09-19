#pragma once
#include <math.h>
//参考きｊhttps://lodev.org/cgtutor/raycasting.html

double rayCast(int map[][24],int mapSizeX, int mapSizeY, 
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
        if (map[mapX][mapY] > 0) {
            hit = 1;
            *hitNumFlag = map[mapX][mapY];
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
            *hitNumFlag = -1; // 天井
        }
    }else if(rayDirZ < 0){
        double distFloor;
        distFloor = -(1.0 / rayDirZ) * heightFloor;
        if(distFloor < true3DWallDist){
            //perpWallDist = distFloor;
            *hitNumFlag = -1; // 床
        }
    }
    
    return perpWallDist;
}
