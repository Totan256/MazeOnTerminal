#pragma once
#include <math.h>
#include "vec.h"
#include "maze.h"

//平面上のuv計算．normalは正規化されたものを使う
void rayCast_calcUV(dvec3 encountPos, dvec3 planePos, dvec3 normal, dvec2 *uv){
    const dvec3 up = (dvec3){0.0, 1.0, 0.0};
    
    dvec3 u;
    if (normal.x * normal.x + normal.z * normal.z < 1e-6) {//完全に真上or真下
        const dvec3 forward_axis = {1.0, 0.0, 0.0};
        u = vec_cross(forward_axis, normal);
    } else {
        u = vec_cross(up, normal);
    }

    vec_normalize3D(&u);
    dvec3 v = vec_cross(normal, u);

    dvec3 d;
    vec_sub3D(encountPos, planePos, &d);

    uv->x = vec_dot3D(d,u);
    uv->y = vec_dot3D(d,v);
}

//平面との距離と交点座標計算，返り値がマイナスなら非接触
double rayCast_sprite(dvec3 rayPos, dvec3 rayDir, dvec3 planePos, dvec3 planeNormal, dvec3 *encountPos){
    double denominator = vec_dot3D(rayDir, planeNormal);

    //内積がほぼ0の場合で平行
    if (fabs(denominator) < 1e-6) {
        return -1.0;
    }

    dvec3 playerToPlane;
    vec_sub3D(planePos, rayPos, &playerToPlane);

    double t = vec_dot3D(playerToPlane, planeNormal) / denominator;

    // 距離tが負の場合、交点はプレイヤーの後ろ側
    if (t >= 0) {
        encountPos->x = rayPos.x + rayDir.x * t;
        encountPos->y = rayPos.y + rayDir.y * t;
        encountPos->z = rayPos.z + rayDir.z * t;
    }
    
    return t;
}

typedef struct {
    bool didHit;
    double distance;
    int objectID;
    int hitSurface;
    dvec3 hitPosition;
} RaycastResult;

//参考記事https://lodev.org/cgtutor/raycasting.html
//２次元配列のマップに対して壁との距離と，X・Y平面のどちらにあたったかと，壁のナンバーを計算
RaycastResult rayCast_map(Map *map, dvec3 playerPos, dvec3 rayDir,
                double heightFloor, double heightCelling){
    int mapX = (int)playerPos.x;
    int mapY = (int)playerPos.z;
    
    double deltaDistX = (rayDir.x == 0) ? 1e30 : fabs(1 / rayDir.x);
    double deltaDistY = (rayDir.z == 0) ? 1e30 : fabs(1 / rayDir.z);

    double sideDistX, sideDistY;
    int stepX, stepY;

    if (rayDir.x < 0) {
        stepX = -1;
        sideDistX = (playerPos.x - mapX) * deltaDistX;
    } else {
        stepX = 1;
        sideDistX = (mapX + 1.0 - playerPos.x) * deltaDistX;
    }
    if (rayDir.z < 0) {
        stepY = -1;
        sideDistY = (playerPos.z - mapY) * deltaDistY;
    } else {
        stepY = 1;
        sideDistY = (mapY + 1.0 - playerPos.z) * deltaDistY;
    }

    bool hit = false;//当たり判定フラグ
    int side;//X面(0)Y面(1)　どちらに当たったかのフラグ
    int wallID;

    while (hit == false) {
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
            hit = true;
            wallID = num;
        }
    }

    double perpWallDist;//壁までの垂直距離　魚眼効果補正
    if (side == 0) {//X
        //1ステップ分戻す
        perpWallDist = (sideDistX - deltaDistX);
    } else {//Y
        perpWallDist = (sideDistY - deltaDistY);
    }

    //床天井との当たり判定
    double rayDirLength2D = sqrt(rayDir.x * rayDir.x + rayDir.z * rayDir.z);
    double true3DWallDist = (rayDirLength2D > 1e-6) ? (perpWallDist / rayDirLength2D) : 1e30;
    if(rayDir.y > 0){
        double distCelling;
        distCelling = (1.0 / rayDir.y) * heightCelling;
        if(distCelling < true3DWallDist){
            perpWallDist = distCelling;
            wallID = -1; // 天井
        }
    }else if(rayDir.y < 0){
        double distFloor;
        distFloor = -(1.0 / rayDir.y) * heightFloor;
        if(distFloor < true3DWallDist){
            perpWallDist = distFloor;
            wallID = -2; // 床
        }
    }
    
    RaycastResult value;
    value.didHit = hit;
    value.distance = perpWallDist;
    value.hitSurface = side;
    value.objectID = wallID;

    return value;
}
