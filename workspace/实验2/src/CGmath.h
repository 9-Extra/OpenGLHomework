#pragma once

#include <iostream>
#include <cmath>
#include <assert.h>

inline float Q_rsqrt(float number) {
    union {
        uint32_t i;
        float y;
    } u;
    float x2;
    const float threehalfs = 1.5F;

    x2 = number * 0.5F;
    u.y = number;
    u.i = 0x5f3759df - (u.i >> 1);
    u.y = u.y * (threehalfs - (x2 * u.y * u.y)); // 1st iteration
    // y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be
    // removed

    return u.y;
}

inline float to_radian(float angle) { return angle / 180.0f * 3.1415926535f; }

struct Vector2f {
    union {
        struct {
            float x, y;
        };
        float v[2];
    };
    Vector2f() : x(0), y(0) {}
    constexpr Vector2f(float x, float y) : x(x), y(y) {}
    Vector2f operator+(const Vector2f b) { return Vector2f(x + b.x, y + b.y); }
    Vector2f operator-(const Vector2f b) { return Vector2f(x - b.x, y - b.y); }
    Vector2f operator*(const float s) { return Vector2f(x * s, y * s); }
    float squared() { return x * x + y * y; }
    float length() { return sqrtf(squared()); }
    Vector2f normalized() {
        float s = Q_rsqrt(squared());
        return Vector2f(x * s, y * s);
    }
    Vector2f rotate(float radiam) {
        return Vector2f(x * cosf(radiam) + y * sinf(radiam), x * -sinf(radiam) + y * cosf(radiam));
    }
    friend std::ostream &operator<<(std::ostream &os, const Vector2f &v) {
        os << "(" << v.x << ", " << v.y << ")";
        return os;
    }
};

struct Vector3f {
    union {
        struct {
            float x, y, z;
        };
        float v[3];
    };

    Vector3f() {}
    constexpr Vector3f(float x, float y, float z) : x(x), y(y), z(z) {}

    const float *data() const { return (float *)v; }
    constexpr float operator[](const unsigned int i) const { return v[i]; }
    constexpr Vector3f operator+(const Vector3f b) const { return Vector3f{x + b.x, y + b.y, z + b.z}; }
    constexpr Vector3f operator-(const Vector3f b) const { return Vector3f{x - b.x, y - b.y, z - b.z}; }
    constexpr Vector3f operator-() const { return Vector3f{-x, -y, -z}; }
    constexpr Vector3f operator+=(const Vector3f b) { return *this = *this + b; }
    constexpr Vector3f operator*(const float n) const { return {x * n, y * n, z * n}; }
    constexpr Vector3f operator/(const float n) const { return *this * (1.0f / n); }
    constexpr float dot(const Vector3f b) const { return x * b.x + y * b.y + z * b.z; }
    constexpr Vector3f cross(const Vector3f b) {
        return {this->y * b.z - this->z * b.y, this->z * b.x * this->x * b.z, this->x * b.y - this->y * b.x};
    }
    constexpr float square() const { return this->dot(*this); }
    Vector3f normalize() {
        float inv_sqrt = Q_rsqrt(this->square());
        return *this * inv_sqrt;
    }
    inline float length() const { return std::sqrt(square()); }
    friend std::ostream &operator<<(std::ostream &os, const Vector3f &v) {
        os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }
};
struct Vector4f {
    union {
        struct {
            float x, y, z, w;
        };
        float v[4];
    };

    Vector4f() {}
    constexpr Vector4f(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    inline Vector4f operator*(const float n) const { return {x * n, y * n, z * n, w * n}; }
    float dot(const Vector4f b) const { return x * b.x + y * b.y + z * b.z + w * b.w; }
    float square() const { return this->dot(*this); }
    Vector4f normalize() const {
        float inv_sqrt = Q_rsqrt(this->square());
        return *this * inv_sqrt;
    }

    friend std::ostream &operator<<(std::ostream &os, const Vector4f &v) {
        os << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
        return os;
    }
};
struct Matrix {
    float m[4][4];

    constexpr Matrix() : m{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}} {}
    constexpr Matrix(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20,
                     float m21, float m22, float m23, float m30, float m31, float m32, float m33)
        : m{{m00, m01, m02, m03}, {m10, m11, m12, m13}, {m20, m21, m22, m23}, {m30, m31, m32, m33}} {}

    const float *data() const { return (float *)m; }

    constexpr Matrix transpose() const {
        Matrix r;
        for (unsigned int i = 0; i < 4; i++) {
            for (unsigned int j = 0; j < 4; j++) {
                r.m[i][j] = this->m[j][i];
            }
        }
        return r;
    }
    constexpr static Matrix identity() {
        Matrix m{
            1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
        };

        return m;
    }
    constexpr Matrix operator*(const Matrix &m) const {
        Matrix r;
        for (unsigned int i = 0; i < 4; i++) {
            for (unsigned int j = 0; j < 4; j++) {
                r.m[i][j] = this->m[i][0] * m.m[0][j] + this->m[i][1] * m.m[1][j] + this->m[i][2] * m.m[2][j] +
                            this->m[i][3] * m.m[3][j];
            }
        }
        return r;
    }
    // 沿z轴顺时针旋转roll，沿x轴顺时针旋转pitch，沿y轴顺时针旋转yaw
    static Matrix rotate(float roll, float pitch, float yaw) {
        float s_p = sin(pitch), c_p = cos(pitch);
        float s_r = sin(roll), c_r = cos(roll);
        float s_y = sin(yaw), c_y = cos(yaw);
        Matrix m{-s_p * s_r * s_y + c_r * c_y,
                 -s_p * s_y * c_r - s_r * c_y,
                 -s_y * c_p,
                 0.0,
                 s_r * c_p,
                 c_p * c_r,
                 -s_p,
                 0.0,
                 s_p * s_r * c_y + s_y * c_r,
                 s_p * c_r * c_y - s_r * s_y,
                 c_p * c_y,
                 0.0,
                 0.0,
                 0.0,
                 0.0,
                 1.0};
        return m;
    }
    static Matrix rotate(Vector3f rotate) { return Matrix::rotate(rotate.x, rotate.y, rotate.z); }
    constexpr static Matrix translate(float x, float y, float z) {
        Matrix m{
            1.0, 0.0, 0.0, x, 0.0, 1.0, 0.0, y, 0.0, 0.0, 1.0, z, 0.0, 0.0, 0.0, 1.0,
        };
        return m;
    }
    constexpr static Matrix translate(Vector3f delta) { return Matrix::translate(delta.x, delta.y, delta.z); }
    constexpr static Matrix scale(float x, float y, float z) {
        Matrix m{
            x, 0.0, 0.0, 0.0, 0.0, y, 0.0, 0.0, 0.0, 0.0, z, 0.0, 0.0, 0.0, 0.0, 1.0,
        };

        return m;
    }
    constexpr static Matrix scale(Vector3f scale) { return Matrix::scale(scale.x, scale.y, scale.z); }
};

inline Matrix compute_orthogonal_matrix(float aspect, float size, float near_z, float far_z){
    assert(near_z < far_z); // 不要写反了！！！！！！！！！！
    float height = 2.0f / size;
    float width = height / aspect;
    return Matrix{
        width,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        height,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        -1.0f / (far_z - near_z),
        -near_z,
        0.0f,
        0.0f,
        0.0f,
        1.0f
    };
}

inline Matrix compute_perspective_matrix(float ratio, float fov, float near_z, float far_z) {
    assert(near_z < far_z); // 不要写反了！！！！！！！！！！
    float SinFov = std::sin(fov * 0.5f);
    float CosFov = std::cos(fov * 0.5f);

    float Height = CosFov / SinFov;
    float Width = Height / ratio;

    return Matrix{Width,
                  0.0f,
                  0.0f,
                  0.0f,
                  0.0f,
                  Height,
                  0.0f,
                  0.0f,
                  0.0f,
                  0.0f,
                  -far_z / (far_z - near_z),
                  -1.0f,
                  0.0f,
                  0.0f,
                  -far_z * near_z / (far_z - near_z),
                  0.0f}
        .transpose();
}

struct Quaternion {
    float x, y, z, w;

    static constexpr Quaternion no_rotate() { return Quaternion{0.0f, 0.0f, 0.0f, 1.0f}; }

    // 需要axis长度为1
    Quaternion from_rotation(Vector3f axis, float angle) {
        angle = angle * 0.5f;
        float sin_theta = sinf(angle), cos_theta = cosf(angle);
        return Quaternion{sin_theta * axis.x, sin_theta * axis.y, sin_theta * axis.z, cos_theta};
    }

    // 按XYZ顺序顺时针，沿X旋转的角度，沿Y旋转的角度，沿Z旋转的角度
    Quaternion from_eular(Vector3f rotate) {
        return Quaternion::from_rotation({1.0f, 0.0f, 0.0f}, rotate.x) *
               Quaternion::from_rotation({0.0f, 1.0f, 0.0f}, rotate.y) *
               Quaternion::from_rotation({0.0f, 0.0f, 1.0f}, rotate.z);
    }

    // 需要长度为1
    Matrix to_matrix() const {
        return Matrix{1.0f - 2.0f * (y * y + z * z),
                      2.0f * (x * y - w * z),
                      2.0f * (x * z + w * y),
                      0.0f,
                      2.0f * (x * y + w * z),
                      1.0f - 2.0f * (x * x + z * z),
                      2.0f * (y * z - w * x),
                      0.0f,
                      2.0f * (x * z - w * y),
                      2.0f * (y * z + w * x),
                      1.0f - 2.0f * (x * x + y * y),
                      0.0f,
                      0.0f,
                      0.0f,
                      0.0f,
                      1.0f};
    }

    Vector3f rotate_vector(Vector3f src) const {
        const Quaternion &q = *this;
        Quaternion p{src.x, src.y, src.z, 1.0f};
        Quaternion rotated = q * p * q.conjugate();
        return Vector3f{rotated.x, rotated.y, rotated.z};
    }

    Quaternion conjugate() const { return Quaternion{-x, -y, -z, w}; }

    constexpr Quaternion operator*(const Quaternion r) const {
        Vector3f qv{x, y, z}, rv{r.x, r.y, r.z};
        Vector3f v = qv.cross(rv) + qv * r.w + rv * w;

        return Quaternion{v.x, v.y, v.z, w * r.w - qv.dot(rv)};
    }
};

inline Vector3f rotation_matrix_to_eulerangles(const Matrix &R)
{
    float sy = sqrt(R.m[0][0] * R.m[0][0] + R.m[1][0] * R.m[1][0]);
    bool singular = sy < 1e-4;
    float x, y, z;
    if (!singular)
    {
        x = atan2( R.m[2][1], R.m[2][2]);
        y = atan2(-R.m[2][0], sy);
        z = atan2( R.m[1][0], R.m[0][0]);
    }
    else
    {
        x = atan2(-R.m[1][2], R.m[1][1]);
        y = atan2(-R.m[2][0], sy);
        z = 0;
    }
    return {x, -y, z};
}