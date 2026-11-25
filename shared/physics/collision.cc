#include "shared/pch.hh"

#include "shared/physics/collision.hh"

#include "shared/utils/coord.hh"
#include "shared/world/block_collisions.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/chunk.hh"
#include "shared/world/fluid_flow.hh"
#include "shared/world/fluid_registry.hh"
#include "shared/world/world.hh"

constexpr static float COLLISION_ADJUSTMENT = 0.001f;
constexpr static float STEP_TOLERANCE = 0.01f;
constexpr static float MAX_MOVEMENT = 8.0f;

struct Collider final {
    Eigen::AlignedBox3f box;
    physics::Contact contact;
    bool steppable { true };
};

struct Candidate final {
    std::size_t index { 0 };
    float t_squared { 0.0f };
};

struct ChunkCache final {
    bool valid { false };
    ChunkPos pos { ChunkPos::Zero() };
    std::shared_ptr<Chunk> chunk;
};

struct MoveState final {
    World* world { nullptr };
    ChunkPos chunk { ChunkPos::Zero() };
    const physics::Body* body { nullptr };

    Eigen::Vector3f position { Eigen::Vector3f::Zero() };
    Eigen::Vector3f movement { Eigen::Vector3f::Zero() };
    Eigen::AlignedBox3f entity_box;
    Eigen::AlignedBox3f extent_box;

    std::array<physics::Contact, 3> contacts;
    physics::Contact support;

    bool on_ground { false };
    bool hit_y_min { false };
    bool was_on_ground { false };
};

thread_local static std::vector<Collider> s_colliders;
thread_local static std::vector<Candidate> s_candidates;

static bool same_chunk(const ChunkPos& a, const ChunkPos& b)
{
    return a.x() == b.x() && a.y() == b.y() && a.z() == b.z();
}

static block_id_type find_block(World* world, const BlockPos& bpos, ChunkCache& cache)
{
    auto cpos = utils::to_chunk(bpos);

    if(!cache.valid || !same_chunk(cache.pos, cpos)) {
        cache.valid = true;
        cache.pos = cpos;
        cache.chunk = world->find_chunk(cpos);
    }

    if(cache.chunk == nullptr) {
        return BLOCK_ID_NULL;
    }

    return cache.chunk->get_block(utils::to_local(bpos));
}

template<typename Callback>
static bool for_each_block(World* world, const ChunkPos& chunk, const Eigen::AlignedBox3f& area, Callback&& callback)
{
    auto origin = utils::to_block(chunk);

    auto min_x = static_cast<BlockPos::value_type>(std::floor(area.min().x()));
    auto min_y = static_cast<BlockPos::value_type>(std::floor(area.min().y()));
    auto min_z = static_cast<BlockPos::value_type>(std::floor(area.min().z()));

    auto max_x = static_cast<BlockPos::value_type>(std::floor(area.max().x()));
    auto max_y = static_cast<BlockPos::value_type>(std::floor(area.max().y()));
    auto max_z = static_cast<BlockPos::value_type>(std::floor(area.max().z()));

    ChunkCache cache;

    for(auto y = min_y; y <= max_y; ++y) {
        for(auto z = min_z; z <= max_z; ++z) {
            for(auto x = min_x; x <= max_x; ++x) {
                BlockPos bpos = origin + BlockPos(x, y, z);
                auto id = find_block(world, bpos, cache);
                auto relative = Eigen::Vector3f(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));

                if(!callback(id, relative)) {
                    return false;
                }
            }
        }
    }

    return true;
}

static float axis_time(float movement, float entity_min, float entity_max, float block_min, float block_max)
{
    if(entity_max >= block_min && entity_min <= block_max) {
        return 0.0f;
    }

    if(movement == 0.0f) {
        return std::numeric_limits<float>::max();
    }

    auto distance = 0.0f;

    if(movement > 0.0f) {
        distance = block_min - entity_max;
    }
    else {
        distance = entity_min - block_max;
    }

    return std::abs(distance / movement);
}

static void calc_time(const Eigen::Vector3f& movement, const Eigen::AlignedBox3f& entity_box, const Eigen::AlignedBox3f& block_box,
    Eigen::Vector3f& time)
{
    time.x() = axis_time(movement.x(), entity_box.min().x(), entity_box.max().x(), block_box.min().x(), block_box.max().x());
    time.y() = axis_time(movement.y(), entity_box.min().y(), entity_box.max().y(), block_box.min().y(), block_box.max().y());
    time.z() = axis_time(movement.z(), entity_box.min().z(), entity_box.max().z(), block_box.min().z(), block_box.max().z());
}

static void update_x(MoveState& state, float position_x)
{
    state.position.x() = position_x;
    state.movement.x() = 0.0f;

    state.entity_box.min().x() = state.position.x() + state.body->local_box.min().x();
    state.entity_box.max().x() = state.position.x() + state.body->local_box.max().x();
    state.extent_box.min().x() = state.entity_box.min().x();
    state.extent_box.max().x() = state.entity_box.max().x();
}

static void update_y(MoveState& state, float position_y)
{
    state.position.y() = position_y;
    state.movement.y() = 0.0f;

    state.entity_box.min().y() = state.position.y() + state.body->local_box.min().y();
    state.entity_box.max().y() = state.position.y() + state.body->local_box.max().y();
    state.extent_box.min().y() = state.entity_box.min().y();
    state.extent_box.max().y() = state.entity_box.max().y();
}

static void update_z(MoveState& state, float position_z)
{
    state.position.z() = position_z;
    state.movement.z() = 0.0f;

    state.entity_box.min().z() = state.position.z() + state.body->local_box.min().z();
    state.entity_box.max().z() = state.position.z() + state.body->local_box.max().z();
    state.extent_box.min().z() = state.entity_box.min().z();
    state.extent_box.max().z() = state.entity_box.max().z();
}

static void clip_y_max(MoveState& state, const Collider& collider)
{
    auto position_y = collider.box.max().y() + COLLISION_ADJUSTMENT - state.body->local_box.min().y();

    update_y(state, position_y);

    state.contacts[1] = collider.contact;
    state.on_ground = true;
    state.support = collider.contact;
}

static void clip_y_min(MoveState& state, const Collider& collider)
{
    auto position_y = collider.box.min().y() - COLLISION_ADJUSTMENT - state.body->local_box.max().y();

    update_y(state, position_y);

    state.contacts[1] = collider.contact;
    state.hit_y_min = true;
}

static bool can_slide_through(World* world, const ChunkPos& chunk, const Eigen::AlignedBox3f& area)
{
    return for_each_block(world, chunk, area, [&area](block_id_type id, const Eigen::Vector3f& relative) {
        if(id == BLOCK_ID_NULL) {
            return true;
        }

        auto bcoll = block_collisions::find(id);

        if(bcoll == nullptr || !bcoll->collides()) {
            return true;
        }

        for(const auto& element : bcoll->elements) {
            auto box = element;
            box.translate(relative);

            if(box.intersects(area)) {
                return false;
            }
        }

        return true;
    });
}

static bool try_step(MoveState& state, const Collider& collider, const Eigen::AlignedBox3f& final_box)
{
    auto y_distance = collider.box.max().y() - state.entity_box.min().y();

    if(y_distance <= 0.0f || y_distance > state.body->step_height + STEP_TOLERANCE) {
        return false;
    }

    auto half_width = std::max(-state.body->local_box.min().x(), state.body->local_box.max().x());
    auto half_depth = std::max(-state.body->local_box.min().z(), state.body->local_box.max().z());
    auto height = state.body->local_box.max().y() - state.body->local_box.min().y();

    auto block_min_x = std::max(collider.box.min().x(), collider.box.max().x() - half_width);
    auto block_max_x = std::min(collider.box.max().x(), collider.box.min().x() + half_width);
    auto block_min_z = std::max(collider.box.min().z(), collider.box.max().z() - half_depth);
    auto block_max_z = std::min(collider.box.max().z(), collider.box.min().z() + half_depth);

    Eigen::AlignedBox3f adjusted;
    adjusted.min().x() = std::min(final_box.min().x(), block_min_x + COLLISION_ADJUSTMENT);
    adjusted.max().x() = std::max(final_box.max().x(), block_max_x - COLLISION_ADJUSTMENT);
    adjusted.min().y() = collider.box.max().y() + COLLISION_ADJUSTMENT;
    adjusted.max().y() = adjusted.min().y() + height;
    adjusted.min().z() = std::min(final_box.min().z(), block_min_z + COLLISION_ADJUSTMENT);
    adjusted.max().z() = std::max(final_box.max().z(), block_max_z - COLLISION_ADJUSTMENT);

    if(!can_slide_through(state.world, state.chunk, adjusted)) {
        return false;
    }

    update_y(state, adjusted.min().y() - state.body->local_box.min().y());

    state.on_ground = true;
    state.support = collider.contact;

    return true;
}

static void clip_x_max(MoveState& state, const Collider& collider, const Eigen::AlignedBox3f& final_box)
{
    if(collider.steppable && state.was_on_ground && try_step(state, collider, final_box)) {
        return;
    }

    auto position_x = collider.box.max().x() + COLLISION_ADJUSTMENT - state.body->local_box.min().x();

    update_x(state, position_x);

    state.contacts[0] = collider.contact;
}

static void clip_x_min(MoveState& state, const Collider& collider, const Eigen::AlignedBox3f& final_box)
{
    if(collider.steppable && state.was_on_ground && try_step(state, collider, final_box)) {
        return;
    }

    auto position_x = collider.box.min().x() - COLLISION_ADJUSTMENT - state.body->local_box.max().x();

    update_x(state, position_x);

    state.contacts[0] = collider.contact;
}

static void clip_z_max(MoveState& state, const Collider& collider, const Eigen::AlignedBox3f& final_box)
{
    if(collider.steppable && state.was_on_ground && try_step(state, collider, final_box)) {
        return;
    }

    auto position_z = collider.box.max().z() + COLLISION_ADJUSTMENT - state.body->local_box.min().z();

    update_z(state, position_z);

    state.contacts[2] = collider.contact;
}

static void clip_z_min(MoveState& state, const Collider& collider, const Eigen::AlignedBox3f& final_box)
{
    if(collider.steppable && state.was_on_ground && try_step(state, collider, final_box)) {
        return;
    }

    auto position_z = collider.box.min().z() - COLLISION_ADJUSTMENT - state.body->local_box.max().z();

    update_z(state, position_z);

    state.contacts[2] = collider.contact;
}

static void collect_colliders(World* world, const ChunkPos& chunk, const Eigen::AlignedBox3f& extent)
{
    s_colliders.clear();

    for_each_block(world, chunk, extent, [&extent](block_id_type id, const Eigen::Vector3f& relative) {
        if(id == BLOCK_ID_NULL) {
            return true;
        }

        auto bcoll = block_collisions::find(id);

        if(bcoll == nullptr || !bcoll->collides()) {
            return true;
        }

        for(const auto& element : bcoll->elements) {
            auto box = element;
            box.translate(relative);

            if(!box.intersects(extent)) {
                continue;
            }

            Collider collider;
            collider.box = box;
            collider.contact.block = id;
            collider.steppable = true;
            s_colliders.push_back(collider);
        }

        return true;
    });
}

static void collect_candidates(const Eigen::AlignedBox3f& entity_box, const Eigen::Vector3f& movement)
{
    s_candidates.clear();

    for(std::size_t i = 0; i < s_colliders.size(); ++i) {
        Eigen::Vector3f time;
        calc_time(movement, entity_box, s_colliders[i].box, time);

        if(time.x() > 1.0f || time.y() > 1.0f || time.z() > 1.0f) {
            continue;
        }

        Candidate candidate;
        candidate.index = i;
        candidate.t_squared = time.squaredNorm();
        s_candidates.push_back(candidate);
    }

    std::sort(s_candidates.begin(), s_candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.t_squared < b.t_squared;
    });
}

static void resolve(MoveState& state)
{
    for(const auto& candidate : s_candidates) {
        const auto& collider = s_colliders[candidate.index];

        Eigen::Vector3f time;
        calc_time(state.movement, state.entity_box, collider.box, time);

        if(time.x() > 1.0f || time.y() > 1.0f || time.z() > 1.0f) {
            continue;
        }

        Eigen::Vector3f offset = state.movement.cwiseProduct(time);
        auto final_box = state.entity_box;
        final_box.translate(offset);

        const auto& block = collider.box;

        if(!state.hit_y_min) {
            if(final_box.min().y() + COLLISION_ADJUSTMENT >= block.max().y()) {
                clip_y_max(state, collider);
            }
            else if(final_box.max().y() - COLLISION_ADJUSTMENT <= block.min().y()) {
                clip_y_min(state, collider);
            }
            else if(final_box.min().x() + COLLISION_ADJUSTMENT >= block.max().x()) {
                clip_x_max(state, collider, final_box);
            }
            else if(final_box.max().x() - COLLISION_ADJUSTMENT <= block.min().x()) {
                clip_x_min(state, collider, final_box);
            }
            else if(final_box.min().z() + COLLISION_ADJUSTMENT >= block.max().z()) {
                clip_z_max(state, collider, final_box);
            }
            else if(final_box.max().z() - COLLISION_ADJUSTMENT <= block.min().z()) {
                clip_z_min(state, collider, final_box);
            }

            continue;
        }

        if(final_box.min().x() + COLLISION_ADJUSTMENT >= block.max().x()) {
            clip_x_max(state, collider, final_box);
        }
        else if(final_box.max().x() - COLLISION_ADJUSTMENT <= block.min().x()) {
            clip_x_min(state, collider, final_box);
        }
        else if(final_box.min().z() + COLLISION_ADJUSTMENT >= block.max().z()) {
            clip_z_max(state, collider, final_box);
        }
        else if(final_box.max().z() - COLLISION_ADJUSTMENT <= block.min().z()) {
            clip_z_min(state, collider, final_box);
        }
        else if(final_box.min().y() + COLLISION_ADJUSTMENT >= block.max().y()) {
            clip_y_max(state, collider);
        }
        else if(final_box.max().y() - COLLISION_ADJUSTMENT <= block.min().y()) {
            clip_y_min(state, collider);
        }
    }
}

Eigen::AlignedBox3f physics::bounds(const Eigen::Vector3f& position, const Eigen::AlignedBox3f& local_box)
{
    auto result = local_box;
    result.translate(position);

    return result;
}

physics::Fluid physics::sample_fluid(World* world, const ChunkPos& chunk, const Eigen::Vector3f& position,
    const Eigen::AlignedBox3f& local_box)
{
    auto body = bounds(position, local_box);
    auto body_volume = local_box.sizes().prod();
    auto origin = utils::to_block(chunk);

    Fluid result;
    auto overlap_volume = 0.0f;

    for_each_block(world, chunk, body, [&](block_id_type id, const Eigen::Vector3f& relative) {
        if(id == BLOCK_ID_NULL) {
            return true;
        }

        auto definition = block_registry::find_definition(id);

        if(definition == nullptr || definition->fluid == FLUID_ID_NULL) {
            return true;
        }

        auto cell_min = relative;
        auto cell_max = relative + Eigen::Vector3f::Ones();

        auto overlap_min = body.min().cwiseMax(cell_min);
        auto overlap_max = body.max().cwiseMin(cell_max);

        if((overlap_min.array() >= overlap_max.array()).any()) {
            return true;
        }

        overlap_volume += (overlap_max - overlap_min).prod();

        auto fluid_def = fluid_registry::find_definition(definition->fluid);

        if(fluid_def == nullptr) {
            return true;
        }

        BlockPos bpos = origin + relative.cast<BlockPos::value_type>();

        auto lookup = [world](const BlockPos& pos) {
            return block_registry::find_definition(world->get_block(pos));
        };

        result.flow += fluid_flow::sample(definition, bpos, lookup, fluid_def->gravity);

        auto cell_top = relative.y() + 0.0625f * static_cast<float>(definition->fluid_level);
        auto covered = std::min(cell_top, body.max().y()) - body.min().y();
        result.submersion = std::max(result.submersion, covered);

        if(result.block == BLOCK_ID_NULL) {
            result.block = id;
            result.fluid = definition->fluid;
        }

        return true;
    });

    if(body_volume > 0.0f) {
        result.volume_fraction = std::min(overlap_volume / body_volume, 1.0f);
    }

    auto flow_length = result.flow.norm();

    if(flow_length > 1.0e-6f) {
        result.flow /= flow_length;
    }

    return result;
}

physics::MoveResult physics::move(World* world, const ChunkPos& chunk, const Eigen::Vector3f& position, const Eigen::Vector3f& movement,
    const Body& body, bool was_on_ground)
{
    MoveResult result;
    result.position = position;
    result.movement = movement;

    if(!movement.allFinite()) {
        result.movement = Eigen::Vector3f::Zero();
        return result;
    }

    Eigen::Vector3f limit = Eigen::Vector3f::Constant(MAX_MOVEMENT);
    Eigen::Vector3f delta = movement.cwiseMax(-limit).cwiseMin(limit);

    if(delta.isZero()) {
        return result;
    }

    result.movement = delta;

    auto entity_box = bounds(position, body.local_box);
    Eigen::Vector3f zero = Eigen::Vector3f::Zero();

    auto extent_box = entity_box;
    extent_box.min() += delta.cwiseMin(zero);
    extent_box.max() += delta.cwiseMax(zero);

    collect_colliders(world, chunk, extent_box);
    collect_candidates(entity_box, delta);

    MoveState state;
    state.world = world;
    state.chunk = chunk;
    state.body = &body;
    state.position = position;
    state.movement = delta;
    state.entity_box = entity_box;
    state.extent_box = extent_box;
    state.was_on_ground = was_on_ground;

    resolve(state);

    result.position = state.position + state.movement;
    result.movement = state.movement;
    result.axis_contacts = state.contacts;
    result.support = state.support;
    result.on_ground = state.on_ground;

    return result;
}
