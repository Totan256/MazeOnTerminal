#include "vec.h"
#include "math.h"
#include "windows.h"
#include "console.h"

void design_startAnimation(Console *c){

}


//0で範囲外
int design_portal(dvec2 uv){
    double s = vec_length2D(uv);
    if(0.3<s&&s<0.5){
        return 1;
    }
    return 0;
}

WORD design_map(int numFlag, int sideFlag){
    WORD col;
    {
        switch (numFlag) {
            case -1://天井
                col = 0x0000; //black
                break;
            case -2://床
                col = 0x0000 | BACKGROUND_INTENSITY; //black
                break;
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                if(sideFlag==1)
                    col = col = BACKGROUND_BLUE | BACKGROUND_INTENSITY;
                else
                    col = col = BACKGROUND_BLUE;
                break;

            default:
                //デバッグ用
                col = BACKGROUND_RED;
                break;
        }
    }
    return col;
}

double hash_1d(double n){
    return floor(sin(n*542.323)*3245.6453);
}
double hash_2d(double n1, double n2){
    return floor(sin(n1*542.323-n2*321.234)*3245.6453);
}

void design_diagonalAnimation(double x, double y, double progress, CHAR_INFO *dest, CHAR_INFO *from, CHAR_INFO *to){
    if(x+y < progress){
        dest->Char.UnicodeChar = (hash_2d(x,y)>0.5) ? L'0' : L'1';
        dest->Attributes = (hash_2d(y,x)>0.5) ? FOREGROUND_GREEN : FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        if(x+y < progress*0.8){
            *dest = *to;
        }
    }else{
        *dest = *from;
    }
}

void design_dissolveAnimation(double x, double y, double progress, CHAR_INFO *dest, CHAR_INFO *from, CHAR_INFO *to){
    float threshold = (rand() % 100) / 100.0f;
    if(threshold*0.5 < progress){
        dest->Char.UnicodeChar = (hash_2d(x,y)>0.5) ? L'0' : L'1';
        if(threshold*0.75 < progress)
            dest->Attributes = from->Attributes;
        else
            dest->Attributes = to->Attributes;
        dest->Attributes |= (hash_2d(y,x)>0.5) ? FOREGROUND_GREEN : FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        if(threshold*0.9 < progress){
            *dest = *to;
        }
    }else{
        *dest = *from;
    }
}