#include <assert.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <GL/glew.h>
#include <GL/glut.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <functional>

#define NOGDICAPMASKS
#define NOSYSMETRICS
// #define NOMENUS
// #define NOICONS
// #define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define OEMRESOURCE
#define NOATOM
#define NOCOLOR
// #define NOCTLMGR
#define NODRAWTEXT
// #define NOGDI
#define NOKERNEL
// #define NONLS
#define NOMEMMGR
#define NOMETAFILE
// #define NOMINMAX
// #define NOMSG
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <FreeImage.h>

static float Q_rsqrt(float number) {
    long i;
    float x2, y;
    const float threehalfs = 1.5F;

    x2 = number * 0.5F;
    y = number;
    i = *(long *)&y;           // evil floating point bit level hacking
    i = 0x5f3759df - (i >> 1); // what the fuck?
    y = *(float *)&i;
    y = y * (threehalfs - (x2 * y * y)); // 1st iteration
    // y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be
    // removed

    return y;
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
    Vector2f(float x, float y) : x(x), y(y) {}

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

struct Color {
    float r, g, b;

    inline const float *data() const { return (float *)this; }

    inline bool operator==(const Color &ps) { return r == ps.r && g == ps.g && b == ps.b; }

    inline bool operator!=(const Color &ps) { return !(*this == ps); }
};

struct Vector3f {
    union {
        struct {
            float x, y, z;
        };
        float v[3];
    };

    Vector3f() {}
    Vector3f(float x, float y, float z) : x(x), y(y), z(z) {}

    const float *data() const { return (float *)v; }

    inline float operator[](const unsigned int i) const { return v[i]; }

    inline Vector3f operator+(const Vector3f b) const { return Vector3f{x + b.x, y + b.y, z + b.z}; }

    inline Vector3f operator-(const Vector3f b) const { return Vector3f{x - b.x, y - b.y, z - b.z}; }

    inline Vector3f operator-() const { return Vector3f{-x, -y, -z}; }

    inline Vector3f operator+=(const Vector3f b) { return *this = *this + b; }

    inline Vector3f operator*(const float n) { return {x * n, y * n, z * n}; }
    inline Vector3f operator/(const float n) { return *this * (1.0f / n); }

    inline float dot(const Vector3f b) const { return x * b.x + y * b.y + z * b.z; }

    inline Vector3f cross(const Vector3f b) {
        return {this->y * b.z - this->z * b.y, this->z * b.x * this->x * b.z, this->x * b.y - this->y * b.x};
    }

    inline float square() const { return this->dot(*this); }

    inline Vector3f normalize() {
        float inv_sqrt = Q_rsqrt(this->square());
        return *this * inv_sqrt;
    }

    inline float length() const { return std::sqrt(square()); }

    friend std::ostream &operator<<(std::ostream &os, const Vector3f &v) {
        os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }
};

struct Matrix {
    float m[4][4];

    Matrix() {}
    constexpr Matrix(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20,
                     float m21, float m22, float m23, float m30, float m31, float m32, float m33)
        : m{{m00, m01, m02, m03}, {m10, m11, m12, m13}, {m20, m21, m22, m23}, {m30, m31, m32, m33}} {}

    const float *data() const { return (float *)m; }

    inline Matrix transpose() const {
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

    inline Matrix operator*(const Matrix &m) const {
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
    inline static Matrix rotate(float roll, float pitch, float yaw) {
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

    inline static Matrix rotate(Vector3f rotate) { return Matrix::rotate(rotate.x, rotate.y, rotate.z); }

    inline static Matrix translate(float x, float y, float z) {
        Matrix m{
            1.0, 0.0, 0.0, x, 0.0, 1.0, 0.0, y, 0.0, 0.0, 1.0, z, 0.0, 0.0, 0.0, 1.0,
        };
        return m;
    }

    inline static Matrix translate(Vector3f delta) { return Matrix::translate(delta.x, delta.y, delta.z); }

    inline static Matrix scale(float x, float y, float z) {
        Matrix m{
            x, 0.0, 0.0, 0.0, 0.0, y, 0.0, 0.0, 0.0, 0.0, z, 0.0, 0.0, 0.0, 0.0, 1.0,
        };

        return m;
    }

    inline static Matrix scale(Vector3f scale) { return Matrix::scale(scale.x, scale.y, scale.z); }
};

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

// struct Quaternion {
// 	float x, y, z, w;

//     static constexpr Quaternion no_rotate(){
//         return Quaternion{0.0f, 0.0f, 0.0f, 1.0f};
//     }

// 	//需要axis长度为1
// 	static Quaternion from_rotation(Vector3f axis, float angle) {
// 		angle = angle * 0.5f;
// 		float sin_theta = sinf(angle), cos_theta = cosf(angle);
// 		return Quaternion{ sin_theta * axis.x, sin_theta * axis.y
// ,sin_theta * axis.z, cos_theta };
// 	}

// 	//按XYZ顺序顺时针，沿X旋转的角度，沿Y旋转的角度，沿Z旋转的角度
// 	static Quaternion from_eular(Vector3f rotate) {
// 		return
// 			Quaternion::from_rotation({ 1.0f, 0.0f, 0.0f }, rotate.x)
// * 			Quaternion::from_rotation({ 0.0f, 1.0f, 0.0f }, rotate.y) *
// 			Quaternion::from_rotation({ 0.0f, 0.0f, 1.0f },
// rotate.z);

// 	}

// 	//需要长度为1
// 	Matrix4f to_matrix() const{
// 		return Matrix4f{ {{
// 			1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - w * z), 2.0f *
// (x * z + w * y), 0.0f, 			2.0f * (x * y + w * z),1.0f - 2.0f * (x * x + z *
// z), 2.0f * (y * z - w * x), 0.0f, 			2.0f * (x * z - w * y), 2.0f * (y * z + w *
// x), 1.0f - 2.0f * (x * x + y * y), 0.0f, 			0.0f,0.0f,0.0f,1.0f
// 		}} };
// 	}

// 	Vector3f rotate_vector(Vector3f src) const{
// 		const Quaternion& q = *this;
// 		Quaternion p{ src.x, src.y, src.z, 1.0f };
// 		Quaternion rotated = q * p * q.conjugate();
// 		return Vector3f{ rotated.x, rotated.y, rotated.z };
// 	}

// 	Quaternion conjugate() const{
// 		return Quaternion{ -x, -y, -z, w };
// 	}

// 	Quaternion operator* (const Quaternion r) const{
// 		Vector3f qv{ x, y, z }, rv{ r.x, r.y, r.z };
// 		Vector3f v = qv.cross(rv) + qv * r.w + rv * w;

// 		return Quaternion{ v.x, v.y, v.z, w * r.w - qv.dot(rv) };
// 	}
// };

inline void checkError() {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cerr << "GL error 0x" << error << ": "  << gluErrorString(error) << std::endl;
    }
}

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

    float get_delta() const { return delta; }
    float get_current_delta() const {
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
    std::ifstream vs_read(vs_path);
    std::string vs_src((std::istreambuf_iterator<char>(vs_read)), std::istreambuf_iterator<char>());

    std::ifstream ps_read(ps_path);
    std::string ps_src((std::istreambuf_iterator<char>(ps_read)), std::istreambuf_iterator<char>());

    unsigned int vs = complie_shader(vs_src.data(), GL_VERTEX_SHADER);
    unsigned int ps = complie_shader(ps_src.data(), GL_FRAGMENT_SHADER);

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
    Vector2f uv = {0.0f, 0.0f};
};

struct PointLight {
    Vector3f position;
    Vector3f flux;
};

struct DirectionalLight {
    Vector3f direction;
    Vector3f flux;
};

struct Mesh {
    const unsigned int VAO_id;
    const uint32_t indices_count;
};

struct TextureDesc{
    std::string path;
};

struct Texture{
    const unsigned int texture_id;
};

struct MaterialDesc{
    struct UniformDataDesc {
        uint32_t binding_id;
        uint32_t size;
        void* data;
    };

    struct SampleData {
        uint32_t binding_id;
        std::string texture_key;
    };

    std::string shader_name;
    std::vector<UniformDataDesc> uniforms;
    std::vector<SampleData> samplers;
};

//绑定标准可用的uniform
void bind_standard_uniform();
struct Material{
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

    void bind() const{
        glUseProgram(shaderprogram_id);

        bind_standard_uniform();

        for(const UniformData& u : uniforms){
            glBindBufferBase(GL_UNIFORM_BUFFER, u.binding_id, u.buffer_id);
        }
        for(const SampleData& s: samplers){
            glActiveTexture(GL_TEXTURE0 + s.binding_id);
            glBindTexture(GL_TEXTURE_2D, s.texture_id);
        }
    }
};
struct PhongMaterial {
    Vector3f diffusion;
    float specular_factor;
};
struct Shader{
    unsigned int program_id;
};

struct ResouceItem {
    virtual ~ResouceItem() = default;
};
class RenderReousce final {
public:
    template<class T>
    class ResourceContainer{
    public:
        uint32_t find(const std::string& key) const{
            return look_up.at(key);
        }

        void add(const std::string& key, T&& item){
            uint32_t id = container.size();
            container.emplace_back(std::move(item));
            look_up[key] = id;
        }

        const T& get(uint32_t id) const{
            return container[id];
        }

        void clear(){
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

    void add_mesh(const std::string &key, const std::vector<Vertex> &vertices,
                  const std::vector<unsigned int> &indices) {

        unsigned int vao_id, ibo_id, vbo_id;
        glGenVertexArrays(1, &vao_id);
        glGenBuffers(1, &ibo_id);
        glGenBuffers(1, &vbo_id);

        glBindVertexArray(vao_id);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        const unsigned int float_per_vertex = 8;
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)sizeof(Vector3f));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)(sizeof(Vector3f) + sizeof(Vector3f)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        checkError();
        meshes.add(key, Mesh{vao_id, (uint32_t)indices.size()});

        glBindVertexArray(0);

        struct MeshResource : public ResouceItem {
            unsigned int vao_id, ibo_id, vbo_id;

            MeshResource(unsigned int vao_id, unsigned int ibo_id, unsigned int vbo_id)
                : vao_id(vao_id), ibo_id(ibo_id), vbo_id(vbo_id) {}

            virtual ~MeshResource() {
                glDeleteVertexArrays(1, &vao_id);
                glDeleteBuffers(1, &vbo_id);
                glDeleteBuffers(1, &ibo_id);
                checkError();
            }
        };

        pool.emplace_back(std::make_unique<MeshResource>(vao_id, ibo_id, vbo_id));
    }

    void add_material(const std::string& key, const MaterialDesc& desc){
        Material mat;
        mat.shaderprogram_id = shaders.get(shaders.find(desc.shader_name)).program_id;
        for(const auto& u : desc.uniforms){
            unsigned int buffer_id;
            glGenBuffers(1, &buffer_id);
            glBindBuffer(GL_UNIFORM_BUFFER, buffer_id);
            glBufferData(GL_UNIFORM_BUFFER, u.size, u.data, GL_STATIC_DRAW);

            mat.uniforms.emplace_back(Material::UniformData{u.binding_id, buffer_id});

            deconstructors.emplace_back([buffer_id](){
                glDeleteBuffers(1, &buffer_id);
            });
        }
        for(const auto& s : desc.samplers){
            mat.samplers.emplace_back(
                Material::SampleData{s.binding_id, textures.get(textures.find(s.texture_key)).texture_id}
            );
        }

        materials.add(key, std::move(mat));
    }

    void add_texture(const std::string& key, const std::string &vs_path){
        FIBITMAP* pImage = FreeImage_Load(
        FreeImage_GetFileType(vs_path.c_str(), 0),
        vs_path.c_str());
        pImage = FreeImage_ConvertTo24Bits(pImage);
        
        unsigned int nWidth = FreeImage_GetWidth(pImage);
        unsigned int nHeight = FreeImage_GetHeight(pImage);

        unsigned int texture_id;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, nWidth, nHeight,
            0, GL_BGR, GL_UNSIGNED_BYTE, (void*)FreeImage_GetBits(pImage));
        glGenerateTextureMipmap(texture_id);

        checkError();

        FreeImage_Unload(pImage);

        textures.add(key, Texture{texture_id});

        deconstructors.emplace_back([texture_id](){
            glDeleteTextures(1, &texture_id);
        });
    }

    void add_shader(const std::string& key, const std::string &vs_path, const std::string &ps_path){
        unsigned int program_id = complie_shader_program(vs_path, ps_path);

        shaders.add(key, Shader{program_id});

        struct ShaderProgramResource: ResouceItem{
            unsigned int id;

            ShaderProgramResource(unsigned int id): id(id) {}

            virtual ~ShaderProgramResource(){
                glDeleteProgram(id);
            }
        };

        pool.emplace_back(std::make_unique<ShaderProgramResource>(program_id));

    }
    // 保存在退出时释放
    template <class T> void add_raw_resource(T &&item) { pool.emplace_back(std::make_unique<T>(std::move(item))); }

    void clear(){
        meshes.clear();
        materials.clear();
        shaders.clear();
        pool.clear();
        for(auto& de : deconstructors){
            de();
        }
        deconstructors.clear();
    }
private:

    std::vector<std::unique_ptr<ResouceItem>> pool; // 保存在这里以析构释放资源
    std::vector<std::function<void()>> deconstructors;
} resources;

struct GameObjectPart {
    uint32_t mesh_id;
    uint32_t material_id;
    uint32_t topology = GL_TRIANGLES;
    Matrix model_matrix;
    Matrix normal_matrix;

    GameObjectPart(const std::string &mesh_name, const std::string &material_name,
                   const uint32_t topology = GL_TRIANGLES, const Transform& transform = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}})
        : mesh_id(resources.meshes.find(mesh_name)), material_id(resources.materials.find(material_name)),
          topology(topology), model_matrix(transform.transform_matrix()), normal_matrix(transform.normal_matrix()) {}

private:
    friend class GObject;
    Matrix base_transform;
    Matrix base_normal_matrix;
};

struct GObjectDesc {
    Transform transform;
    std::vector<GameObjectPart> parts;
};

class GObject : public std::enable_shared_from_this<GObject> {
public:
    Transform transform;

    Matrix relate_model_matrix;
    bool is_relat_dirty = true;

    GObject(){};
    GObject(GObjectDesc &&desc) : transform(desc.transform), parts(std::move(desc.parts)){};

    // 可以重载
    virtual void tick() {
        if (is_relat_dirty) {
            relate_model_matrix = transform.transform_matrix();
            for (GameObjectPart &p : parts) {
                p.base_transform = relate_model_matrix * p.model_matrix;
                p.base_normal_matrix = transform.normal_matrix() * p.normal_matrix;
            }
            is_relat_dirty = false;
        }
    };

    virtual void render();

    void add_part(const GameObjectPart &part) {
        GameObjectPart &p = parts.emplace_back(part);
        p.base_transform = relate_model_matrix * p.model_matrix;
        p.base_normal_matrix = transform.normal_matrix() * p.normal_matrix;
    }

    virtual ~GObject() = default;

private:
    friend class World;
    std::vector<GameObjectPart> parts;
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

    Matrix get_view_perspective_matrix() const {
        return compute_perspective_matrix(aspect, fov, near_z, far_z) * Matrix::rotate(rotation).transpose() *
               Matrix::translate(-position);
    }
};

class ISystem {
public:
    bool enable;

    ISystem(const std::string &name, bool enable = true) : enable(enable), name(name) {}
    const std::string &get_name() const { return name; }

    virtual void tick() = 0;

    virtual ~ISystem() = default;

private:
    const std::string name;
};

class World {
public:
    Camera camera;
    Clock clock;

    template <class T = GObject, class... ARGS> std::shared_ptr<T> create_object(ARGS &&...args) {
        std::shared_ptr<T> g_object = std::make_shared<T>(std::forward<ARGS>(args)...);
        assert(dynamic_cast<GObject *>(g_object.get()));
        objects.emplace(g_object.get(), g_object);
        return g_object;
    }

    void register_object(std::shared_ptr<GObject> g_object) {
        assert(objects.find(g_object.get()) == objects.end()); // 不可重复添加
        objects.emplace(g_object.get(), g_object);
    }

    void kill_object(std::shared_ptr<GObject> g_object) {
        size_t i = objects.erase(g_object.get());
        assert(i != 0); // 试图删除不存在的object
    }

    void register_system(ISystem *system) {
        assert(system != nullptr && systems.find(system->get_name()) == systems.end());
        systems.emplace(system->get_name(), system);
    }

    ISystem *get_system(const std::string &name) { return systems.at(name).get(); }

    void remove_system(const std::string &name) { systems.erase(name); }

    void tick();

    void render();
    // 获取屏幕上的一点对应的射线方向
    Vector3f get_screen_point_oritation(Vector2f screen_xy) const;

private:
    std::unordered_map<GObject *, std::shared_ptr<GObject>> objects;
    std::unordered_map<std::string, std::unique_ptr<ISystem>> systems;
};

template<class T>
struct WritableUniformBuffer{
    //在初始化opengl后才能初始化
    void init(){
        assert(id == 0);
        glGenBuffers(1, &id);
        glBindBuffer(GL_UNIFORM_BUFFER, id);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(T), nullptr, GL_DYNAMIC_DRAW);
    }

    T* map(){
        void* ptr = glMapNamedBuffer(id, GL_WRITE_ONLY);
        assert(ptr != nullptr);
        return (T*)ptr;
    }
    void unmap(){
        bool ret = glUnmapNamedBuffer(id);
        assert(ret);
    }

    unsigned int get_id() const{
        return id;
    }

    void clear(){
        glDeleteBuffers(1, &id);
        id = 0;
    }
private:
    unsigned int id = 0;
};

struct PerFrameData{
    Matrix view_perspective_matrix;
};

struct PerObjectData{
    Matrix model_matrix;
    Matrix normal_matrix;
};
struct RenderInfo {
    struct Viewport {
        GLint x;
        GLint y;
        GLsizei width;
        GLsizei height;
    } main_viewport;

    WritableUniformBuffer<PerFrameData> per_frame_uniform;
    WritableUniformBuffer<PerObjectData> per_object_uniform;

    void init(){
        per_frame_uniform.init();
        per_object_uniform.init();
    }

    void clear(){
        per_frame_uniform.clear();
        per_object_uniform.clear();
    }
};

World world;
RenderInfo render_info;

void bind_standard_uniform(){
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, render_info.per_frame_uniform.get_id());
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, render_info.per_object_uniform.get_id());
}

void set_viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    render_info.main_viewport = {x, y, width, height};
    world.camera.aspect = float(width) / float(height);
}

void setup_opengl() {
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
    }

    if (!GLEW_ARB_compatibility) {
        std::cerr << "系统不支持旧的API" << std::endl;
    }
    const char *vendorName = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    std::cout << vendorName << ": " << version << std::endl;

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // 逆时针的面为正面
    glDepthFunc(GL_LESS);

    checkError();
}

// render的默认实现
void GObject::render() {
    {
        for (const GameObjectPart &p : parts) {
            auto data = render_info.per_object_uniform.map();
            data->model_matrix = p.base_transform.transpose();
            data->normal_matrix = p.base_normal_matrix.transpose();
            //data->object_matrix = Matrix::identity();
            render_info.per_object_uniform.unmap();

            // glBindBuffer(GL_UNIFORM_BUFFER, render_info.per_object_uniform.get_id());
            // glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Matrix), p.base_transform.transpose().data());

            const Material &material = resources.materials.get(p.material_id);
            material.bind();

            const Mesh &mesh = resources.meshes.get(p.mesh_id);

            for (uint32_t i = 0; i < mesh.indices_count; i++) {
                glBindVertexArray(mesh.VAO_id);
                glDrawElements(p.topology, mesh.indices_count, GL_UNSIGNED_INT, 0);
                checkError();
            }
        }
    }
}

namespace Assets {
const std::vector<Vertex> cube_vertices = {{{-1.0, -1.0, -1.0}, {0.0, 0.0, 0.0}}, {{1.0, -1.0, -1.0}, {1.0, 0.0, 0.0}},
                                           {{1.0, 1.0, -1.0}, {1.0, 1.0, 0.0}},   {{-1.0, 1.0, -1.0}, {0.0, 1.0, 0.0}},
                                           {{-1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}},  {{1.0, -1.0, 1.0}, {1.0, 0.0, 1.0}},
                                           {{1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}},    {{-1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}}};

const std::vector<Vertex> platform_vertices = {
    {{-1.0, -1.0, -1.0}, {0.5, 0.5, 0.5}}, {{1.0, -1.0, -1.0}, {0.5, 0.5, 0.5}}, {{1.0, 1.0, -1.0}, {0.5, 0.5, 0.5}},
    {{-1.0, 1.0, -1.0}, {0.5, 0.5, 0.5}},  {{-1.0, -1.0, 1.0}, {0.5, 0.5, 0.5}}, {{1.0, -1.0, 1.0}, {0.5, 0.5, 0.5}},
    {{1.0, 1.0, 1.0}, {0.5, 0.5, 0.5}},    {{-1.0, 1.0, 1.0}, {0.5, 0.5, 0.5}}};

const std::vector<unsigned int> cube_indices = {1, 0, 3, 3, 2, 1, 3, 7, 6, 6, 2, 3, 7, 3, 0, 0, 4, 7,
                                                2, 6, 5, 5, 1, 2, 4, 5, 6, 6, 7, 4, 5, 4, 0, 0, 1, 5};

const std::vector<Vertex> plane_vertices = {
    {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 1.0f}},
    {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, -1.0f}},
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {-1.0f, -1.0f}},
};

const std::vector<unsigned int> plane_indices = {3, 2, 0, 2, 1, 0};

const std::vector<Vertex> line_vertices = {{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                                           {{1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}}};

const std::vector<unsigned int> line_indices = {0, 1};
} // namespace Assets

void init_resource() {
    RenderReousce &resource = resources;

    resource.add_shader("single_color", 
    "assets/shaders/single_color/vs.vert", 
    "assets/shaders/single_color/ps.frag"
    );

    resource.add_shader("flat", 
    "assets/shaders/flat/vs.vert", 
    "assets/shaders/flat/ps.frag"
    );

    resource.add_shader("phong", 
    "assets/shaders/phong/vs.vert", 
    "assets/shaders/phong/ps.frag"
    );

    resource.add_texture("wood_diffusion", "assets/materials/wood_flat/basecolor.jpg");

    Vector3f color_while{1.0f, 1.0f, 1.0f};
    resource.add_material("wood_flat", MaterialDesc{
        "phong",
        {
            {2, sizeof(Vector3f), &color_while}
        },
        {
            {3, "wood_diffusion"}
        }
    });


    resource.add_mesh("cube", Assets::cube_vertices, Assets::cube_indices);
    resource.add_mesh("platform", Assets::platform_vertices, Assets::cube_indices);
    resource.add_mesh("default", {}, {});

    MaterialDesc green_material_desc;
    Vector3f color_green{0.0f, 1.0f, 0.0f};
    green_material_desc.shader_name = "single_color";
    green_material_desc.uniforms.emplace_back(
        MaterialDesc::UniformDataDesc{
            2,
            sizeof(Vector3f),
            &color_green
        }
    );
    resource.add_material("default", green_material_desc);

    resource.add_mesh("line", Assets::line_vertices, Assets::line_indices);
    resource.add_mesh("plane", Assets::plane_vertices, Assets::plane_indices);

    // 生产circle的顶点
    std::vector<Vertex> circle_vertices(101);
    // 中心点为{0, 0, 0}，半径为1，100边型，101个顶点
    circle_vertices[0] = {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}; // 中心点
    for (size_t i = 1; i < 101; i++) {
        float angle = to_radian(360.0f / 100 * (i - 1));
        circle_vertices[i] = {{sinf(angle), cosf(angle), 0.0f},
                              {sinf(angle) / 2 + 1, cosf(angle) / 2 + 1, float(i) / 100}};
    }

    std::vector<unsigned int> circle_indices(300); // 100个三角形
    for (size_t i = 0; i <= 99; i++) {             // 前99个
        // 逆时针
        circle_indices[i * 3] = 0;
        circle_indices[i * 3 + 1] = i + 2;
        circle_indices[i * 3 + 2] = i + 1;
    }
    // 最后一个
    circle_indices[297] = 0;
    circle_indices[298] = 1;
    circle_indices[299] = 100;

    resource.add_mesh("circle", circle_vertices, circle_indices);
}

void init_start_scene() {
    {
        Camera &desc = world.camera;
        desc.position = {0.0f, 0.0f, 0.0f};
        desc.rotation = {0.0f, 0.0f, 0.0f};
        desc.fov = 1.57f;
        desc.near_z = 1.0f;
        desc.far_z = 1000.0f;
        desc.aspect = float(render_info.main_viewport.width) / float(render_info.main_viewport.height);
    }

    {
        world.create_object(
            GObjectDesc{{{0.0f, 0.0f, -10.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}, {{"cube", "default"}}});
    }
    {
        world.create_object(
            GObjectDesc{
                {
                    {0.0f, 5.0f, -10.0f}, 
                    {0.0f, 0.0f, 1.57f}, 
                    {1.0f, 1.0f, 1.0f}}, 
                    {{"plane", "wood_flat"}
                    }});
    }
    {
        auto platform = world.create_object();
        platform->add_part(GameObjectPart{"platform", "default"});
        platform->transform = {{0.0f, -2.0f, -10.0f}, {0.0f, 0.0f, 0.0f}, {4.0, 0.1, 4.0}};
    }
}

inline unsigned int calculate_fps(float delta_time) {
    const float ratio = 0.1f;
    static float avarage_frame_time = std::nan("");

    if (std::isnormal(avarage_frame_time)) {
        avarage_frame_time = avarage_frame_time * (1.0f - ratio) + delta_time * ratio;
    } else {
        avarage_frame_time = delta_time;
    }

    return (unsigned int)(1000.0f / avarage_frame_time);
}

void World::tick() {
    clock.update();

    for (const auto &[name, sys] : systems) {
        if (sys->enable) {
            sys->tick();
        }
    }

    for (auto &[ptr, obj] : objects) {
        obj->tick();
    }
}

void World::render() {
    //glDrawBuffer(GL_BACK); // 渲染到后缓冲区
    RenderInfo::Viewport &v = render_info.main_viewport;
    glViewport(v.x, v.y, v.width, v.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto data = render_info.per_frame_uniform.map();
    data->view_perspective_matrix = camera.get_view_perspective_matrix().transpose();
    render_info.per_frame_uniform.unmap();

    for (auto &[ptr, obj] : objects) {
        obj->render();
    }

    glutSwapBuffers();
    checkError();
}

#define MY_TITLE "2020270901005 homework 3"

void loop_func() {
    world.tick();
    world.render();

    input.clear_mouse_move();
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
        static bool is_middle_button_down = false;

        // if (!is_middle_button_down && input.is_middle_button_down()) {
        //     runtime->display.grab_mouse();
        // }
        // if (is_middle_button_down && !input.is_middle_button_down()) {
        //     runtime->display.release_mouse();
        // }

        if (input.is_middle_button_down()) {
            auto [dx, dy] = input.get_mouse_move().v;
            // 鼠标向右拖拽，相机沿y轴顺时针旋转。鼠标向下拖拽时，相机沿x轴逆时针旋转
            const float rotate_speed = 0.003f;
            Camera &desc = world.camera;
            desc.rotation.z += dx * rotate_speed;
            desc.rotation.y -= dy * rotate_speed;
        }

        is_middle_button_down = input.is_middle_button_down();
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
    }

    void tick() override {
        handle_keyboard(world.clock.get_delta());
        handle_mouse();
    }
};

int main(int argc, char **argv) {
    std::cout << "Hello\n";

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition(50, 100);
    glutInitWindowSize(400, 300);
    glutCreateWindow(MY_TITLE);

    set_viewport(0, 0, 400, 300);

    glutIdleFunc(loop_func);
    glutDisplayFunc([]() {});
    glutReshapeFunc([](int w, int h) { set_viewport(0, 0, w, h); });
    glutMouseFunc(handle_mouse_click);
    glutMotionFunc(handle_mouse_move);
    glutPassiveMotionFunc(handle_mouse_move);

    setup_opengl();
    render_info.init();
    init_resource();
    init_start_scene();

    world.register_system(new MoveSystem("move"));

    world.clock.update();

    glutMainLoop();

    //可惜这些代码都不能执行
    resources.clear();
    render_info.clear();
    std::cout << "Normal Exit!\n";
    return 0;
}