#pragma once

#include <vector>
#include <memory>

class ISystem {
public:
    virtual void init() {};
    virtual void tick() = 0;
    virtual bool delete_me() {return false;}

    virtual ~ISystem() = default;
};

extern std::vector<std::unique_ptr<ISystem>> system_list;