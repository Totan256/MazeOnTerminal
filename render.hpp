#pragma once
#include <cmath>
#include <stdlib.h>
#include <algorithm>
#include "vec.hpp"
#include "maze.hpp"
#include "console.hpp"
#include "player.hpp"
#include "rayCast.hpp"
#include "design.hpp"

namespace render
{
    void setBuffer(Player *player, maze::Maze *map, ScreenBuffer *sb,
            vec::vec3 portalPos, vec::vec3 portalNormal){

        for (int y = 0; y < sb->height; y++) {
            for (int x = 0; x < sb->width; x++) {
                //uv -1~1
                double uvX = (x*2.-sb->width)/std::min<int>(sb->height, sb->width);
                double uvY = (sb->height - y*2.0)/std::min<int>(sb->height, sb->width);
                double offSet = 2.0;
                
                vec::vec3 rayDirection = {uvX, uvY, offSet};
                vec::vec3 rayPosition = player->getPos();
                rayDirection.normalize();
                vec::rotate(rayDirection.y, rayDirection.z, player->getDir().y);//Pitch回転 (X軸を中心にYとZを回転)
                vec::rotate(rayDirection.x, rayDirection.z, player->getDir().x);//Yaw回転 (Y軸を中心にXとZを回転)                
                
                rayCast::RaycastResult mapResult = rayCast::map(map, rayPosition, rayDirection, 0.4, 0.8);
                CHAR_INFO pixelData = design::map(mapResult.objectID, mapResult.hitSurface);

                vec::vec3 encountPos;
                double portalDist = rayCast::sprite(rayPosition,rayDirection, portalPos, portalNormal, &encountPos);
                if(0 <= portalDist && portalDist < mapResult.distance){//壁よりポータルが近い
                    vec::vec2 portalUV;
                    rayCast::calcUV(encountPos, portalPos, portalNormal, &portalUV);
                    //if(portalUV.y < 0.0){
                    if(design::portal(portalUV) != 0){
                        pixelData.Attributes = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
                        pixelData.Char.UnicodeChar = L' ';
                    }
                }
                
                int id = y * sb->width + x;
                sb->buffer[id] = pixelData;
            }
        }
    }

    void transAnimation(ScreenBuffer *dest, const ScreenBuffer *from, const ScreenBuffer *to, double progress){
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

                design::dissolveAnimation((double)x/dest->width, (double)y/dest->height,
                    progress, &dest->buffer[destId],&fromCharInfo, &toCharInfo);
            }
        }
    }
} // namespace render

