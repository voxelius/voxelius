#include "shared/pch.hh"

#include "shared/physics/physics.hh"

#include "shared/physics/ray_aabb.hh"
#include "shared/physics/ray_dda.hh"
#include "shared/utils/coord.hh"
#include "shared/world/block_collisions.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/world.hh"

static block_face make_block_face(const Eigen::Vector3f& normal)
{
    if(normal.isApprox(Eigen::Vector3f::UnitX())) {
        return BLOCK_FACE_EAST;
    }
    else if(normal.isApprox(-Eigen::Vector3f::UnitX())) {
        return BLOCK_FACE_WEST;
    }
    else if(normal.isApprox(Eigen::Vector3f::UnitY())) {
        return BLOCK_FACE_TOP;
    }
    else if(normal.isApprox(-Eigen::Vector3f::UnitY())) {
        return BLOCK_FACE_BOTTOM;
    }
    else if(normal.isApprox(Eigen::Vector3f::UnitZ())) {
        return BLOCK_FACE_SOUTH;
    }
    else if(normal.isApprox(-Eigen::Vector3f::UnitZ())) {
        return BLOCK_FACE_NORTH;
    }

    return BLOCK_FACE_NORTH; // fallback
}

std::optional<physics::BlockHit> physics::raycast_block(const Ray& ray, block_filter bfilter)
{
    assert(bfilter);

    std::optional<physics::BlockHit> closest = std::nullopt;
    auto closest_distance = std::numeric_limits<float>::max();

    Ray_DDA dda(ray.world, ray.start_chunk, ray.start, ray.direction);

    while(dda.distance() < ray.max_distance) {
        auto block_id = dda.step();

        if(block_id == BLOCK_ID_NULL) {
            continue;
        }

        auto def = block_registry::find_definition(block_id);
        auto bcoll = block_collisions::find(block_id);

        if(def == nullptr || bcoll == nullptr) {
            continue;
        }

        if(bfilter == BLOCK_FILTER_SOLID && (bcoll->elements.empty() || def->touch == BLOCK_TOUCH_NONE)) {
            continue;
        }

        auto hit_cpos = utils::to_chunk(dda.position());
        auto hit_lpos = utils::to_local(dda.position());
        auto bpos_float = utils::to_relative(ray.start_chunk, hit_cpos, hit_lpos.cast<float>());
        auto hit_in_block = false;

        for(const auto& aabb : bcoll->elements) {
            Eigen::AlignedBox3f world_aabb = aabb;
            world_aabb.translate(bpos_float);

            Ray_AABB ray_aabb(ray.start, ray.direction);

            if(auto hit = ray_aabb.intersect(world_aabb)) {
                if(hit->distance <= ray.max_distance) {
                    if(hit->distance < closest_distance) {
                        physics::BlockHit new_hit {};
                        new_hit.world = ray.world;

                        new_hit.distance = hit->distance;
                        new_hit.point = utils::to_relative(hit_cpos, ray.start_chunk, hit->point);
                        new_hit.normal = hit->normal;

                        auto family = block_registry::find_family(def->family);

                        new_hit.id = block_id;

                        if(family) {
                            new_hit.stem = family->stem_id;
                        }
                        else {
                            new_hit.stem = block_id;
                        }

                        new_hit.face = make_block_face(hit->normal);

                        new_hit.block_pos = dda.position();
                        new_hit.chunk_pos = hit_cpos;
                        new_hit.local_pos = hit_lpos;

                        closest = std::move(new_hit);
                        closest_distance = hit->distance;

                        hit_in_block = true;
                    }
                }
            }
        }

        if(hit_in_block) {
            break;
        }
    }

    return closest;
}

std::optional<physics::EntityHit> physics::raycast_entity(const Ray&, entity_filter efilter)
{
    assert(efilter);

    return std::nullopt; // TODO: raycast entities once the registry can enumerate candidates
}

physics::Hit physics::raycast(const Ray& ray, block_filter bfilter, entity_filter efilter)
{
    std::optional<physics::BlockHit> block_hit = std::nullopt;
    std::optional<physics::EntityHit> entity_hit = std::nullopt;

    if(bfilter) {
        block_hit = raycast_block(ray, bfilter);
    }

    if(efilter) {
        entity_hit = raycast_entity(ray, efilter);
    }

    auto block_distance = std::numeric_limits<float>::max();
    auto entity_distance = std::numeric_limits<float>::max();

    if(block_hit.has_value()) {
        block_distance = block_hit->distance;
    }

    if(entity_hit.has_value()) {
        entity_distance = entity_hit->distance;
    }

    if(block_distance < entity_distance) {
        return block_hit.value();
    }
    else if(entity_distance < block_distance) {
        return entity_hit.value();
    }

    return std::monostate {};
}
