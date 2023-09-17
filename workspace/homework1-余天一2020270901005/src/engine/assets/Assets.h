#pragma once
#include "engine/render/RenderAspect.h"
#include <vector>

namespace Assets {
extern const std::vector<Vertex> cube_vertices;
extern const std::vector<Vertex> platform_vertices;

extern const std::vector<unsigned int> cube_indices;
extern const std::vector<Vertex> plane_vertices;

extern const std::vector<unsigned int> plane_indices;

extern const std::vector<Vertex> line_vertices;

extern const std::vector<unsigned int> line_indices;
} // namespace Assets