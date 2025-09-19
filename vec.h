#pragma once
#include <math.h>

typedef struct {
    double x, y;
} dvec2;

typedef struct {
    int x, y;
} ivec2;

typedef struct {
    double x, y, z;
} dvec3;

typedef struct {
    int x, y, z;
} ivec3;

void rotate2D(dvec2 *n, double angle){
    double oldX = n->x;
    n->x = n->x * cos(angle) - n->y * sin(angle);
    n->y = oldX   * sin(angle) + n->y * cos(angle);
}

void rotate2double(double *x, double *y, double angle){
    double oldX = *x;
    *x = *x * cos(angle) - *y * sin(angle);
    *y = oldX   * sin(angle) + *y * cos(angle);
}

float length2D(dvec2 n){
    return sqrt(n.x*n.x + n.y*n.y);
}

void normalize2D(dvec2 *n){
    float len = length2D(*n);
    if(len==0){
        n->x=n->y=1e30;
        return;
    }
    n->x/=len;
    n->y/=len;
}
