#include "Passes.h"

#include "Renderer.h"

//一般物体渲染
void LambertianPass::run() {
    // 初始化渲染配置
    glEnable(GL_CULL_FACE);  // 启用面剔除
    glEnable(GL_DEPTH_TEST); // 启用深度测试
    glDrawBuffer(GL_BACK);   // 渲染到后缓冲区
    Renderer::Viewport &v = renderer.main_viewport;
    glViewport(v.x, v.y, v.width, v.height);
    // 清除旧画面
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 绑定per_frame和per_object uniform buffer
    per_frame_uniform.bind(0);
    per_object_uniform.bind(1);

    // 计算透视投影矩阵
    const float aspect = float(v.width) / float(v.height);
    const Camera &camera = renderer.main_camera;
    const Matrix view_perspective_matrix =
        compute_perspective_matrix(aspect, camera.fov, camera.near_z, camera.far_z) *
        Matrix::rotate(camera.rotation).transpose() * Matrix::translate(-camera.position);

    // 填充per_frame uniform数据
    auto data = per_frame_uniform.map();
    // 透视投影矩阵
    data->view_perspective_matrix = view_perspective_matrix.transpose();
    // 相机位置
    data->camera_position = camera.position;
    // 雾参数
    assert(renderer.fog_density >= 0.0f);
    data->fog_density = renderer.fog_density;
    data->fog_min_distance = renderer.fog_min_distance;
    // 灯光参数
    data->ambient_light = renderer.ambient_light;
    if (renderer.pointlights.size() > POINTLIGNT_MAX) {
        std::cout << "超出最大点光源数量" << std::endl;
    }
    uint32_t count = (uint32_t)std::min<size_t>(renderer.pointlights.size(), POINTLIGNT_MAX);
    for (uint32_t i = 0; i < count; ++i) {
        data->pointlight_list[i].position = renderer.pointlights[i].position;
        data->pointlight_list[i].intensity = renderer.pointlights[i].color * renderer.pointlights[i].factor;
    }
    data->pointlight_num = count;
    // 填充结束
    per_frame_uniform.unmap();

    // 遍历所有part，绘制每一个part
    for (const GameObjectPart *p : parts) {
        // 填充per_object uniform buffer
        auto data = per_object_uniform.map();
        data->model_matrix = p->base_transform.transpose();      // 变换矩阵
        data->normal_matrix = p->base_normal_matrix.transpose(); // 法线变换矩阵
        per_object_uniform.unmap();

        // 查找并绑定材质
        const Material &material = resources.materials.get(p->material_id);
        material.bind();

        const Mesh &mesh = resources.meshes.get(p->mesh_id); // 网格数据

        glBindVertexArray(mesh.VAO_id);                                        // 绑定网格
        glDrawElements(p->topology, mesh.indices_count, GL_UNSIGNED_SHORT, 0); // 绘制
        checkError();
    }
}

// 渲染天空盒
void SkyBoxPass::run() {
    glEnable(GL_CULL_FACE);  // 启用面剔除
    glEnable(GL_DEPTH_TEST); // 启用深度测试
    glDrawBuffer(GL_BACK);   // 渲染到后缓冲区

    // 用于天空盒的投影矩阵
    const float aspect = float(renderer.main_viewport.width) / float(renderer.main_viewport.height);
    const Camera &camera = renderer.main_camera;
    const Matrix skybox_view_perspective_matrix =
        compute_perspective_matrix(aspect, camera.fov, camera.near_z, camera.far_z) *
        Matrix::rotate(camera.rotation).transpose();

    // 填充天空盒需要的参数（透视投影矩阵）
    auto data = skybox_uniform.map();
    data->skybox_view_perspective_matrix = skybox_view_perspective_matrix.transpose();
    skybox_uniform.unmap();

    // 绑定天空盒专用着色器
    glUseProgram(shader_program_id);
    // 绑定uniform buffer
    skybox_uniform.bind(0);
    // 绑定天空盒纹理
    glActiveTexture(GL_TEXTURE0 + SKYBOX_TEXTURE_BINDIGN);
    assert(renderer.skybox.color_texture_id != 0); // 天空纹理必须存在
    glBindTexture(GL_TEXTURE_CUBE_MAP, renderer.skybox.color_texture_id);
    // 获取天空盒的网格（向内的Cube）
    const Mesh &mesh = resources.meshes.get(mesh_id);

    glBindVertexArray(mesh.VAO_id);                                         // 绑定网格
    glDrawElements(GL_TRIANGLES, mesh.indices_count, GL_UNSIGNED_SHORT, 0); // 绘制
    checkError();
}

// 拾取
void PickupPass::run() {
    set_framebuffer_size(renderer.main_viewport.width, renderer.main_viewport.height);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_pickup);
    glViewport(0, 0, renderer.main_viewport.width, renderer.main_viewport.height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //todo

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
} 