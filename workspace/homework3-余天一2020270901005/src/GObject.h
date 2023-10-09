#pragma once

#include <memory>
#include <string>
#include <vector>

#include "CGmath.h"
#include "Component.h"
#include "RenderItem.h"

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

    
    ~GObject() {
        assert(!parent.lock()); // 必须没有父节点
    };

    void add_part(const GameObjectPart &part) {
        GameObjectPart &p = parts.emplace_back(part);
        p.base_transform = relate_model_matrix * p.model_matrix;
        p.base_normal_matrix = transform.normal_matrix() * p.normal_matrix;
    }

    std::vector<GameObjectPart> &get_parts() { return parts; }

    void add_component(std::unique_ptr<Component>&& component) {
        component->set_owner(this);
        components.push_back(std::move(component));
    }

    Component* get_component(const std::type_info& t_info){
        for (auto &component : components) {
            if (typeid(component) == t_info) {
                return component.get();
            }
        }
        return nullptr;
    }

    template<class T, std::enable_if_t<std::is_base_of_v<T, Component>>>
    T* get_component() {
        return (T*)get_component(typeid(T));
    }

    void remove_component(const std::type_info& t_info) {
        for (auto it = components.begin(); it!= components.end(); ++it) {
            if (typeid(*it) == t_info) {
                components.erase(it);
                break;
            }
        }
    }

    template<class T, std::enable_if_t<std::is_base_of_v<T, Component>>>
    void remove_component() {
        remove_component(typeid(T));
    }

    std::vector<std::unique_ptr<Component>>& get_components(){
        return components;
    }

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
        if (auto old_parent = child->parent.lock(); old_parent) {
            old_parent->remove_child(child.get());
        }
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

    void attach_parent(std::shared_ptr<GObject> new_parent) {
        if (new_parent == parent.lock())
            return;
        if (new_parent) {
            new_parent->attach_child(shared_from_this());
        }
    }

private:
    std::vector<GameObjectPart> parts;
    std::vector<std::unique_ptr<Component>> components;
    std::weak_ptr<GObject> parent;
    std::vector<std::shared_ptr<GObject>> children;
};