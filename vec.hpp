#pragma once
#include <cmath>


namespace vec
{
    struct vec2 {
        double x = 0.0, y = 0.0;

        // コンストラクタを追加
        vec2() = default;
        vec2(double x, double y) : x(x), y(y) {}

        // メンバ関数化
        double length() const { // constを付けるとメンバ変数を変更しない関数だと示せる
            return std::sqrt(x*x + y*y);
        }

        double dotSelf() const {
            return x*x + y*y;
        }

        double dot(vec2 n) const {
            return x*n.x + y*n.y;
        }
        
        void normalize() {
            double len = length();
            if (len > 1e-9) { // ゼロ除算を避ける
                x /= len;
                y /= len;
            }
        }

        // 演算子オーバーロード
        vec2 operator+(const vec2& other) const {
            return vec2(x + other.x, y + other.y);
        }    
        vec2 operator-(const vec2& other) const {
            return vec2(x - other.x, y - other.y);
        }
        vec2 operator*(const vec2& other) const {
            return vec2(x * other.x, y * other.y);
        }
        vec2 operator/(const vec2& other) const {
            return vec2(x / other.x, y / other.y);
        }
    };

    void rotate(double& a, double& b, double angle){
        double oldA = a;
        a = a * cos(angle) - b * sin(angle);
        b = oldA   * sin(angle) + b * cos(angle);
    }

    struct vec3 {
        double x = 0.0, y = 0.0, z = 0.0;

        // コンストラクタを追加
        vec3() = default;
        vec3(double x, double y, double z) : x(x), y(y), z(z) {}

        // メンバ関数化
        double length() const { // constを付けるとメンバ変数を変更しない関数だと示せる
            return std::sqrt(x*x + y*y + z*z);
        }

        double dotSelf() const {
            return x*x + y*y + z*z;
        }

        double dot(vec3 n) const {
            return x*n.x + y*n.y + z*n.z;
        }

        vec3 cross(vec3 n) const {
            return vec3(y * n.z - z * n.y,
                z * n.x - x * n.z,
                x * n.y - y * n.x);
        }
        
        void normalize() {
            double len = length();
            if (len > 1e-9) { // ゼロ除算を避ける
                x /= len;
                y /= len;
                z /= len;
            }
        }

        // 演算子オーバーロード
        vec3 operator+(const vec3& other) const {
            return vec3(x + other.x, y + other.y, z + other.z);
        }    
        vec3 operator-(const vec3& other) const {
            return vec3(x - other.x, y - other.y, z - other.z);
        }
        vec3 operator*(const vec3& other) const {
            return vec3(x * other.x, y * other.y, z * other.z);
        }
        vec3 operator/(const vec3& other) const {
            return vec3(x / other.x, y / other.y, z / other.z);
        }
    };

}