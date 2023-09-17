#include "GObject.h"

void GObject::update_render(LockedSwapData &swap_data) {
    if (is_render_dirty) {
        Matrix transform = Matrix::translate(position) * Matrix::rotate(rotation) * Matrix::scale(scale);

        std::vector<GameObjectPartDesc> dirty_parts;
        for (const GameObjectPartDesc &p : parts) {
            dirty_parts.emplace_back(p.resource_key_mesh, p.resource_key_material, p.topology,
                                     transform * p.model_matrix);
        }

        swap_data.add_dirty_gameobject(GameObjectRenderDesc{(size_t)this->shared_from_this().get(), std::move(dirty_parts)});
        is_render_dirty = false;
    }
}