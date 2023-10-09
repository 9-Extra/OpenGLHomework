#pragma once

#include <memory>
#include <string>
#include <vector>
#include <algorithm>

#include "CGmath.h"
#include "Component.h"
#include "RenderAspect.h"

// 一个物体，单独的物体没有功能，也不可见，需要挂上组件来实现具体的功能
class GObject final : public std::enable_shared_from_this<GObject> {
public:
    std::string name;
    Transform transform;

    Matrix relate_model_matrix;
    Matrix relate_normal_matrix;
    bool is_relat_dirty = true;

    bool render_need_update;

    GObject(const std::string name = "") : name(name){};
    GObject(const Transform transform, const std::string name = "")
        : name(name), transform(transform){};

    
    ~GObject() {
        assert(!parent.lock()); // 必须没有父节点
    };

    void enable(){
        if (!enabled){
            enabled = true;
            is_relat_dirty = true;// 重新计算变换矩阵
        }
    }

    void disable(){
        enabled = false;
    }

    void add_component(std::unique_ptr<Component>&& component) {
        component->set_owner(this);
        components.push_back(std::move(component));
    }

    Component* get_component(const std::type_info& t_info){
        for (auto &component : components) {
            auto& c = *component;// 比较其内容而非智能指针
            if (typeid(c) == t_info) {
                return component.get();
            }
        }
        return nullptr;
    }

    template<class T, typename = std::enable_if_t<std::is_base_of_v<Component, T>>>
    T* get_component() {
        return (T*)get_component(typeid(T));
    }

    void remove_component(const std::type_info& t_info) {
        for (auto it = components.begin(); it!= components.end(); ++it) {
            auto& c = **it;// 比较其内容而非智能指针
            if (typeid(c) == t_info) {
                components.erase(it);
                break;
            }
        }
    }

    template<class T, typename = std::enable_if_t<std::is_base_of_v<Component, T>>>
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
    friend class World;
    std::vector<std::unique_ptr<Component>> components;
    std::weak_ptr<GObject> parent;
    std::vector<std::shared_ptr<GObject>> children;

    bool enabled = true;

    void tick(uint32_t dirty_flags);
};