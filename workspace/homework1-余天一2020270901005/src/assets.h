#pragma once
#include "render_aspect.h"
#include <vector>

namespace Assets {
const std::vector<Vertex> cube_vertices = {
    {{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}}, {{1.0, -1.0, -1.0}, {1.0, 0.0, 0.0}},
    {{1.0, 1.0, -1.0}, {1.0, 1.0, 0.0}},   {{-1.0, 1.0, -1.0}, {0.0, 1.0, 0.0}},
    {{-1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}},  {{1.0, -1.0, 1.0}, {1.0, 0.0, 1.0}},
    {{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}},    {{-1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}}};

const std::vector<Vertex> platform_vertices = {
    {{-1.0, -1.0, -1.0}, {0.5, 0.5, 0.5}}, {{1.0, -1.0, -1.0}, {0.5, 0.5, 0.5}},
    {{1.0, 1.0, -1.0}, {0.5, 0.5, 0.5}},   {{-1.0, 1.0, -1.0}, {0.5, 0.5, 0.5}},
    {{-1.0, -1.0, 1.0}, {0.5, 0.5, 0.5}},  {{1.0, -1.0, 1.0}, {0.5, 0.5, 0.5}},
    {{1.0, 1.0, 1.0}, {0.5, 0.5, 0.5}},    {{-1.0, 1.0, 1.0}, {0.5, 0.5, 0.5}}};

const std::vector<unsigned int> cube_indices = {
    1, 0, 3, 3, 2, 1, 3, 7, 6, 6, 2, 3, 7, 3, 0, 0, 4, 7,
    2, 6, 5, 5, 1, 2, 4, 5, 6, 6, 7, 4, 5, 4, 0, 0, 1, 5};

const std::vector<Vertex> plane_vertices = {
    {{-1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}, {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}, {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
};

const std::vector<unsigned int> plane_indices = {2, 1, 0, 3, 2, 0}; 

const std::vector<Vertex> line_vertices = {
    {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}, {{1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<unsigned int> line_indices = {0, 1}; 
} // namespace Assets