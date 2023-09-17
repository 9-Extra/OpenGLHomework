#include "Render.h"

void Renderer::render() {
    scene.update(resouces, swap_data[1]);

    glDrawBuffer(GL_BACK); // 渲染到后缓冲区
    glViewport(target_viewport.x, target_viewport.y, target_viewport.width, target_viewport.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(scene.main_camera.get_view_perspective_matrix().transpose().data());

    glMatrixMode(GL_MODELVIEW);
    for (const auto &[id, item] : scene.item_list) {
        const Mesh &mesh = resouces.get_mesh(item.mesh_id);
        const PhongMaterial &material = resouces.get_material(item.material_id);

        // glMaterialfv(GL_FRONT, GL_DIFFUSE, material.diffusion.data());

        glLoadMatrixf(item.model_matrix.transpose().data());
        glBegin(item.topology);
        for (uint32_t i = 0; i < mesh.indices_count; i++) {
            const Vertex &v = mesh.vertices[mesh.indices[i]];
            glColor3fv(v.color.data());
            glVertex3fv(v.position.data());
        }
        glEnd();
    }

    glFlush();
    checkError();

    do_swap();
}
void Renderer::do_swap() {
    // 由渲染线程调用
    swap_data[1].clear();
    WaitForSingleObject(swap_data_lock, INFINITE);
    std::swap(swap_data[0], swap_data[1]);
    bool r = ReleaseMutex(swap_data_lock);
    assert(r);
}
void Renderer::start_thread() {
    do_swap(); // 第一个swap_data是无效的，取出第一个有效的data（world在构造后会立即updata一次），在渲染时保证数据有效
    assert(render_thread == NULL);
    render_thread = CreateThread(NULL, 0, render_thread_func, nullptr, 0, nullptr);
    if (render_thread == NULL) {
        std::cerr << "创建渲染线程失败" << std::endl;
        exit(-1);
    }
}
Renderer::~Renderer() {
    if (!render_thread_should_exit) {
        std::cerr << "渲染线程意外退出！！！" << std::endl;
        exit(-1);
    }
    assert(GetCurrentThreadId() != GetThreadId(render_thread));
    WaitForSingleObject(render_thread, INFINITE);

    CloseHandle(swap_data_lock);
}
Renderer::Renderer() {
    swap_data_lock = CreateMutex(NULL, 0, NULL);
    if (swap_data_lock == NULL) {
        std::cerr << "创建互斥锁失败: " << GetLastError() << std::endl;
        exit(-1);
    }
}
void RenderScene::update(RenderReousce &resources, const RenderSwapData &swap_data) {

    if (swap_data.camera_desc.has_value()) {
        const CameraDesc &desc = swap_data.camera_desc.value();
        main_camera.position = desc.position;
        main_camera.rotation = desc.rotation;
        main_camera.fov = desc.fov;
        main_camera.near_z = desc.near_z;
        main_camera.far_z = desc.far_z;
    }

    // 先更新Object再删除，防止删除的Object因为更新而重新加入
    for (const GameObjectRenderDesc &object : swap_data.gameobject_desc) {
        for (uint32_t i = 0; i < object.parts.size(); i++) {
            GameObjectPartId part_id{object.id, i};
            const GameObjectPartDesc &part = object.parts[i];

            RenderItem &item = item_list[part_id];
            item.mesh_id = resources.find_mesh(part.resource_key_mesh);
            item.material_id = resources.find_material(part.resource_key_material);
            item.topology = part.topology;
            item.model_matrix = part.model_matrix;
        }
    }

    for (uint32_t id : swap_data.game_object_to_delete) {
        // 删除此object的所有part
        for (uint32_t i = 0; true; i++) {
            GameObjectPartId part_id{id, i};
            if (auto it = item_list.find(part_id); it != item_list.end()) {
                item_list.erase(it);
            } else {
                break;
            }
        }
    }

    for (const LightDesc &light : swap_data.light_desc) {
    }
}
