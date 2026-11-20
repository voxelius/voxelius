#include "server/pch.hh"

#include "server/world/chunk_unloader.hh"

#include "core/config/ref.hh"

#include "shared/component/transform.hh"
#include "shared/entity/class.hh"
#include "shared/globals.hh"
#include "shared/utils/view.hh"
#include "shared/world/chunk.hh"
#include "shared/world/world.hh"

#include "server/constant.hh"
#include "server/entity/entity_loader.hh"
#include "server/globals.hh"
#include "server/net/interest.hh"
#include "server/net/sessions.hh"
#include "server/world/chunk_loader.hh"

constexpr static std::size_t UNLOAD_BUDGET = constant::REGION_VOLUME;

void chunk_unloader::init(void)
{
    // empty
}

void chunk_unloader::fixed_update_late(void)
{
    ZoneScoped;

    auto all_sessions = sessions::all();

    std::vector<ChunkAlignedBox> boxes;
    boxes.reserve(all_sessions.size());

    for(const auto& session : all_sessions) {
        if(globals::registry.valid(session.player)) {
            auto& transform = globals::registry.get<Transform>(session.player);
            auto radius = static_cast<ChunkPos::value_type>(interest::view_distance.value());
            boxes.push_back(utils::view_box(transform.chunk, radius));
        }
    }

    std::vector<ChunkPos> to_unload;
    std::size_t autosaved = 0;

    auto& chunk_registry = globals::world->chunk_registry();
    auto chunks = chunk_registry.view<Chunk_Component>();

    for(const auto [entity, component] : chunks.each()) {
        auto inside_view = false;

        for(const auto& box : boxes) {
            auto& min = box.min();
            auto& max = box.max();
            auto& pos = component.position;

            // Keep whole columns alive while their XZ position is in view;
            // light needs every chunk of the realm, not only the visible ones.
            if(pos.x() < min.x() || pos.x() > max.x()) {
                continue;
            }

            if(pos.z() < min.z() || pos.z() > max.z()) {
                continue;
            }

            inside_view = true;
            break;
        }

        if(!inside_view) {
            if(to_unload.size() < UNLOAD_BUDGET) {
                to_unload.push_back(component.position);
            }

            continue;
        }
    }

    auto entities = globals::registry.view<EntityClass, Transform>();

    for(const auto& pos : to_unload) {
        std::vector<entt::entity> chunk_entities;

        for(const auto [entity, cls, transform] : entities.each()) {
            if(transform.chunk == pos) {
                chunk_entities.push_back(entity);
            }
        }

        entity_loader::save(pos, chunk_entities);

        for(const auto entity : chunk_entities) {
            globals::registry.destroy(entity);
        }

        chunk_loader::save(pos);
        globals::world->remove_chunk(pos);
    }
}
