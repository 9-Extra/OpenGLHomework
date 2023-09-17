#pragma once

#include <unordered_map>
#include <vector>
#include <assert.h>
//learned from piccolo. But not the same thing.

class GuidAlloctor{
public:
    uint32_t alloc_id(){
        if (!available_id.empty()){
            uint32_t id = available_id.back();
            available_id.pop_back();
            return id;
        } else {
            return ++last_id;
        }
    }

    void free_id(uint32_t id){
        assert(id <= last_id);
        assert(available_id.size() <= last_id);
        available_id.push_back(id);
    }

    void clear(){
        available_id.clear();
        last_id = 0;
    }

private:
    std::vector<uint32_t> available_id;
    uint32_t last_id{0};//最大分配的id，同时是已分配的id的个数
};