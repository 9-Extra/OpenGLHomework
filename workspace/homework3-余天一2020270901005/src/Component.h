#pragma once

#include <assert.h>

class GObject;
// 组件基类，组件挂在GObject上，在每个tick时会被调用，不同的组件通过重载tick实现其功能
class Component{
public:
    Component() : owner(nullptr) {}
    GObject* get_owner() const{
        return owner;
    }
    virtual ~Component() = default;
protected:
    friend class GObject;
    friend class World;
    void set_owner(GObject* obj){
        assert(owner == nullptr);// 一个Component只能有一个owner
        owner = obj;
    }
    virtual void tick() = 0;
private:    
    GObject* owner; // Weak reference
};