#include "World.h"

#include "Sjson.h"
#include "Renderer.h"
#include "CpntMeshRender.h"
#include "CpntPointLight.h"
#include "CpntCamera.h"

World world; // global world instance

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
    this->root->tick(0);

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

// 从json加载组件
void load_conponents_from_json(GObject *obj, const SimpleJson::JsonObject &json) {
    for (const auto& [cpnt_name, cpnt_desc] :json.get_map()) {
        if (cpnt_name == "mesh_render") {
            std::unique_ptr<CpntMeshRender> cpnt_ptr = std::make_unique<CpntMeshRender>();
            CpntMeshRender* cpnt = cpnt_ptr.get();
            obj->add_component(std::move(cpnt_ptr));   
            if (cpnt_desc.has("parts")){
                for(const SimpleJson::JsonObject &p : cpnt_desc["parts"].get_list()) {
                    cpnt->add_part(GameObjectPart{p["mesh"].get_string(), p["material"].get_string(), GL_TRIANGLES, load_transform(p)});
                }
            }
        } else if (cpnt_name == "point_light") {
            Vector3f color = load_vec3(cpnt_desc["color"]);
            float radius = (float)cpnt_desc["factor"].get_number();
            obj->add_component(std::make_unique<CpntPointLight>(color, radius));      
        } else if (cpnt_name == "camera") {
            bool is_main = cpnt_desc.has("is_main") && cpnt_desc["is_main"].get_bool();
            float near_z = (float)cpnt_desc["near_z"].get_number();
            float far_z = (float)cpnt_desc["far_z"].get_number();
            float fov = (float)cpnt_desc["fov"].get_number();
            obj->add_component(std::make_unique<CpntCamera>(near_z, far_z, fov));  
            if (is_main){
                obj->get_component<CpntCamera>()->set_main_camera();
            }
        } else {
            std::cout << "unknown component: " << cpnt_name << std::endl;
        }
    }
}

// 从json递归加载节点
void load_node_from_json(const SimpleJson::JsonObject &node, GObject *root) {
    for (const SimpleJson::JsonObject &object_desc : node.get_list()) {
        const std::string &name = object_desc.has("name") ? object_desc["name"].get_string() : "";
        auto gobject = std::make_shared<GObject>(load_transform(object_desc), name);
        if (object_desc.has("components")) {
            load_conponents_from_json(gobject.get(), object_desc["components"]);
        }
    
        root->attach_child(gobject);
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
}