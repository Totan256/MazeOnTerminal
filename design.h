#include "vec.h"
#include "math.h"
#include "windows.h"

int design_portal(dvec2 uv){
    double s = vec_length2D(uv);
    if(3.<s&&s<5.){
        return 1;
    }
    return 0;
}

WORD design_map(int numFlag, int sideFlag){
    WORD col;
    {
        switch (numFlag) {
            case -1://床・天井
                col = 0x0000; //black
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