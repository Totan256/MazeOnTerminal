#pragma once
#include <math.h>
#include "vec.hpp"
#include "maze.hpp"
namespace rayCast
{
    //平面上のuv計算．normalは正規化されたものを使う
    void calcUV(vec::vec3 encountPos, vec::vec3 planePos, vec::vec3 normal, vec::vec2 *uv){
        const vec::vec3 up = {0.0, 1.0, 0.0};
        
        vec::vec3 u;
        if (normal.x * normal.x + normal.z * normal.z < 1e-6) {//完全に真上or真下
            const vec::vec3 forward_axis = {1.0, 0.0, 0.0};
            u = forward_axis.cross(normal);
        } else {
            u = up.cross(normal);
        }

        u.normalize();
        vec::vec3 v = normal.cross(u);

        vec::vec3 d;
        d = encountPos-planePos;

        uv->x = d.dot(u);
        uv->y = d.dot(v);
    }

    //平面との距離と交点座標計算，返り値がマイナスなら非接触
    double sprite(vec::vec3 rayPos, vec::vec3 rayDir, vec::vec3 planePos, vec::vec3 planeNormal, vec::vec3 *encountPos){
        double denominator = rayDir.dot(planeNormal);

        //内積がほぼ0の場合で平行
        if (fabs(denominator) < 1e-6) {
            return -1.0;
        }

        vec::vec3 playerToPlane = planePos - rayPos;

        double t =playerToPlane.dot(planeNormal) / denominator;

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
        vec::vec3 hitPosition;
    } RaycastResult;

    //参考記事https://lodev.org/cgtutor/raycasting.html
    //２次元配列のマップに対して壁との距離と，X・Y平面のどちらにあたったかと，壁のナンバーを計算
    RaycastResult map(maze::Maze *map, vec::vec3 playerPos, vec::vec3 rayDir,
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
            int num = map->getNum(mapX, mapY);
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
}