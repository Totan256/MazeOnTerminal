#pragma once
#include "vec.hpp"
#include "math.h"
#include "windows.h"
#include "console.hpp"
namespace design
{
    //0で範囲外
    int portal(vec::vec2 uv){
        double s = uv.length();
        if(0.3<s&&s<0.5){
            return 1;
        }
        return 0;
    }

    col::CHAR_INF map(int numFlag, int sideFlag){
        col::CHAR_INF result;
        col::COL_INF backCol;
        WCHAR s = L' ';
        {
            switch (numFlag) {
                case -1://天井
                    backCol = {col::BLACK, false}; //black
                    s=L'#';
                    break;
                case -2://床
                    backCol = {col::BLACK, true}; //black
                    s=L'▓';
                    break;
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                    s=L'P';
                    if(sideFlag==1)
                        backCol = {col::BLUE, true};
                    else
                        backCol = {col::BLUE, false};
                    break;

                default:
                    //デバッグ用
                    backCol.hue = col::RED;
                    break;
            }
        }
        result.charactor = s;
        result.back = backCol;
        return result;
    }

    double hash_1d(double n){
        return floor(sin(n*542.323)*3245.6453);
    }
    double hash_2d(double n1, double n2){
        return floor(sin(n1*542.323-n2*321.234)*3245.6453);
    }

    // void diagonalAnimation(double x, double y, double progress, CHAR_INFO *dest, CHAR_INFO *from, CHAR_INFO *to){
    //     if(x+y < progress){
    //         dest->Char.UnicodeChar = (hash_2d(x,y)>0.5) ? L'0' : L'1';
    //         dest->Attributes = (hash_2d(y,x)>0.5) ? FOREGROUND_GREEN : FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    //         if(x+y < progress*0.8){
    //             *dest = *to;
    //         }
    //     }else{
    //         *dest = *from;
    //     }
    // }

    void dissolveAnimation(double x, double y, double progress, col::CHAR_INF *dest, col::CHAR_INF *from, col::CHAR_INF *to){
        float threshold = (rand() % 100) / 100.0f;
        if(threshold*0.5 < progress){
            dest->charactor = (hash_2d(x,y)>0.5) ? L'0' : L'1';
            if(threshold*0.75 < progress){
                dest->back = from->back;
                dest->fore = dest->fore;
            }
            else{
                dest->back = to->back;
                dest->fore = to->fore;
            }
            dest->back = {col::GREEN, (hash_2d(y,x)>0.5)};
            if(threshold*0.9 < progress){
                *dest = *to;
            }
        }else{
            *dest = *from;
        }
    }
}