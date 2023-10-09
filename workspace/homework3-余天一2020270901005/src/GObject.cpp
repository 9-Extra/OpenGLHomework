#include "GObject.h"

void GObject::tick(uint32_t dirty_flags) {
    if (!enabled)
        return; // 跳过不启用的物体

    dirty_flags |= is_relat_dirty;
    if (dirty_flags) {
        if (has_parent()) {
            // 子节点的transform为父节点的transform叠加上自身的transform
            relate_model_matrix = get_parent().lock()->relate_model_matrix * transform.transform_matrix();
            relate_normal_matrix = get_parent().lock()->relate_normal_matrix * transform.normal_matrix();
        } else {
            relate_model_matrix = transform.transform_matrix();
            relate_normal_matrix = transform.normal_matrix();
        }
        render_need_update = true;
        is_relat_dirty = false;
    }
    // 更新组件
    for (auto &c : get_components()) {
        c->tick();
    }

    for (auto &child : get_children()) {
        child->tick(dirty_flags); // 更新子节点
    }
}
