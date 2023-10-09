#include "World.h"

#include "Sjson.h"
#include "Renderer.h"

World world; // global world instance

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

        for (GameObjectPart &p : root->get_parts()) {
            // 更新自身每一个part的transform
            p.base_transform = root->relate_model_matrix * p.model_matrix;
            p.base_normal_matrix = root->relate_normal_matrix * p.normal_matrix;
        }
        root->is_relat_dirty = false;
    }
    //将此物体提交渲染
    for (GameObjectPart &p : root->get_parts()) {
        // 提交自身每一个part
        renderer.accept(&p);
    }

    for (auto &child : root->get_children()) {
        walk_gobject(child.get(), dirty_flags); // 更新子节点
    }
}

void World::tick() {
    tick_count++;

    clock.update(); // 更新时钟
    // 调用所有的系统
    for (const auto &[name, sys] : systems) {
        if (sys->enable) {
            sys->tick();
        }
    }
    // 递归更新所有物体
    walk_gobject(this->root.get(), 0);

    renderer.main_camera = camera;

    //上传天空盒
    //renderer.set_skybox();
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

    renderer.set_skybox(json["skybox"]["specular_texture"].get_string());
    // 加载物体
    if (json.has("root")) {
        load_node_from_json(json["root"], world.get_root().get());
    }
    // 加载点光源
    if (json.has("pointlights")) {
        for (const SimpleJson::JsonObject &pointlight_desc : json["pointlights"].get_list()) {
            PointLight &light = renderer.pointlights.emplace_back();
            light.position = load_vec3(pointlight_desc["position"]);
            light.color = load_vec3(pointlight_desc["color"]);
            light.factor = (float)pointlight_desc["factor"].get_number();
        }
    }
}