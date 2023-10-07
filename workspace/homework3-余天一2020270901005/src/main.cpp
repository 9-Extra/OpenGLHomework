#include <GL/glew.h>
#include <GL/glut.h>
#include <algorithm>
#include <array>
#include <assert.h>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "CGmath.h"
#include "Sjson.h"

#include "winapi.h"
#include <FreeImage.h>

// Mingw的ifstream不知道为什么导致了崩溃，手动实现文件读取
std::string read_whole_file(const std::string &path) {
    HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open file: " << path << std::endl;
        exit(-1);
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(handle, &size)) {
        std::cerr << "Failed to get file size\n";
        exit(-1);
    }
    std::string chunk;
    chunk.resize(size.QuadPart);

    char *ptr = (char *)chunk.data();
    char *end = ptr + chunk.size();
    DWORD read = 0;
    while (ptr < end) {
        DWORD to_read = (DWORD)std::min<size_t>(std::numeric_limits<DWORD>::max(), end - ptr);
        if (!ReadFile(handle, ptr, to_read, &read, NULL)) {
            std::cerr << "Failed to read file\n";
            exit(-1);
        }
        ptr += read;
    }

    CloseHandle(handle);

    return chunk;
}

inline void _check_error(const std::string &file, size_t line) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cerr << "GL error 0x" << error << ": " << gluErrorString(error) << std::endl;
        std::cerr << "At: " << file << ':' << line << std::endl;
    }
}

#define checkError() _check_error(__FILE__, __LINE__)

class Clock {
private:
    std::chrono::time_point<std::chrono::steady_clock> now;
    float delta;

public:
    Clock() : now(std::chrono::steady_clock::now()) {}

    void update() {
        using namespace std::chrono;
        delta = duration_cast<duration<float, std::milli>>(steady_clock::now() - now).count();
        now = std::chrono::steady_clock::now();
    }

    float get_delta() const { return delta; } // 返回该帧相对上一帧过去的以float表示的毫秒数
    float get_current_delta() const // 获得调用此函数的时间相对上一帧过去的以float表示的毫秒数
    {
        using namespace std::chrono;
        return duration_cast<duration<float, std::milli>>(steady_clock::now() - now).count();
    }
};

#define IS_KEYDOWN(key) GetKeyState(key) & 0x8000

class InputHandler {
public:
    InputHandler() { clear_mouse_state(); }

    inline Vector2f get_mouse_position() const { return mouse_position; }
    inline Vector2f get_mouse_move() const { return mouse_delta; }
    inline bool is_left_button_down() const { return mouse_state & (1 << mouse_button_map(GLUT_LEFT_BUTTON)); }
    inline bool is_right_button_down() const { return mouse_state & (1 << mouse_button_map(GLUT_RIGHT_BUTTON)); }
    inline bool is_middle_button_down() const { return mouse_state & (1 << mouse_button_map(GLUT_MIDDLE_BUTTON)); }

private:
    friend void handle_mouse_click(int button, int state, int x, int y);
    friend void handle_mouse_move(int x, int y);
    Vector2f mouse_position;
    Vector2f mouse_delta;
    uint32_t mouse_state;

    // 这个需要每一帧清理
    friend void loop_func();
    void clear_mouse_move() { mouse_delta = Vector2f(0.0f, 0.0f); }
    void clear_mouse_state() {
        mouse_position = Vector2f(0.5f, 0.5f);
        mouse_delta = Vector2f(0.0f, 0.0f);
        mouse_state = 0;
    }
    static constexpr unsigned char mouse_button_map(int button) {
        switch (button) {
        case GLUT_LEFT_BUTTON:
            return 0;
        case GLUT_MIDDLE_BUTTON:
            return 1;
        case GLUT_RIGHT_BUTTON:
            return 2;
        }
        return 3;
    }
} input;

void handle_mouse_move(int x, int y) {
    input.mouse_delta = input.mouse_delta + Vector2f(x, y) - input.mouse_position;
    input.mouse_position = Vector2f(x, y);
}
void handle_mouse_click(int button, int state, int x, int y) {
    handle_mouse_move(x, y);

    if (state == GLUT_DOWN) {
        input.mouse_state |= 1 << InputHandler::mouse_button_map(button);
    } else {
        input.mouse_state &= 0 << InputHandler::mouse_button_map(button);
    }
}

unsigned int complie_shader(const char *const src, unsigned int shader_type) {
    unsigned int id = glCreateShader(shader_type);

    glShaderSource(id, 1, &src, NULL);
    glCompileShader(id);

    int success;
    char infoLog[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(id, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        exit(-1);
    }

    return id;
}

unsigned int complie_shader_program(const std::string &vs_path, const std::string &ps_path) {
    std::string vs_src = read_whole_file(vs_path);
    std::string ps_src = read_whole_file(ps_path);

    unsigned int vs = complie_shader(vs_src.c_str(), GL_VERTEX_SHADER);
    unsigned int ps = complie_shader(ps_src.c_str(), GL_FRAGMENT_SHADER);

    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, ps);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << infoLog << std::endl;
        exit(-1);
    }

    glDeleteShader(vs);
    glDeleteShader(ps);

    return shaderProgram;
}

// 加载图像并预先进行反向gamma矫正
FIBITMAP *freeimage_load_and_convert_image(const std::string &image_path, bool is_normal = false) {
    FIBITMAP *pImage_ori = FreeImage_Load(FreeImage_GetFileType(image_path.c_str(), 0), image_path.c_str());
    if (pImage_ori == nullptr) {
        std::cerr << "Failed to load image: " << image_path << std::endl;
        exit(-1);
    }
    FIBITMAP *pImage = FreeImage_ConvertTo24Bits(pImage_ori);
    FreeImage_FlipVertical(pImage); // 翻转，适应opengl的方向
    if (!is_normal) {
        FreeImage_AdjustGamma(pImage, 1 / 2.2);
    }
    FreeImage_Unload(pImage_ori);

    return pImage;
}

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
    bool enabled = false;
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
                  size_t indices_count) {
        std::cout << "Load mesh: " << key << std::endl;
        unsigned int vao_id, ibo_id, vbo_id;
        glGenVertexArrays(1, &vao_id);
        glGenBuffers(1, &ibo_id);
        glGenBuffers(1, &vbo_id);

        glBindVertexArray(vao_id);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Vertex), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_count * sizeof(uint16_t), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, tangent));
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);

        checkError();

        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        meshes.add(key, Mesh{vao_id, (uint32_t)indices_count});

        deconstructors.emplace_back([vao_id, vbo_id, ibo_id]() {
            glDeleteVertexArrays(1, &vao_id);
            glDeleteBuffers(1, &vbo_id);
            glDeleteBuffers(1, &ibo_id);
            checkError();
        });
    }

    void add_mesh(const std::string &key, const std::vector<Vertex> &vertices, const std::vector<uint16_t> &indices) {
        add_mesh(key, vertices.data(), vertices.size(), indices.data(), indices.size());
    }

    void add_material(const std::string &key, const MaterialDesc &desc) {
        std::cout << "Load material: " << key << std::endl;
        Material mat;
        mat.shaderprogram_id = shaders.get(shaders.find(desc.shader_name)).program_id;
        for (const auto &u : desc.uniforms) {
            unsigned int buffer_id;
            glGenBuffers(1, &buffer_id);
            glBindBuffer(GL_UNIFORM_BUFFER, buffer_id);
            glBufferData(GL_UNIFORM_BUFFER, u.size, u.data, GL_STATIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            mat.uniforms.emplace_back(Material::UniformData{u.binding_id, buffer_id});

            deconstructors.emplace_back([buffer_id]() { glDeleteBuffers(1, &buffer_id); });
        }
        for (const auto &s : desc.samplers) {
            mat.samplers.emplace_back(
                Material::SampleData{s.binding_id, textures.get(textures.find(s.texture_key)).texture_id});
        }

        materials.add(key, std::move(mat));
    }

    void add_texture(const std::string &key, const std::string &image_path, bool is_normal = false) {
        std::cout << "Load texture: " << key << std::endl;

        FIBITMAP *pImage = freeimage_load_and_convert_image(image_path, is_normal);

        unsigned int nWidth = FreeImage_GetWidth(pImage);
        unsigned int nHeight = FreeImage_GetHeight(pImage);

        unsigned int texture_id;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, nWidth, nHeight, 0, GL_BGR, GL_UNSIGNED_BYTE,
                     (void *)FreeImage_GetBits(pImage));
        glGenerateTextureMipmap(texture_id);

        checkError();

        FreeImage_Unload(pImage);

        glBindTexture(GL_TEXTURE_2D, 0);

        textures.add(key, Texture{texture_id});

        deconstructors.emplace_back([texture_id]() { glDeleteTextures(1, &texture_id); });
    }

    void add_cubemap(const std::string &key, const std::string &image_px, const std::string &image_nx,
                     const std::string &image_py, const std::string &image_ny, const std::string &image_pz,
                     const std::string &image_nz) {
        std::cout << "Load skybox: " << key << std::endl;

        // GL_TEXTURE_CUBE_MAP_POSITIVE_X
        // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
        // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
        // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
        // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
        // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
        std::array<const std::string *, 6> textures_faces{
            &image_px, &image_nx, &image_py, &image_ny, &image_pz, &image_nz,
        };

        unsigned int texture_id;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        for (unsigned int i = 0; i < textures_faces.size(); i++) {
            FIBITMAP *pImage = freeimage_load_and_convert_image(*textures_faces[i]);

            unsigned int nWidth = FreeImage_GetWidth(pImage);
            unsigned int nHeight = FreeImage_GetHeight(pImage);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, nWidth, nHeight, 0, GL_BGR, GL_UNSIGNED_BYTE,
                         (void *)FreeImage_GetBits(pImage));

            FreeImage_Unload(pImage);
        }
        glGenerateTextureMipmap(texture_id);

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        cubemaps.add(key, CubeMap{texture_id});

        deconstructors.emplace_back([texture_id] { glDeleteTextures(1, &texture_id); });
    }

    void add_shader(const std::string &key, const std::string &vs_path, const std::string &ps_path) {
        std::cout << "Load shader: " << key << std::endl;
        unsigned int program_id = complie_shader_program(vs_path, ps_path);

        shaders.add(key, Shader{program_id});

        deconstructors.emplace_back([program_id]() { glDeleteProgram(program_id); });
    }

    void load_gltf(const std::string &base_key, const std::string &path) {
        std::string root = path;
        for (; !(root.empty() || root.back() == '/' || root.back() == '\\'); root.pop_back())
            ;
        using Json = SimpleJson::JsonObject;
        Json json = SimpleJson::parse_file(path);

        struct Buffer {
            char *ptr = nullptr;
            size_t len;
            Buffer(size_t len) : ptr(new char[len]), len(len) {}
            ~Buffer() {
                if (ptr != nullptr) {
                    delete[] ptr;
                }
            }
        };
        std::vector<Buffer> buffers;
        if (json.has("buffers")) {
            for (const Json &buffer : json["buffers"].get_list()) {
                std::string bin_path = root + buffer["uri"].get_string();
                Buffer &b = buffers.emplace_back((size_t)buffer["byteLength"].get_number());
                FILE *read;
                if (fopen_s(&read, bin_path.c_str(), "rb") != 0) {
                    std::cerr << "Falied to read file: " << bin_path << std::endl;
                    exit(-1);
                }
                fread((char *)b.ptr, 1, b.len, read);
                fclose(read);
            }
        }
        auto get_buffer = [&](uint64_t accessor_id) -> const Json & {
            const Json &buffer_view = json["bufferViews"][json["accessors"][accessor_id]["bufferView"].get_uint()];
            return buffer_view;
        };
        // 加载网格
        if (json.has("meshes")) {
            for (const Json &mesh : json["meshes"].get_list()) {
                const std::string &key = base_key + '.' + mesh["name"].get_string();
                const Json &primitive = mesh["primitives"][0];
                const Json &indices_buffer = get_buffer(primitive["indices"].get_uint());
                const Json &position_buffer = get_buffer(primitive["attributes"]["POSITION"].get_uint());
                const Json &normal_buffer = get_buffer(primitive["attributes"]["NORMAL"].get_uint());
                const Json &uv_buffer = get_buffer(primitive["attributes"]["TEXCOORD_0"].get_uint());
                const Json &tangent_buffer = get_buffer(primitive["attributes"]["TANGENT"].get_uint());

                uint32_t indices_count = json["accessors"][primitive["indices"].get_uint()]["count"].get_uint();
                uint16_t *indices_ptr = (uint16_t *)((char *)buffers[indices_buffer["buffer"].get_uint()].ptr +
                                                     indices_buffer["byteOffset"].get_uint());

                uint32_t vertex_count = position_buffer["byteLength"].get_uint() / sizeof(Vector3f);
                uint32_t normal_count = normal_buffer["byteLength"].get_uint() / sizeof(Vector3f);
                uint32_t uv_count = uv_buffer["byteLength"].get_uint() / sizeof(Vector2f);
                uint32_t tangent_count = tangent_buffer["byteLength"].get_uint() / sizeof(Vector4f);
                assert(vertex_count == normal_count && vertex_count == uv_count && vertex_count == tangent_count);
                Vector3f *pos = (Vector3f *)((char *)buffers[position_buffer["buffer"].get_uint()].ptr +
                                             position_buffer["byteOffset"].get_uint());
                Vector3f *normal = (Vector3f *)((char *)buffers[normal_buffer["buffer"].get_uint()].ptr +
                                                normal_buffer["byteOffset"].get_uint());
                Vector2f *uv = (Vector2f *)((char *)buffers[uv_buffer["buffer"].get_uint()].ptr +
                                            uv_buffer["byteOffset"].get_uint());
                Vector4f *tangent = (Vector4f *)((char *)buffers[tangent_buffer["buffer"].get_uint()].ptr +
                                                 tangent_buffer["byteOffset"].get_uint());

                std::vector<Vertex> vertices(vertex_count);
                for (uint32_t i = 0; i < vertex_count; i++) {
                    // tangent的第四个分量是用来根据平台决定手性的，在opengl中始终应该取1
                    Vector3f tang = Vector3f(tangent[i].x, tangent[i].y, tangent[i].z);
                    vertices[i] = {pos[i], normal[i], tang, uv[i]};
                }

                add_mesh(key, vertices.data(), vertices.size(), indices_ptr, indices_count);
            }
        }
        // 加载纹理（在加载材质时加载需要的纹理）
        auto load_texture = [&](size_t index, bool is_normal = false) -> std::string {
            const Json &texture = json["images"][index];
            const std::string key = base_key + '.' + texture["name"].get_string();
            add_texture(key, root + texture["uri"].get_string(), is_normal);
            return key;
        };

        // 加载材质
        if (json.has("materials")) {
            for (const Json &material : json["materials"].get_list()) {
                const std::string &key = base_key + '.' + material["name"].get_string();

                std::string normal_texture = "default_normal";
                if (material.has("normalTexture")) {
                    normal_texture = load_texture(material["normalTexture"]["index"].get_uint(), true);
                }
                const std::string basecolor_texture =
                    load_texture(material["pbrMetallicRoughness"]["baseColorTexture"]["index"].get_uint());
                std::string metallic_roughness_texture = "white";
                if (material["pbrMetallicRoughness"].has("metallicRoughnessTexture")) {
                    metallic_roughness_texture =
                        load_texture(material["pbrMetallicRoughness"]["metallicRoughnessTexture"]["index"].get_uint());
                }

                float metallicFactor = 1.0f;
                if (material["pbrMetallicRoughness"].has("metallicFactor")) {
                    metallicFactor = (float)material["pbrMetallicRoughness"]["metallicFactor"].get_number();
                }
                float roughnessFactor = 1.0f;
                if (material["pbrMetallicRoughness"].has("roughnessFactor")) {
                    roughnessFactor = (float)material["pbrMetallicRoughness"]["roughnessFactor"].get_number();
                }
                float uniform_data[2] = {metallicFactor, roughnessFactor};

                MaterialDesc desc = {"pbr",
                                     {{2, sizeof(float) * 2, &uniform_data}},
                                     {{0, basecolor_texture}, {1, normal_texture}, {2, metallic_roughness_texture}}};

                add_material(key, desc);
            }
        }
    }

    void load_json(const std::string &path) {
        SimpleJson::JsonObject json = SimpleJson::parse_file(path);
        std::string base_dir;
        if (size_t it = path.find_last_of("/\\"); it != std::string::npos) {
            base_dir = path.substr(0, it + 1); // 包含'/'
        } else {
            base_dir = ""; // 可能在同一目录下
        }

        if (json.has("shader")) {
            for (const auto &[key, shader_desc] : json["shader"].get_map()) {
                add_shader(key, base_dir + shader_desc["vs_path"].get_string(),
                           base_dir + shader_desc["ps_path"].get_string());
            }
        }

        if (json.has("texture")) {
            for (const auto &[key, texture_desc] : json["texture"].get_map()) {
                bool is_normal = texture_desc.has("is_normal") && texture_desc["is_normal"].get_bool();
                add_texture(key, base_dir + texture_desc["image"].get_string(), is_normal);
            }
        }

        if (json.has("cubemap")) {
            for (const auto &[key, cubemap_desc] : json["cubemap"].get_map()) {
                add_cubemap(key, base_dir + cubemap_desc["px"].get_string(), base_dir + cubemap_desc["nx"].get_string(),
                            base_dir + cubemap_desc["py"].get_string(), base_dir + cubemap_desc["ny"].get_string(),
                            base_dir + cubemap_desc["pz"].get_string(), base_dir + cubemap_desc["nz"].get_string());
            }
        }

        if (json.has("gltf")) {
            for (const auto &[key, gltf_desc] : json["gltf"].get_map()) {
                load_gltf(key, base_dir + gltf_desc["path"].get_string());
            }
        }
    }

    void clear() {
        meshes.clear();
        materials.clear();
        shaders.clear();
        cubemaps.clear();
        textures.clear();
        for (auto &de : deconstructors) {
            de();
        }
        deconstructors.clear();
    }
} resources;

struct GameObjectPart {
    uint32_t mesh_id;
    uint32_t material_id;
    uint32_t topology;
    Matrix model_matrix;
    Matrix normal_matrix;

    GameObjectPart(const std::string &mesh_name, const std::string &material_name,
                   const uint32_t topology = GL_TRIANGLES,
                   const Transform &transform = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}})
        : mesh_id(resources.meshes.find(mesh_name)), material_id(resources.materials.find(material_name)),
          topology(topology), model_matrix(transform.transform_matrix()), normal_matrix(transform.normal_matrix()) {}

    Matrix base_transform;
    Matrix base_normal_matrix;
};

struct GObjectDesc {
    Transform transform;
    std::vector<GameObjectPart> parts;
};

class GObject final : public std::enable_shared_from_this<GObject> {
public:
    std::string name;
    Transform transform;

    Matrix relate_model_matrix;
    Matrix relate_normal_matrix;
    bool is_relat_dirty = true;

    GObject(const std::string name = "") : name(name){};
    GObject(GObjectDesc &&desc, const std::string name = "")
        : name(name), transform(desc.transform), parts(std::move(desc.parts)){};

    void add_part(const GameObjectPart &part) {
        GameObjectPart &p = parts.emplace_back(part);
        p.base_transform = relate_model_matrix * p.model_matrix;
        p.base_normal_matrix = transform.normal_matrix() * p.normal_matrix;
    }

    ~GObject() {
        assert(!parent.lock()); // 必须没有父节点
    };

    void set_transform(const Transform &transform) {
        this->transform = transform;
        is_relat_dirty = true;
    }

    const std::vector<std::shared_ptr<GObject>> &get_children() const { return children; }
    std::shared_ptr<GObject> get_child_by_name(const std::string name) {
        if (name.empty())
            return nullptr;
        for (auto &child : children) {
            if (child->name == name) {
                return child;
            }
        }
        return nullptr;
    }

    void attach_child(std::shared_ptr<GObject> child) {
        children.push_back(child);
        child->parent = weak_from_this();
    }

    void remove_child(GObject *child) {
        auto it =
            std::find_if(children.begin(), children.end(), [child](const auto &c) -> bool { return c.get() == child; });
        if (it != children.end()) {
            (*it)->parent.reset();
            children.erase(it);
        } else {
            assert(false); // 试图移除不存在的子节点
        }
    }

    bool has_parent() const { return !parent.expired(); }

    std::weak_ptr<GObject> get_parent() { return parent; }

    void attach_parent(GObject *new_parent) {
        if (new_parent == parent.lock().get())
            return;
        if (auto old_parent = parent.lock(); old_parent) {
            old_parent->remove_child(this);
        }
        if (new_parent != nullptr) {
            new_parent->attach_child(shared_from_this());
        }
    }

private:
    friend class World;
    std::vector<GameObjectPart> parts;
    std::weak_ptr<GObject> parent;
    std::vector<std::shared_ptr<GObject>> children;
};

// 一个可写的uniform buuffer对象的封装
template <class T> struct WritableUniformBuffer {
    // 在初始化opengl后才能初始化
    void init() {
        assert(id == 0);
        glGenBuffers(1, &id);
        glBindBuffer(GL_UNIFORM_BUFFER, id);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(T), nullptr, GL_DYNAMIC_DRAW);
    }

    T *map() {
        void *ptr = glMapNamedBuffer(id, GL_WRITE_ONLY);
        assert(ptr != nullptr);
        return (T *)ptr;
    }
    void unmap() {
        bool ret = glUnmapNamedBuffer(id);
        assert(ret);
    }

    unsigned int get_id() const { return id; }

    void clear() {
        glDeleteBuffers(1, &id);
        id = 0;
    }

private:
    unsigned int id = 0;
};

struct PointLightData final {
    alignas(16) Vector3f position;  // 0
    alignas(16) Vector3f intensity; // 4
};

#define POINTLIGNT_MAX 8
struct PerFrameData final {
    Matrix view_perspective_matrix;
    alignas(16) Vector3f ambient_light; // 3 * 4 + 4
    alignas(16) Vector3f camera_position;
    alignas(4) float fog_min_distance;
    alignas(4) float fog_density;
    alignas(4) uint32_t pointlight_num; // 4
    PointLightData pointlight_list[POINTLIGNT_MAX];
};

struct PerObjectData {
    Matrix model_matrix;
    Matrix normal_matrix;
};
struct RenderInfo {
    uint32_t tick_count = 0;

    struct Viewport {
        GLint x;
        GLint y;
        GLsizei width;
        GLsizei height;
    } main_viewport;

    WritableUniformBuffer<PerFrameData> per_frame_uniform;   // 用于一般渲染每帧变化的数据
    WritableUniformBuffer<PerObjectData> per_object_uniform; // 用于一般渲染每个物体不同的数据

    unsigned int framebuffer_pickup;
    unsigned int framebuffer_pickup_rbo;

    void init() {
        per_frame_uniform.init();
        per_object_uniform.init();

        // 初始化pickup用的framebuffer
        glGenRenderbuffers(1, &framebuffer_pickup_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, framebuffer_pickup_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // 完整的初始化推迟到set_viewport
        // glNamedRenderbufferStorage(framebuffer_pickup_rbo, GL_R32UI, main_viewport.width, main_viewport.height);

        checkError();

        glGenFramebuffers(1, &framebuffer_pickup);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_pickup);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, framebuffer_pickup_rbo);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        checkError();
    }

    void clear() {
        per_frame_uniform.clear();
        per_object_uniform.clear();

        glDeleteRenderbuffers(1, &framebuffer_pickup_rbo);
        glDeleteFramebuffers(1, &framebuffer_pickup);
    }

    void set_viewport(GLint x, GLint y, GLsizei width, GLsizei height);
} render_info;

class Pass{
public:
    void accept(GameObjectPart* part){
        parts.push_back(part);
    }

    virtual void run() = 0;
protected:
    std::vector<GameObjectPart*> parts;
    void reset() {
        parts.clear();
    }

    friend class Renderer;
};

class LamertainPass : public Pass{
public:
    virtual void run() override;
};

class SkyBoxPass : public Pass{
public:
    SkyBoxPass(){
        shader_program_id = resources.shaders.get(resources.shaders.find("skybox")).program_id;
        mesh_id = resources.meshes.find("skybox_cube");
    }
    void set_skybox(unsigned int texture_id) {
        skybox_texture_id = texture_id;
    }
    virtual void run() override;
private:
    //每帧更新
    unsigned int skybox_texture_id = 0;
    //初始化时设定
    unsigned int shader_program_id;
    uint32_t mesh_id;
};

class Renderer final{
public:
    std::unique_ptr<LamertainPass> lamertain_pass;
    std::unique_ptr<SkyBoxPass> skybox_pass;
    void init() {
        lamertain_pass = std::make_unique<LamertainPass>();
        skybox_pass = std::make_unique<SkyBoxPass>();
    }

    void render(){
        lamertain_pass->run();
        skybox_pass->run();

        lamertain_pass->reset();
        skybox_pass->reset();

        glutSwapBuffers(); // 渲染完毕，交换缓冲区，显示新一帧的画面
        checkError();
    }
} renderer;

struct Camera {
public:
    Camera() {
        position = {0.0f, 0.0f, 0.0f};
        rotation = {0.0f, 0.0f, 0.0f};
        fov = 1.57f;
        near_z = 1.0f;
        far_z = 1000.0f;
    }

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

    Vector3f get_up_direction() const {
        float sp = sinf(rotation[1]);
        float cp = cosf(rotation[1]);
        float cr = cosf(rotation[0]);
        float sr = sinf(rotation[0]);
        float sy = sinf(rotation[2]);
        float cy = cosf(rotation[2]);

        return {-sp * sy * cr - sr * cy, cp * cr, sp * cr * cy - sr * sy};
    }

    // 用于一般物体的变换矩阵
    Matrix get_view_perspective_matrix() const {
        return compute_perspective_matrix(aspect, fov, near_z, far_z) * Matrix::rotate(rotation).transpose() *
               Matrix::translate(-position);
    }
    // 用于天空盒的变换矩阵
    Matrix get_skybox_view_perspective_matrix() const {
        return compute_perspective_matrix(aspect, fov, near_z, far_z) * Matrix::rotate(rotation).transpose() *
               Matrix::scale({far_z / 2, far_z / 2, far_z / 2});
    }
};

struct SkyBox {
    unsigned int color_texture_id;
};

class ISystem {
public:
    bool enable;

    ISystem(const std::string &name, bool enable = true) : enable(enable), name(name) {}
    const std::string &get_name() const { return name; }

    virtual void on_attach(){};
    virtual void tick() = 0;

    virtual ~ISystem() = default;

private:
    const std::string name;
};

#define POINTLIGNT_MAX 8

class World {
public:
    Camera camera;
    Clock clock;

    Vector3f ambient_light = {0.02f, 0.02f, 0.02f}; // 环境光
    PointLight pointlights[POINTLIGNT_MAX];         // 点光源
    bool is_light_dirty = true;

    float fog_min_distance = 5.0f; // 雾开始的距离
    float fog_density = 0.001f;    // 雾强度

    World() {
        root = std::make_shared<GObject>(GObjectDesc{{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}, {}},
                                         "root");
    }

    std::shared_ptr<GObject> get_root() { return root; }

    void register_system(ISystem *system) {
        assert(system != nullptr && systems.find(system->get_name()) == systems.end());
        systems.emplace(system->get_name(), system);
        system->on_attach();
    }

    void set_skybox(const std::string &color_cubemap_key) {
        skybox.color_texture_id = resources.cubemaps.get(resources.cubemaps.find(color_cubemap_key)).texture_id;
    }

    ISystem *get_system(const std::string &name) { return systems.at(name).get(); }

    void remove_system(const std::string &name) { systems.erase(name); }

    void walk_gobject(GObject *root, uint32_t dirty_flags);
    void tick();
    // 获取屏幕上的一点对应的射线方向
    Vector3f get_screen_point_oritation(Vector2f screen_xy) const {
        float w_w = 400;
        float w_h = 300;
        Vector3f camera_up = camera.get_up_direction();
        Vector3f camera_forward = camera.get_orientation();
        Vector3f camera_right = camera_forward.cross(camera_up);
        float tan_fov = std::tan(camera.fov / 2);

        Vector3f ori = camera.get_orientation() +
                       camera_right * ((screen_xy.x / w_w - 0.5f) * tan_fov * w_w / w_h * 2.0f) +
                       camera_up * ((0.5f - screen_xy.y / w_h) * tan_fov * 2.0f);

        return ori.normalize();
    }

    std::shared_ptr<GObject> pick_up_object(Vector2f screen_xy) const { return nullptr; }

private:
    std::shared_ptr<GObject> root;
    std::unordered_map<std::string, std::unique_ptr<ISystem>> systems;
    SkyBox skybox;
} world;

void RenderInfo::set_viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    main_viewport = {x, y, width, height};
    world.camera.aspect = float(width) / float(height);
    glNamedRenderbufferStorage(framebuffer_pickup_rbo, GL_R32UI, main_viewport.width, main_viewport.height);
    checkError();
    assert(glCheckNamedFramebufferStatus(framebuffer_pickup, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

void setup_opengl() {
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
    }

    const char *vendorName = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    std::cout << vendorName << ": " << version << std::endl;

    if (!GL_EXT_gpu_shader4) {
        std::cerr << "不兼容拓展" << std::endl;
    }

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // 逆时针的面为正面
    glDepthFunc(GL_LESS);

    checkError();
}

//一般物体渲染
void LamertainPass::run() {
    // 初始化渲染配置
    glEnable(GL_CULL_FACE);  // 启用面剔除
    glEnable(GL_DEPTH_TEST); // 启用深度测试
    glDrawBuffer(GL_BACK);   // 渲染到后缓冲区
    RenderInfo::Viewport &v = render_info.main_viewport;
    glViewport(v.x, v.y, v.width, v.height);
    // 清除旧画面
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    assert(world.fog_density >= 0.0f);

    // 填充per_frame uniform数据
    auto data = render_info.per_frame_uniform.map();
    // 透视投影矩阵
    data->view_perspective_matrix = world.camera.get_view_perspective_matrix().transpose();
    // 相机位置
    data->camera_position = world.camera.position;
    // 雾参数
    data->fog_density = world.fog_density;
    data->fog_min_distance = world.fog_min_distance;
    // 灯光参数
    if (world.is_light_dirty) {
        data->ambient_light = world.ambient_light;
        uint32_t count = 0;
        for (const PointLight &l : world.pointlights) {
            if (l.enabled) {
                PointLightData &d = data->pointlight_list[count];
                d.position = l.position;
                d.intensity = l.color * l.factor;
                count++;
            }
        }
        data->pointlight_num = count;
        world.is_light_dirty = false;

        // std::cout << "Update light: count:" << count << std::endl;
    }
    // 填充结束
    render_info.per_frame_uniform.unmap();

    // 绑定per_frame和per_object uniform buffer
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, render_info.per_frame_uniform.get_id());
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, render_info.per_object_uniform.get_id());

    // 遍历所有part，绘制每一个part
    for (const GameObjectPart *p : parts) {
        // 填充per_object uniform buffer
        auto data = render_info.per_object_uniform.map();
        data->model_matrix = p->base_transform.transpose();      // 变换矩阵
        data->normal_matrix = p->base_normal_matrix.transpose(); // 法线变换矩阵
        render_info.per_object_uniform.unmap();

        // 查找并绑定材质
        const Material &material = resources.materials.get(p->material_id);
        material.bind();

        const Mesh &mesh = resources.meshes.get(p->mesh_id); // 网格数据

        glBindVertexArray(mesh.VAO_id);                                        // 绑定网格
        glDrawElements(p->topology, mesh.indices_count, GL_UNSIGNED_SHORT, 0); // 绘制
        checkError();
    }
}

#define SKYBOX_COLOR_BINDING 5

// 渲染天空盒
void SkyBoxPass::run() {
    glEnable(GL_CULL_FACE);  // 启用面剔除
    glEnable(GL_DEPTH_TEST); // 启用深度测试
    glDrawBuffer(GL_BACK);   // 渲染到后缓冲区

    // 填充天空盒需要的参数（透视投影矩阵）
    auto data = render_info.per_frame_uniform.map();
    data->view_perspective_matrix = world.camera.get_skybox_view_perspective_matrix().transpose();
    render_info.per_frame_uniform.unmap();

    // 绑定天空盒专用着色器
    glUseProgram(shader_program_id);
    // 绑定天空盒纹理
    glActiveTexture(GL_TEXTURE0 + SKYBOX_COLOR_BINDING);
    assert(skybox_texture_id != 0); // 天空纹理必须存在
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture_id);
    // 获取天空盒的网格（向内的Cube）
    const Mesh &mesh = resources.meshes.get(mesh_id);

    glBindVertexArray(mesh.VAO_id);                                         // 绑定网格
    glDrawElements(GL_TRIANGLES, mesh.indices_count, GL_UNSIGNED_SHORT, 0); // 绘制
    checkError();
}

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

void init_resource() {
    // 从json加载大部分的资源
    resources.load_json("assets/resources.json");
    // 材质资源比较特殊，暂时通过硬编码加载
    {
        // 平滑着色材质
        Vector3f color_while{1.0f, 1.0f, 1.0f};
        resources.add_material("wood_flat",
                               MaterialDesc{"flat", {{2, sizeof(Vector3f), &color_while}}, {{3, "wood_diffusion"}}});
    }

    {
        // 单一颜色材质
        MaterialDesc green_material_desc;
        Vector3f color_green{0.0f, 1.0f, 0.0f};
        green_material_desc.shader_name = "single_color";
        green_material_desc.uniforms.emplace_back(
            MaterialDesc::UniformDataDesc{2, sizeof(Vector3f), color_green.data()});
        resources.add_material("default", green_material_desc);
    }
    // 部分硬编码的mesh
    resources.add_mesh("default", {}, {});
    resources.add_mesh("plane", Assets::plane_vertices, Assets::plane_indices);

    // 添加天空盒的mesh，因为格式不一样所以单独处理
    {
        unsigned int vao_id, ibo_id, vbo_id;
        glGenVertexArrays(1, &vao_id);
        glGenBuffers(1, &ibo_id);
        glGenBuffers(1, &vbo_id);

        glBindVertexArray(vao_id);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
        glBufferData(GL_ARRAY_BUFFER, Assets::skybox_cube_vertices.size() * sizeof(Vector3f),
                     Assets::skybox_cube_vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, Assets::skybox_cube_indices.size() * sizeof(uint16_t),
                     Assets::skybox_cube_indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3f), (void *)0);
        glEnableVertexAttribArray(0);

        checkError();

        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        resources.meshes.add("skybox_cube", Mesh{vao_id, (uint32_t)Assets::skybox_cube_indices.size()});
        // 注册销毁用回调函数在程序结束时调用
        resources.deconstructors.emplace_back([vao_id, vbo_id, ibo_id]() {
            glDeleteVertexArrays(1, &vao_id);
            glDeleteBuffers(1, &vbo_id);
            glDeleteBuffers(1, &ibo_id);
            checkError();
        });
    }
}

// 从json加载一个Vector3f
Vector3f load_vec3(const SimpleJson::JsonObject &json) {
    assert(json.get_type() == SimpleJson::JsonType::List);
    const std::vector<SimpleJson::JsonObject> &numbers = json.get_list();
    return {(float)numbers[0].get_number(), (float)numbers[1].get_number(), (float)numbers[2].get_number()};
}
// 从json加载transform，对不完整或不存在的取默认值
Transform load_transform(const SimpleJson::JsonObject &json) {
    Transform trans{
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
    };
    if (json.has("transform")) {
        const SimpleJson::JsonObject &t = json["transform"];
        if (t.has("position")) {
            trans.position = load_vec3(t["position"]);
        }
        if (t.has("rotation")) {
            trans.rotation = load_vec3(t["rotation"]);
        }
        if (t.has("scale")) {
            trans.scale = load_vec3(t["scale"]);
        }
    }

    return trans;
}

// 从json递归加载节点
void load_node_from_json(const SimpleJson::JsonObject &node, GObject *root) {
    for (const SimpleJson::JsonObject &object_desc : node.get_list()) {
        GObjectDesc desc{load_transform(object_desc), {}};
        for (const SimpleJson::JsonObject &part_desc : object_desc["parts"].get_list()) {
            desc.parts.emplace_back(part_desc["mesh"].get_string(), part_desc["material"].get_string());
        }
        const std::string &name = object_desc.has("name") ? object_desc["name"].get_string() : "";
        root->attach_child(std::make_shared<GObject>(std::move(desc), name));
        if (object_desc.has("children")) {
            load_node_from_json(object_desc["children"], root->get_children().back().get());
        }
    }
}

// 从json文件加载场景
void load_scene_from_json(const std::string &path) {
    SimpleJson::JsonObject json = SimpleJson::parse_file(path);
    // 加载天空盒
    if (!json.has("skybox")) {
        std::cerr << "Skybox is required for a scene" << std::endl;
        exit(-1);
    }

    world.set_skybox(json["skybox"]["specular_texture"].get_string());
    // 加载物体
    if (json.has("root")) {
        load_node_from_json(json["root"], world.get_root().get());
    }
    // 加载点光源
    if (json.has("pointlights")) {
        size_t light_index = 0;
        for (const SimpleJson::JsonObject &pointlight_desc : json["pointlights"].get_list()) {
            PointLight &light = world.pointlights[light_index++];
            light.enabled = true;
            light.position = load_vec3(pointlight_desc["position"]);
            light.color = load_vec3(pointlight_desc["color"]);
            light.factor = (float)pointlight_desc["factor"].get_number();
        }
    }
}
void init_start_scene() {
    load_scene_from_json("assets/scene1.json"); // 整个场景的所有物体都从json加载了
}

unsigned int calculate_fps(float delta_time) {
    const float ratio = 0.1f; // 平滑比例
    static float avarage_frame_time = std::nan("");

    if (std::isnormal(avarage_frame_time)) {
        avarage_frame_time = avarage_frame_time * (1.0f - ratio) + delta_time * ratio;
    } else {
        avarage_frame_time = delta_time;
    }

    return (unsigned int)(1000.0f / avarage_frame_time);
}

// 递归更新物体
void World::walk_gobject(GObject *root, uint32_t dirty_flags) {
    dirty_flags |= root->is_relat_dirty;
    if (dirty_flags) {
        if (root->has_parent()) {
            // 子节点的transform为父节点的transform叠加上自身的transform
            root->relate_model_matrix =
                root->get_parent().lock()->relate_model_matrix * root->transform.transform_matrix();
            root->relate_normal_matrix =
                root->get_parent().lock()->relate_normal_matrix * root->transform.normal_matrix();
        } else {
            root->relate_model_matrix = root->transform.transform_matrix();
            root->relate_normal_matrix = root->transform.normal_matrix();
        }

        for (GameObjectPart &p : root->parts) {
            // 更新自身每一个part的transform
            p.base_transform = root->relate_model_matrix * p.model_matrix;
            p.base_normal_matrix = root->relate_normal_matrix * p.normal_matrix;
        }
        root->is_relat_dirty = false;
    }
    //将此物体提交渲染
    for (GameObjectPart &p : root->parts) {
        // 提交自身每一个part
        renderer.lamertain_pass->accept(&p);
    }

    for (auto &child : root->children) {
        walk_gobject(child.get(), dirty_flags); // 更新子节点
    }
}

void World::tick() {
    clock.update(); // 更新时钟
    // 调用所有的系统
    for (const auto &[name, sys] : systems) {
        if (sys->enable) {
            sys->tick();
        }
    }
    // 递归更新所有物体
    walk_gobject(this->root.get(), 0);

    //上传天空盒
    renderer.skybox_pass->set_skybox(skybox.color_texture_id);
}

#define MY_TITLE "2020270901005 homework 3"

void loop_func() {
    render_info.tick_count++;

    world.tick();
    renderer.render();

    input.clear_mouse_move();

    // 计算和显示FPS
    std::stringstream formatter;
    formatter << MY_TITLE << "  ";
    formatter << "FPS: " << calculate_fps(world.clock.get_delta()) << "  ";
    glutSetWindowTitle(formatter.str().c_str());
}

class MoveSystem : public ISystem {
private:
public:
    using ISystem::ISystem;
    void handle_mouse() {
        // 使用鼠标中键旋转视角
        //  左上角为(0,0)，右下角为(w,h)
        static bool is_left_button_down = false;
        static bool is_right_button_down = false;

        if (!is_left_button_down && input.is_left_button_down()) {
            world.pointlights[0].enabled = false;
            world.pointlights[1].enabled = false;
            world.is_light_dirty = true;
        }

        if (!is_right_button_down && input.is_right_button_down()) {
            world.pointlights[0].enabled = true;
            world.pointlights[1].enabled = true;
            world.is_light_dirty = true;
        }

        if (input.is_middle_button_down()) {
            auto [dx, dy] = input.get_mouse_move().v;
            // 鼠标向右拖拽，相机沿y轴顺时针旋转。鼠标向下拖拽时，相机沿x轴逆时针旋转
            const float rotate_speed = 0.003f;
            Camera &desc = world.camera;
            desc.rotation.z += dx * rotate_speed;
            desc.rotation.y -= dy * rotate_speed;
        }

        is_left_button_down = input.is_left_button_down();
        is_right_button_down = input.is_right_button_down();
    }

    void handle_keyboard(float delta) {
        // wasd移动
        const float move_speed = 0.02f * delta;

        if (IS_KEYDOWN(VK_ESCAPE)) {
            PostQuitMessage(0);
        }
        if (IS_KEYDOWN('W')) {
            Vector3f ori = world.camera.get_orientation();
            ori.y = 0.0;
            world.camera.position += ori.normalize() * move_speed;
        }
        if (IS_KEYDOWN('S')) {
            Vector3f ori = world.camera.get_orientation();
            ori.y = 0.0;
            world.camera.position += ori.normalize() * -move_speed;
        }
        if (IS_KEYDOWN('A')) {
            Vector3f ori = world.camera.get_orientation();
            ori = {ori.z, 0.0, -ori.x};
            world.camera.position += ori.normalize() * move_speed;
        }
        if (IS_KEYDOWN('D')) {
            Vector3f ori = world.camera.get_orientation();
            ori = {ori.z, 0.0, -ori.x};
            world.camera.position += ori.normalize() * -move_speed;
        }
        if (IS_KEYDOWN(VK_SPACE)) {
            world.camera.position.y += move_speed;
        }
        if (IS_KEYDOWN(VK_SHIFT)) {
            world.camera.position.y -= move_speed;
        }

        if (IS_KEYDOWN('0')) {
            world.camera.position = {0.0, 0.0, 0.0};
            world.camera.rotation = {0.0, 0.0, 0.0};
        }

        if (IS_KEYDOWN(VK_UP)) {
            world.fog_density += 0.00001f * delta;
        }

        if (IS_KEYDOWN(VK_DOWN)) {
            world.fog_density -= 0.00001f * delta;
            world.fog_density = std::max(world.fog_density, 0.0f);
        }
    }

    void on_attach() override { ball = world.get_root()->get_child_by_name("球"); }

    void tick() override {
        handle_keyboard(world.clock.get_delta());
        handle_mouse();

        ball->transform.rotation.x += world.clock.get_delta() * 0.001f;
        ball->transform.rotation.y += world.clock.get_delta() * 0.003f;
        ball->is_relat_dirty = true;
        world.pointlights[0].position.x = 20.0f * sinf(render_info.tick_count * 0.01f);
        world.is_light_dirty = true;
    }

private:
    std::shared_ptr<GObject> ball;
};

#define INIT_WINDOW_WIDTH 1080
#define INIT_WINDOW_HEIGHT 720

int main(int argc, char **argv) {
    // 初始化glut和创建窗口
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT);
    glutCreateWindow(MY_TITLE);

    // 注册相关函数
    glutIdleFunc(loop_func);  // 一直不停执行循环函数，此函数包含主要逻辑
    glutDisplayFunc([]() {}); // 无视DisplayFunc
    // 保证视口与窗口大小相同
    glutReshapeFunc([](int w, int h) { render_info.set_viewport(0, 0, w, h); });
    glutMouseFunc(handle_mouse_click);
    glutMotionFunc(handle_mouse_move);
    glutPassiveMotionFunc(handle_mouse_move);

    // 初始化opengl
    setup_opengl();

    // 初始化资源和加载资源
    render_info.init();
    render_info.set_viewport(0, 0, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT); // 初始化时也需要设置一下视口
    init_resource();

    renderer.init();

    // 加载初始场景
    init_start_scene();

    // 注册系统
    world.register_system(new MoveSystem("move"));

    // 注册在退出时执行的清理操作
    atexit([] {
        resources.clear();
        render_info.clear();
        std::cout << "Exit!\n";
    });

    world.clock.update(); // 重置一下时钟

    // XX，启动！
    glutMainLoop();

    return 0;
}