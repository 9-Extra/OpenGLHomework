#include "RenderResource.h"

#include <array>

#include "Sjson.h"
#include "utils.h"
#include "windows.h"
#include <FreeImage.h>

std::string read_whole_file(const std::string &path);

RenderReousce resources; // Global

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
FIBITMAP *freeimage_load_and_convert_image(const std::string &image_path, bool is_color=true) {
    FIBITMAP *pImage_ori = FreeImage_Load(FreeImage_GetFileType(image_path.c_str(), 0), image_path.c_str());
    if (pImage_ori == nullptr) {
        std::cerr << "Failed to load image: " << image_path << std::endl;
        exit(-1);
    }
    FIBITMAP *pImage = FreeImage_ConvertTo24Bits(pImage_ori);
    FreeImage_FlipVertical(pImage); // 翻转，适应opengl的方向
    if (is_color) {
        FreeImage_AdjustGamma(pImage, 1 / 2.2);// 对于颜色贴图，进行矫正
    }
    FreeImage_Unload(pImage_ori);

    return pImage;
}
void RenderReousce::load_gltf(const std::string &base_key, const std::string &path) {
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
            Vector2f *uv =
                (Vector2f *)((char *)buffers[uv_buffer["buffer"].get_uint()].ptr + uv_buffer["byteOffset"].get_uint());
            Vector4f *tangent = (Vector4f *)((char *)buffers[tangent_buffer["buffer"].get_uint()].ptr +
                                             tangent_buffer["byteOffset"].get_uint());

            std::vector<Vertex> vertices(vertex_count);
            for (uint32_t i = 0; i < vertex_count; i++) {
                // tangent的第四个分量是用来根据平台决定手性的，在opengl中始终应该取1，所以忽略
                Vector3f tang = Vector3f(tangent[i].x, tangent[i].y, tangent[i].z);
                vertices[i] = {pos[i], normal[i], tang, uv[i]};
            }

            add_mesh(key, vertices.data(), vertices.size(), indices_ptr, indices_count);
        }
    }
    // 加载纹理（在加载材质时加载需要的纹理）
    auto load_texture = [&](size_t index, bool is_color) -> std::string {
        const Json &texture = json["images"][index];
        const std::string key = base_key + '.' + texture["name"].get_string();
        add_texture(key, root + texture["uri"].get_string(), is_color);
        return key;
    };

    // 加载材质
    if (json.has("materials")) {
        for (const Json &material : json["materials"].get_list()) {
            const std::string &key = base_key + '.' + material["name"].get_string();

            std::string normal_texture = "default_normal";
            if (material.has("normalTexture")) {
                normal_texture = load_texture(material["normalTexture"]["index"].get_uint(), false);
            }
            const std::string basecolor_texture =
                load_texture(material["pbrMetallicRoughness"]["baseColorTexture"]["index"].get_uint(), true);
            std::string metallic_roughness_texture = "white";
            if (material["pbrMetallicRoughness"].has("metallicRoughnessTexture")) {
                metallic_roughness_texture =
                    load_texture(material["pbrMetallicRoughness"]["metallicRoughnessTexture"]["index"].get_uint(), false);
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
void RenderReousce::load_json(const std::string &path) {
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
            bool is_color = texture_desc.has("is_color") ? texture_desc["is_color"].get_bool() : true;
            add_texture(key, base_dir + texture_desc["image"].get_string(), is_color);
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
void RenderReousce::clear() {
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
void RenderReousce::add_shader(const std::string &key, const std::string &vs_path, const std::string &ps_path) {
    std::cout << "Load shader: " << key << std::endl;
    unsigned int program_id = complie_shader_program(vs_path, ps_path);

    shaders.add(key, Shader{program_id});

    deconstructors.emplace_back([program_id]() { glDeleteProgram(program_id); });
}
void RenderReousce::add_cubemap(const std::string &key, const std::string &image_px, const std::string &image_nx,
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
void RenderReousce::add_texture(const std::string &key, const std::string &image_path, bool is_color) {
    std::cout << "Load texture: " << key << std::endl;

    FIBITMAP *pImage = freeimage_load_and_convert_image(image_path, is_color);

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
void RenderReousce::add_material(const std::string &key, const MaterialDesc &desc) {
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
void RenderReousce::add_mesh(const std::string &key, const Vertex *vertices, size_t vertex_count,
                             const uint16_t *const indices, size_t indices_count) {
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
