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
#include "server/universe.hh"
#include "server/world/chunk_loader.hh"
#include "server/world/world.hh"

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
    std::vector<ChunkPos> to_unload;
    std::vector<entt::entity> chunk_entities;

    for(const auto& world : universe::worlds()) {
        auto& registry = world->registry();

        boxes.clear();

        for(const auto& session : all_sessions) {
            if(session.world.get() != static_cast<World*>(world.get())) {
                continue;
            }

            if(!registry.valid(session.player)) {
                continue;
            }

            auto& transform = registry.get<Transform>(session.player);
            auto radius = static_cast<ChunkPos::value_type>(interest::view_distance.value());
            boxes.push_back(utils::view_box(transform.chunk, radius));
        }

        to_unload.clear();

        auto& chunk_registry = world->chunk_registry();
        auto chunks = chunk_registry.view<ChunkComponent>();

        for(const auto [entity, component] : chunks.each()) {
            auto inside_view = false;

            for(const auto& box : boxes) {
                auto& min = box.min();
                auto& max = box.max();
                auto& pos = component.position;

                if(pos.x() < min.x() || pos.x() > max.x()) {
                    continue;
                }

                if(pos.z() < min.z() || pos.z() > max.z()) {
                    continue;
                }

                inside_view = true;
                break;
            }

            if(!inside_view && to_unload.size() < UNLOAD_BUDGET) {
                to_unload.push_back(component.position);
            }
        }

        auto entities = registry.view<EntityClass, Transform>();

        for(const auto& pos : to_unload) {
            chunk_entities.clear();

            for(const auto [entity, cls, transform] : entities.each()) {
                if(transform.chunk == pos) {
                    chunk_entities.push_back(entity);
                }
            }

            entity_loader::save(world.get(), pos, chunk_entities);

            for(const auto entity : chunk_entities) {
                registry.destroy(entity);
            }

            chunk_loader::save(world, pos);
            world->remove_chunk(pos);
        }
    }
}
