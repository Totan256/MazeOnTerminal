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

void vec_rotate2D(dvec2 *n, double angle){
    double oldX = n->x;
    n->x = n->x * cos(angle) - n->y * sin(angle);
    n->y = oldX   * sin(angle) + n->y * cos(angle);
}

void vec_rotate2double(double *x, double *y, double angle){
    double oldX = *x;
    *x = *x * cos(angle) - *y * sin(angle);
    *y = oldX   * sin(angle) + *y * cos(angle);
}

float vec_length2D(dvec2 n){
    return sqrt(n.x*n.x + n.y*n.y);
}

float vec_length3D(dvec3 n){
    return sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
}

void vec_normalize2D(dvec2 *n){
    float len = vec_length2D(*n);
    if(len==0){
        n->x=n->y=1e30;
        return;
    }
    n->x/=len;
    n->y/=len;
}

void vec_normalize3D(dvec3 *n){
    float len = vec_length3D(*n);
    if(len==0){
        n->x=n->y=n->z=1e30;
        return;
    }
    n->x/=len;
    n->y/=len;
    n->z/=len;
}

double vec_dot2D(dvec2 a, dvec2 b) {
    return a.x * b.x + a.y * b.y;
}

double vec_dot3D(dvec3 a, dvec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

dvec3 vec_cross(dvec3 a, dvec3 b) {
    return (dvec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

void vec_add3D(dvec3 a, dvec3 b, dvec3 *v) {
    *v = (dvec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

void vec_sub3D(dvec3 a, dvec3 b, dvec3 *v) {
    *v = (dvec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

void vec_mul3D(dvec3 a, dvec3 b, dvec3 *v) {
    *v = (dvec3){a.x * b.x, a.y * b.y, a.z * b.z};
}