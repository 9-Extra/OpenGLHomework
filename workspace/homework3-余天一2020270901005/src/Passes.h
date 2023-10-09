#pragma once

#include "utils.h"
#include "RenderItem.h"

// Pass 基类
class Pass{
public:
    virtual void run() = 0;
    friend class Renderer;

    virtual ~Pass() = default;
};

class LambertianPass : public Pass {
public:
    void accept(GameObjectPart* part){
        parts.push_back(part);
    }

    void reset() {
        parts.clear();
    }

    virtual void run() override;

private:
    static constexpr unsigned int POINTLIGNT_MAX = 8;
    struct PointLightData final {
        alignas(16) Vector3f position;
        alignas(16) Vector3f intensity;
    };
    struct PerFrameData final {
        Matrix view_perspective_matrix;
        alignas(16) Vector3f ambient_light;
        alignas(16) Vector3f camera_position;
        alignas(4) float fog_min_distance;
        alignas(4) float fog_density;
        alignas(4) uint32_t pointlight_num;
        PointLightData pointlight_list[POINTLIGNT_MAX];
    };

    struct PerObjectData final{
        Matrix model_matrix;
        Matrix normal_matrix;
    };

    std::vector<GameObjectPart*> parts; // 记录要渲染的对象
    WritableUniformBuffer<PerFrameData> per_frame_uniform;   // 用于一般渲染每帧变化的数据
    WritableUniformBuffer<PerObjectData> per_object_uniform; // 用于一般渲染每个物体不同的数据
};

class SkyBoxPass : public Pass{
public:
    SkyBoxPass(){
        shader_program_id = resources.shaders.get(resources.shaders.find("skybox")).program_id;
        mesh_id = resources.meshes.find("skybox_cube");
    }

    virtual void run() override;
private:
    const static unsigned int SKYBOX_TEXTURE_BINDIGN = 5;
    struct SkyBoxData final {
        Matrix skybox_view_perspective_matrix;
    };

    //每帧更新
    WritableUniformBuffer<SkyBoxData> skybox_uniform;
    //初始化时设定
    unsigned int shader_program_id;
    uint32_t mesh_id;
};

class PickupPass : public Pass {
public:
    PickupPass(){
        // 初始化pickup用的framebuffer
        glGenRenderbuffers(1, &framebuffer_pickup_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, framebuffer_pickup_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // 完整的初始化推迟到使用的时候
        checkError();

        glGenFramebuffers(1, &framebuffer_pickup);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_pickup);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, framebuffer_pickup_rbo);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        checkError();
    }

    virtual void run() override;

    ~PickupPass(){
        glDeleteRenderbuffers(1, &framebuffer_pickup_rbo);
        glDeleteFramebuffers(1, &framebuffer_pickup);
    }
private:
    unsigned int framebuffer_pickup;
    unsigned int framebuffer_pickup_rbo;

    // 设置framebuffer大小，显然要和视口一样大
    void set_framebuffer_size(unsigned int width, unsigned int height) {
        glNamedRenderbufferStorage(framebuffer_pickup_rbo, GL_R32UI, width, height);
        checkError();
        assert(glCheckNamedFramebufferStatus(framebuffer_pickup, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }
};