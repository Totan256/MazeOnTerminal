#pragma once
#include <math.h>
#include <stdlib.h>
#include "maze.h"
#include "vec.h"
#include "console.h"
#include "rayCast.h"
#include "design.h"

void render_setBuffer(Player *player, Map *map, ScreenBuffer *sb,
        dvec3 portalPos, dvec3 portalNormal){

    for (int y = 0; y < sb->height; y++) {
        for (int x = 0; x < sb->width; x++) {
            //uv -1~1
            double uvX = (x*2.-sb->width)/min(sb->height, sb->width);
            double uvY = (sb->height - y*2.0)/min(sb->height, sb->width);
            double offSet = 2.0;
            
            dvec3 rayDirection = (dvec3){uvX, uvY, offSet};
            dvec3 rayPosition = player->pos;
            vec_normalize3D(&rayDirection);
            vec_rotate2double(&rayDirection.y,&rayDirection.z, player->dir.y);//Pitch回転 (X軸を中心にYとZを回転)
            vec_rotate2double(&rayDirection.x,&rayDirection.z, player->dir.x);//Yaw回転 (Y軸を中心にXとZを回転)
            
            
            RaycastResult mapResult = rayCast_map(map, rayPosition, rayDirection, 0.4, 0.8);
            
            WORD col = design_map(mapResult.objectID, mapResult.hitSurface);

            WCHAR s = L' ';
            dvec3 encountPos;
            double portalDist = rayCast_sprite(rayPosition,rayDirection, portalPos, portalNormal, &encountPos);
            if(0 <= portalDist && portalDist < mapResult.distance){//壁よりポータルが近い
                dvec2 portalUV;
                //s = '@';//test
                rayCast_calcUV(encountPos, portalPos, portalNormal, &portalUV);
                //if(portalUV.y < 0.0){
                if(design_portal(portalUV) != 0){
                    col = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
                }
            }
            
            int id = y * sb->width + x;
            sb->buffer[id].Char.UnicodeChar = s;
            sb->buffer[id].Attributes = col;
        }
    }
}

void render_transAnimation(ScreenBuffer *dest, ScreenBuffer *from, ScreenBuffer *to, double progress){
    bool fromIsUsable = (from!=NULL);
    bool toIsUsable = (to!=NULL);

    for(int y=0; y<dest->height; y++){
        for(int x=0; x<dest->width; x++){
            int destId = x+y*dest->width;
            CHAR_INFO fromCharInfo, toCharInfo;
            CHAR_INFO exceptionCharInfo;
            exceptionCharInfo.Char.UnicodeChar = L' ';
            exceptionCharInfo.Attributes = 0; // 黒
            
            if(x < to->width && y < to->height && toIsUsable){
                int toId = x+y*to->width;
                toCharInfo = to->buffer[toId];
            }else{
                toCharInfo = exceptionCharInfo;
            }

            if(x < from->width && y < from->height && fromIsUsable){
                int fromId = x+y*from->width;
                fromCharInfo = from->buffer[fromId];
            }else{
                fromCharInfo = exceptionCharInfo;
            }

            design_dissolveAnimation((double)x/dest->width, (double)y/dest->height,
                progress, &dest->buffer[destId],&fromCharInfo, &toCharInfo);
        }
    }
}
