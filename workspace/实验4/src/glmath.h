#pragma once

#define _USE_MATH_DEFINES // M_PI
#include <array>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

//--------------------------
struct vec2 {
    //--------------------------
    float x, y;

    vec2(float x0 = 0, float y0 = 0) {
        x = x0;
        y = y0;
    }
    vec2 operator*(float a) const { return vec2(x * a, y * a); }
    vec2 operator/(float a) const { return vec2(x / a, y / a); }
    vec2 operator+(const vec2 &v) const { return vec2(x + v.x, y + v.y); }
    vec2 operator-(const vec2 &v) const { return vec2(x - v.x, y - v.y); }
    vec2 operator*(const vec2 &v) const { return vec2(x * v.x, y * v.y); }
    vec2 operator-() const { return vec2(-x, -y); }
    float &operator[](size_t index) { return ((float *)this)[index]; }
};

inline float dot(const vec2 &v1, const vec2 &v2) { return (v1.x * v2.x + v1.y * v2.y); }

inline float length(const vec2 &v) { return sqrtf(dot(v, v)); }

inline vec2 normalize(const vec2 &v) { return v * (1 / length(v)); }

inline vec2 operator*(float a, const vec2 &v) { return vec2(v.x * a, v.y * a); }

//--------------------------
struct vec3 {
    //--------------------------
    float x, y, z;

    vec3(float x0 = 0, float y0 = 0, float z0 = 0) {
        x = x0;
        y = y0;
        z = z0;
    }
    vec3(vec2 v) {
        x = v.x;
        y = v.y;
        z = 0;
    }

    vec3 operator*(float a) const { return vec3(x * a, y * a, z * a); }
    vec3 operator/(float a) const { return vec3(x / a, y / a, z / a); }
    vec3 operator+(const vec3 &v) const { return vec3(x + v.x, y + v.y, z + v.z); }
    vec3 operator+=(const vec3 &v) { x += v.x;y += v.y;z += v.z; return *this; }
    vec3 operator-(const vec3 &v) const { return vec3(x - v.x, y - v.y, z - v.z); }
    vec3 operator*(const vec3 &v) const { return vec3(x * v.x, y * v.y, z * v.z); }
    vec3 operator-() const { return vec3(-x, -y, -z); }
    float &operator[](size_t index) { return ((float *)this)[index]; }
    vec3 operator/(const vec3 denom) const { return vec3(this->x / denom.x, this->y / denom.y, this->z / denom.z); }

    bool operator==(const vec3 &n2) const{
        if (this->x == n2.x && this->y == n2.y && this->z == n2.z) {
            return true;
        }
        return false;
    }
};

inline float dot(const vec3 &v1, const vec3 &v2) { return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z); }

inline float length(const vec3 &v) { return sqrtf(dot(v, v)); }

inline vec3 normalize(const vec3 &v) { return v * (1 / length(v)); }

inline vec3 cross(const vec3 &v1, const vec3 &v2) {
    return vec3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}

inline vec3 operator*(float a, const vec3 &v) { return vec3(v.x * a, v.y * a, v.z * a); }

//---------------------------
struct mat4 { // row-major matrix 4x4
              //---------------------------
    float m[4][4];

public:
    mat4() {}
    mat4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21,
         float m22, float m23, float m30, float m31, float m32, float m33) {
        m[0][0] = m00;
        m[0][1] = m01;
        m[0][2] = m02;
        m[0][3] = m03;
        m[1][0] = m10;
        m[1][1] = m11;
        m[1][2] = m12;
        m[1][3] = m13;
        m[2][0] = m20;
        m[2][1] = m21;
        m[2][2] = m22;
        m[2][3] = m23;
        m[3][0] = m30;
        m[3][1] = m31;
        m[3][2] = m32;
        m[3][3] = m33;
    }

    mat4 operator*(const mat4 &right) const {
        mat4 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i][j] = 0;
                for (int k = 0; k < 4; k++)
                    result.m[i][j] += m[i][k] * right.m[k][j];
            }
        }
        return result;
    }

    vec3 operator*(const vec3 &right) const {
        return right;
    }
};

inline mat4 TranslateMatrix(vec3 t) { return mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, t.x, t.y, t.z, 1); }

inline mat4 ScaleMatrix(vec3 s) { return mat4(s.x, 0, 0, 0, 0, s.y, 0, 0, 0, 0, s.z, 0, 0, 0, 0, 1); }

inline mat4 RotationMatrix(float angle, vec3 w) {
    float c = cosf(angle), s = sinf(angle);
    w = normalize(w);
    return mat4(c * (1 - w.x * w.x) + w.x * w.x, w.x * w.y * (1 - c) + w.z * s, w.x * w.z * (1 - c) - w.y * s, 0,
                w.x * w.y * (1 - c) - w.z * s, c * (1 - w.y * w.y) + w.y * w.y, w.y * w.z * (1 - c) + w.x * s, 0,
                w.x * w.z * (1 - c) + w.y * s, w.y * w.z * (1 - c) - w.x * s, c * (1 - w.z * w.z) + w.z * w.z, 0, 0, 0,
                0, 1);
}

//--------------------------
struct vec4 {
    //--------------------------
    float x, y, z, w;

    vec4(float x0 = 0, float y0 = 0, float z0 = 0, float w0 = 0) {
        x = x0;
        y = y0;
        z = z0;
        w = w0;
    }
    vec4 operator*(float a) const { return vec4(x * a, y * a, z * a, w * a); }
    vec4 operator/(float d) const { return vec4(x / d, y / d, z / d, w / d); }
    vec4 operator+(const vec4 &v) const { return vec4(x + v.x, y + v.y, z + v.z, w + v.w); }
    vec4 operator-(const vec4 &v) const { return vec4(x - v.x, y - v.y, z - v.z, w - v.w); }
    vec4 operator*(const vec4 &v) const { return vec4(x * v.x, y * v.y, z * v.z, w * v.w); }
    void operator+=(const vec4 right) {
        x += right.x;
        y += right.y;
        z += right.z, w += right.z;
    }

    vec4 operator*(const mat4 &mat) {
        return vec4(x * mat.m[0][0] + y * mat.m[1][0] + z * mat.m[2][0] + w * mat.m[3][0],
                    x * mat.m[0][1] + y * mat.m[1][1] + z * mat.m[2][1] + w * mat.m[3][1],
                    x * mat.m[0][2] + y * mat.m[1][2] + z * mat.m[2][2] + w * mat.m[3][2],
                    x * mat.m[0][3] + y * mat.m[1][3] + z * mat.m[2][3] + w * mat.m[3][3]);
    }
};

inline float dot(const vec4 &v1, const vec4 &v2) { return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w); }

inline vec4 operator*(float a, const vec4 &v) { return vec4(v.x * a, v.y * a, v.z * a, v.w * a); }

inline vec2 solve_uv_guass(vec3 x, vec3 y, vec3 z, vec3 p) {
    vec2 uv = vec2(0.0, 0.0);

    p = p - z;
    x = x - z;
    y = y - z;

    std::array<vec3, 3> l{
        vec3(x[0], y[0], p[0]),
        vec3(x[1], y[1], p[1]),
        vec3(x[2], y[2], p[2]),
    };

    vec3 v(fabs(x[0]), fabs(x[1]), fabs(x[2]));
    auto m_i = [&] {
        if (v[0] > v[1]) {
            if (v[0] > v[2]) {
                return 0;
            } else {
                return 2;
            }
        } else if (v[1] > v[2]) {
            return 1;
        } else {
            return 2;
        }
    }();
    std::swap(l[0], l[m_i]);

    l[1] = l[1] + l[0] * (-l[1][0] / l[0][0]);
    l[2] = l[2] + l[0] * (-l[2][0] / l[0][0]);

    uv[1] = fabs(l[1][1]) > fabs(l[2][1]) ? l[1][2] / l[1][1] : l[2][2] / l[2][1];

    uv[0] = (l[0][2] - l[0][1] * uv[1]) / l[0][0];

    return uv;
}
