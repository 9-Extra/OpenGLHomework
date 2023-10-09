#pragma once

#include <vector>

#include "RenderAspect.h"

namespace Assets {

const std::vector<Vector3f> skybox_cube_vertices = {{-1.0, -1.0, -1.0}, {1.0, -1.0, -1.0}, {1.0, 1.0, -1.0},
                                                    {-1.0, 1.0, -1.0},  {-1.0, -1.0, 1.0}, {1.0, -1.0, 1.0},
                                                    {1.0, 1.0, 1.0},    {-1.0, 1.0, 1.0}};

const std::vector<uint16_t> skybox_cube_indices = {3, 0, 1, 1, 2, 3, 6, 7, 3, 3, 2, 6, 0, 3, 7, 7, 4, 0,
                                                   5, 6, 2, 2, 1, 5, 6, 5, 4, 4, 7, 6, 0, 4, 5, 5, 1, 0};

const std::vector<Vertex> plane_vertices = {
    {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
};

const std::vector<uint16_t> plane_indices = {3, 2, 0, 2, 1, 0};
} // namespace Assets