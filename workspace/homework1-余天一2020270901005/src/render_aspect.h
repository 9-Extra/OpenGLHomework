#pragma once
#include "YGraphics.h"
#include <cstdint>
#include <vector>

struct BoundingBox{
    Vector3f min_corner;
    Vector3f max_corner;
};

struct PipeLineState{

};

struct RenderItem final {
    uint32_t mesh_id;
    uint32_t material_id;

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
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t indices_count;
};

struct PhongMaterial{
    unsigned int diffusion_texture;
    unsigned int normal_texture;
    float specular_factor;
};
