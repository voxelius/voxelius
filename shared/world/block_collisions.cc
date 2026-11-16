#include "shared/pch.hh"

#include "shared/world/block_collisions.hh"

#include "core/res/resource.hh"

#include "shared/res/block_collision.hh"
#include "shared/world/block_registry.hh"

static std::vector<std::unique_ptr<BakedBlockCollision>> s_collisions;

static Eigen::Matrix3f facing_rotation(block_face facing)
{
    constexpr auto pi = std::numbers::pi_v<float>;

    switch(facing) {
        case BLOCK_FACE_NORTH:
            return Eigen::Matrix3f::Identity();

        case BLOCK_FACE_SOUTH:
            return Eigen::AngleAxisf(pi, Eigen::Vector3f::UnitY()).toRotationMatrix();

        case BLOCK_FACE_EAST:
            return Eigen::AngleAxisf(-pi * 0.5f, Eigen::Vector3f::UnitY()).toRotationMatrix();

        case BLOCK_FACE_WEST:
            return Eigen::AngleAxisf(pi * 0.5f, Eigen::Vector3f::UnitY()).toRotationMatrix();

        case BLOCK_FACE_TOP:
            return Eigen::AngleAxisf(-pi * 0.5f, Eigen::Vector3f::UnitX()).toRotationMatrix();

        case BLOCK_FACE_BOTTOM:
            return Eigen::AngleAxisf(pi * 0.5f, Eigen::Vector3f::UnitX()).toRotationMatrix();
    }

    return Eigen::Matrix3f::Identity();
}

static Eigen::AlignedBox3f transform_aabb(const Eigen::AlignedBox3f& aabb, const Eigen::Matrix3f& rot, const Eigen::Vector3f& offset)
{
    Eigen::Vector3f center = Eigen::Vector3f::Constant(0.5f);
    Eigen::AlignedBox3f transformed;

    for(Eigen::Index i = 0; i < 8; ++i) {
        Eigen::Vector3f corner = aabb.corner(static_cast<Eigen::AlignedBox3f::CornerType>(i));
        Eigen::Vector3f rotated = center + rot * (corner - center);
        transformed.extend(rotated + offset);
    }

    return transformed;
}

static std::unique_ptr<BakedBlockCollision> bake_collision(const BlockDefinition& def)
{
    if(def.is_stem || def.bcoll_name.is_empty()) {
        return nullptr;
    }

    auto collision = res::load<BlockCollision>(def.bcoll_name, "collisions/block", ".json");

    if(collision == nullptr) {
        msg::log_warning("{}: load failed", def.bcoll_name.full_string());
        return nullptr;
    }

    auto rot = facing_rotation(def.bcoll_facing);
    auto baked = std::make_unique<BakedBlockCollision>();

    for(const auto& element : collision->elements) {
        Eigen::AlignedBox3f transformed = transform_aabb(element, rot, def.bcoll_offset);
        baked->elements.push_back(transformed);
        baked->bounds.extend(transformed);
    }

    return baked;
}

void block_collisions::init_late(void)
{
    auto definitions = block_registry::all_definitions();

    s_collisions.clear();
    s_collisions.resize(definitions.size());

    for(block_id_type id = 0; id < definitions.size(); id += 1) {
        s_collisions[id] = bake_collision(definitions[id]);
    }
}

void block_collisions::shutdown(void)
{
    s_collisions.clear();
}

const BakedBlockCollision* block_collisions::find(block_id_type id)
{
    if(id == BLOCK_ID_NULL || id >= s_collisions.size()) {
        return nullptr;
    }

    return s_collisions[id].get();
}
