#pragma once

#include "CGmath.h"
// 平移旋转缩放
struct Transform {
    Vector3f position;
    Vector3f rotation;
    Vector3f scale;

    Matrix transform_matrix() const {
        return Matrix::translate(position) * Matrix::rotate(rotation) * Matrix::scale(scale);
    }
    Matrix normal_matrix() const {
        return Matrix::rotate(rotation) * Matrix::scale({1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z});
    }
};

struct Camera {
public:
    Camera() {
        position = {0.0f, 0.0f, 0.0f};
        rotation = {0.0f, 0.0f, 0.0f};
        near_z = 1.0f;
        far_z = 1000.0f;
        fov = 1.57;
    }

    Vector3f position;
    Vector3f rotation; // zxy
    float fov;
    float near_z, far_z;

    // 获取目视方向
    Vector3f get_orientation() const {
        float pitch = rotation[1];
        float yaw = rotation[2];
        return {sinf(yaw) * cosf(pitch), sinf(pitch), -cosf(pitch) * cosf(yaw)};
    }

    Vector3f get_up_direction() const {
        float sp = sinf(rotation[1]);
        float cp = cosf(rotation[1]);
        float cr = cosf(rotation[0]);
        float sr = sinf(rotation[0]);
        float sy = sinf(rotation[2]);
        float cy = cosf(rotation[2]);

        return {-sp * sy * cr - sr * cy, cp * cr, sp * cr * cy - sr * sy};
    }
};

struct Vertex {
    Vector3f position;
    Vector3f normal;
    Vector3f tangent;
    Vector2f uv;
};

struct PointLight {
    Vector3f position;
    Vector3f color;
    float factor;
};

struct DirectionalLight {
    Vector3f direction;
    Vector3f flux;
};

struct SkyBox {
    unsigned int color_texture_id;
};