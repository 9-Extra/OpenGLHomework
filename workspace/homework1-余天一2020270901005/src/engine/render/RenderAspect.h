#pragma once
#include "../utils/CGmath.h"
#include <cstdint>
#include <vector>

struct BoundingBox{
    Vector3f min_corner;
    Vector3f max_corner;
};

struct Vertex {
    Vector3f position;
    Color color;
};


struct PipeLineState{

};

struct RenderItem final {
    uint32_t mesh_id;
    uint32_t material_id;
    uint32_t topology;

    Matrix model_matrix;
};

struct PointLight{
    Vector3f position;
    Vector3f flux;
};

struct DirectionalLight{
    Vector3f direction;
    Vector3f flux;
};

struct Mesh{
    const Vertex* vertices;
    const uint32_t* indices;
    const uint32_t indices_count;
};

struct PhongMaterial{
    Vector3f diffusion;
    float specular_factor;
};

struct Camera {
public:
    Camera() {}
    
    Vector3f position;
    Vector3f rotation; // zxy
    float aspect;
    float fov, near_z, far_z;

    // 获取目视方向
    Vector3f get_orientation() const {
        float pitch = rotation[1];
        float yaw = rotation[2];
        return {sinf(yaw) * cosf(pitch), sinf(pitch), -cosf(pitch) * cosf(yaw)};
    }

    Matrix get_view_perspective_matrix() const{
        return compute_perspective_matrix(aspect, fov, near_z, far_z) *
            Matrix::rotate(rotation).transpose() *
            Matrix::translate(-position);
    }
};
