#pragma once

#include <GL/glew.h>
#include <GL/glut.h>
#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>

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

struct Mesh {
    unsigned int VAO_id;
    uint32_t indices_count;
};

struct TextureDesc {
    std::string path;
};

struct Texture {
    unsigned int texture_id;
};

struct CubeMap {
    unsigned int texture_id;
};

struct MaterialDesc {
    struct UniformDataDesc {
        const uint32_t binding_id;
        const uint32_t size;
        const void *data;
    };

    struct SampleData {
        uint32_t binding_id;
        std::string texture_key;
    };

    std::string shader_name;
    std::vector<UniformDataDesc> uniforms;
    std::vector<SampleData> samplers;
};

struct Material {
    struct UniformData {
        uint32_t binding_id;
        uint32_t buffer_id;
    };
    struct SampleData {
        uint32_t binding_id;
        unsigned int texture_id;
    };

    unsigned int shaderprogram_id; // 对应的shader的id
    std::vector<UniformData> uniforms;
    std::vector<SampleData> samplers;

    void bind() const {
        glUseProgram(shaderprogram_id); // 绑定此材质关联的着色器

        // 绑定材质的uniform buffer
        for (const UniformData &u : uniforms) {
            glBindBufferBase(GL_UNIFORM_BUFFER, u.binding_id, u.buffer_id);
        }
        // 绑定所有纹理
        for (const SampleData &s : samplers) {
            glActiveTexture(GL_TEXTURE0 + s.binding_id);
            glBindTexture(GL_TEXTURE_2D, s.texture_id);
        }
    }
};

struct Shader {
    unsigned int program_id;
};

// 资源管理器
class RenderReousce final {
public:
    template <class T> class ResourceContainer {
    public:
        uint32_t find(const std::string &key) const {
#ifndef NDEBUG
            if (look_up.find(key) == look_up.end()) {
                std::cerr << "Unknown key: " << key << std::endl;
                exit(-1);
            }
#endif
            return look_up.at(key);
        }

        void add(const std::string &key, T &&item) {
            if (auto it = look_up.find(key); it != look_up.end()) {
                std::cout << "Add duplicated key: " << key << std::endl;
                uint32_t id = it->second;
                container[id] = std::move(item);
            } else {
                uint32_t id = (uint32_t)container.size();
                container.emplace_back(std::move(item));
                look_up[key] = id;
            }
        }

        const T &get(uint32_t id) const { return container[id]; }

        void clear() {
            look_up.clear();
            container.clear();
        }

    private:
        std::unordered_map<std::string, uint32_t> look_up;
        std::vector<T> container;
    };

    ResourceContainer<Mesh> meshes;
    ResourceContainer<Material> materials;
    ResourceContainer<Shader> shaders;
    ResourceContainer<Texture> textures;
    ResourceContainer<CubeMap> cubemaps;

    std::vector<std::function<void()>> deconstructors;

    void add_mesh(const std::string &key, const Vertex *vertices, size_t vertex_count, const uint16_t *const indices,
                  size_t indices_count);
    void add_mesh(const std::string &key, const std::vector<Vertex> &vertices, const std::vector<uint16_t> &indices) {
        add_mesh(key, vertices.data(), vertices.size(), indices.data(), indices.size());
    }
    void add_material(const std::string &key, const MaterialDesc &desc);
    void add_texture(const std::string &key, const std::string &image_path, bool is_normal = false);
    void add_cubemap(const std::string &key, const std::string &image_px, const std::string &image_nx,
                     const std::string &image_py, const std::string &image_ny, const std::string &image_pz,
                     const std::string &image_nz);
    void add_shader(const std::string &key, const std::string &vs_path, const std::string &ps_path);
    void load_gltf(const std::string &base_key, const std::string &path);
    void load_json(const std::string &path);
    void clear();
};

extern RenderReousce resources;