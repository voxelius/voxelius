#include "client/pch.hh"

#include "client/world/bmodel_cache.hh"

#include "core/res/resource.hh"
#include "core/utils/angles.hh"

#include "shared/res/block_model.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/tint_registry.hh"

#include "client/world/block_atlas.hh"

constexpr static std::array ALL_FACES = {
    BLOCK_FACE_NORTH,
    BLOCK_FACE_SOUTH,
    BLOCK_FACE_EAST,
    BLOCK_FACE_WEST,
    BLOCK_FACE_TOP,
    BLOCK_FACE_BOTTOM,
};

constexpr static std::array FACE_ROTATIONS = {
    std::array { BLOCK_FACE_NORTH, BLOCK_FACE_SOUTH, BLOCK_FACE_EAST, BLOCK_FACE_WEST, BLOCK_FACE_TOP, BLOCK_FACE_BOTTOM },
    std::array { BLOCK_FACE_SOUTH, BLOCK_FACE_NORTH, BLOCK_FACE_WEST, BLOCK_FACE_EAST, BLOCK_FACE_TOP, BLOCK_FACE_BOTTOM },
    std::array { BLOCK_FACE_EAST, BLOCK_FACE_WEST, BLOCK_FACE_SOUTH, BLOCK_FACE_NORTH, BLOCK_FACE_TOP, BLOCK_FACE_BOTTOM },
    std::array { BLOCK_FACE_WEST, BLOCK_FACE_EAST, BLOCK_FACE_NORTH, BLOCK_FACE_SOUTH, BLOCK_FACE_TOP, BLOCK_FACE_BOTTOM },
    std::array { BLOCK_FACE_BOTTOM, BLOCK_FACE_TOP, BLOCK_FACE_EAST, BLOCK_FACE_WEST, BLOCK_FACE_NORTH, BLOCK_FACE_SOUTH },
    std::array { BLOCK_FACE_TOP, BLOCK_FACE_BOTTOM, BLOCK_FACE_EAST, BLOCK_FACE_WEST, BLOCK_FACE_SOUTH, BLOCK_FACE_NORTH },
};

static std::vector<std::unique_ptr<CachedBlockModel>> s_models;

static Eigen::Vector3f face_normal(block_face face)
{
    switch(face) {
        case BLOCK_FACE_NORTH:
            return -Eigen::Vector3f::UnitZ();

        case BLOCK_FACE_SOUTH:
            return Eigen::Vector3f::UnitZ();

        case BLOCK_FACE_EAST:
            return Eigen::Vector3f::UnitX();

        case BLOCK_FACE_WEST:
            return -Eigen::Vector3f::UnitX();

        case BLOCK_FACE_TOP:
            return Eigen::Vector3f::UnitY();

        case BLOCK_FACE_BOTTOM:
            return -Eigen::Vector3f::UnitY();
    }

    return Eigen::Vector3f::Zero();
}

static block_face rotate_face(block_face face, block_face facing)
{
    return FACE_ROTATIONS[facing][face];
}

static Eigen::Matrix3f angle_axis_rotation(float angle, const Eigen::Vector3f& axis)
{
    auto rotation = Eigen::AngleAxisf(angle, axis);

    return rotation.toRotationMatrix();
}

static Eigen::Matrix3f facing_rotation(block_face facing)
{
    constexpr auto pi = std::numbers::pi_v<float>;

    switch(facing) {
        case BLOCK_FACE_NORTH:
            return Eigen::Matrix3f::Identity();

        case BLOCK_FACE_SOUTH:
            return angle_axis_rotation(pi, Eigen::Vector3f::UnitY());

        case BLOCK_FACE_EAST:
            return angle_axis_rotation(-pi * 0.5f, Eigen::Vector3f::UnitY());

        case BLOCK_FACE_WEST:
            return angle_axis_rotation(pi * 0.5f, Eigen::Vector3f::UnitY());

        case BLOCK_FACE_TOP:
            return angle_axis_rotation(-pi * 0.5f, Eigen::Vector3f::UnitX());

        case BLOCK_FACE_BOTTOM:
            return angle_axis_rotation(pi * 0.5f, Eigen::Vector3f::UnitX());
    }

    return Eigen::Matrix3f::Identity();
}

static Eigen::Matrix3f element_rotation(const Eigen::Vector3f& angles)
{
    auto radians = utils::radians(angles);

    auto rx = Eigen::AngleAxisf(radians.x(), Eigen::Vector3f::UnitX());
    auto ry = Eigen::AngleAxisf(radians.y(), Eigen::Vector3f::UnitY());
    auto rz = Eigen::AngleAxisf(radians.z(), Eigen::Vector3f::UnitZ());

    return (rz * ry * rx).toRotationMatrix();
}

static std::uint32_t pack_normal_2_10_10_10(const Eigen::Vector3f& normal)
{
    auto snx = static_cast<std::int32_t>(std::clamp(normal.x() * 511.0f, -511.0f, 511.0f));
    auto sny = static_cast<std::int32_t>(std::clamp(normal.y() * 511.0f, -511.0f, 511.0f));
    auto snz = static_cast<std::int32_t>(std::clamp(normal.z() * 511.0f, -511.0f, 511.0f));

    auto nx = static_cast<std::uint32_t>(snx) & 0x3FF;
    auto ny = static_cast<std::uint32_t>(sny) & 0x3FF;
    auto nz = static_cast<std::uint32_t>(snz) & 0x3FF;

    return (nz << 20) | (ny << 10) | nx;
}

static std::int32_t unpack_normal_component(std::uint32_t packed_normal, unsigned shift)
{
    auto bits = (packed_normal >> shift) & 0x3FFU;

    return static_cast<std::int32_t>(bits << 22U) >> 22U;
}

static void make_face_geometry(const Eigen::Vector3f& min, const Eigen::Vector3f& max, block_face face, CachedBlockModel_Quad& quad)
{
    quad.positions.fill(Eigen::Vector3f::Zero());

    switch(face) {
        case BLOCK_FACE_NORTH:
            quad.positions[0] = Eigen::Vector3f(min.x(), max.y(), min.z());
            quad.positions[1] = Eigen::Vector3f(max.x(), max.y(), min.z());
            quad.positions[2] = Eigen::Vector3f(max.x(), min.y(), min.z());
            quad.positions[3] = Eigen::Vector3f(min.x(), min.y(), min.z());
            break;

        case BLOCK_FACE_SOUTH:
            quad.positions[0] = Eigen::Vector3f(max.x(), max.y(), max.z());
            quad.positions[1] = Eigen::Vector3f(min.x(), max.y(), max.z());
            quad.positions[2] = Eigen::Vector3f(min.x(), min.y(), max.z());
            quad.positions[3] = Eigen::Vector3f(max.x(), min.y(), max.z());
            break;

        case BLOCK_FACE_EAST:
            quad.positions[0] = Eigen::Vector3f(max.x(), max.y(), min.z());
            quad.positions[1] = Eigen::Vector3f(max.x(), max.y(), max.z());
            quad.positions[2] = Eigen::Vector3f(max.x(), min.y(), max.z());
            quad.positions[3] = Eigen::Vector3f(max.x(), min.y(), min.z());
            break;

        case BLOCK_FACE_WEST:
            quad.positions[0] = Eigen::Vector3f(min.x(), max.y(), max.z());
            quad.positions[1] = Eigen::Vector3f(min.x(), max.y(), min.z());
            quad.positions[2] = Eigen::Vector3f(min.x(), min.y(), min.z());
            quad.positions[3] = Eigen::Vector3f(min.x(), min.y(), max.z());
            break;

        case BLOCK_FACE_TOP:
            quad.positions[0] = Eigen::Vector3f(min.x(), max.y(), min.z());
            quad.positions[1] = Eigen::Vector3f(min.x(), max.y(), max.z());
            quad.positions[2] = Eigen::Vector3f(max.x(), max.y(), max.z());
            quad.positions[3] = Eigen::Vector3f(max.x(), max.y(), min.z());
            break;

        case BLOCK_FACE_BOTTOM:
            quad.positions[0] = Eigen::Vector3f(min.x(), min.y(), max.z());
            quad.positions[1] = Eigen::Vector3f(min.x(), min.y(), min.z());
            quad.positions[2] = Eigen::Vector3f(max.x(), min.y(), min.z());
            quad.positions[3] = Eigen::Vector3f(max.x(), min.y(), max.z());
            break;
    }

    switch(face) {
        case BLOCK_FACE_TOP:
            quad.uvs[0] = Eigen::Vector2f(0.0f, 0.0f);
            quad.uvs[1] = Eigen::Vector2f(0.0f, 1.0f);
            quad.uvs[2] = Eigen::Vector2f(1.0f, 1.0f);
            quad.uvs[3] = Eigen::Vector2f(1.0f, 0.0f);
            break;

        case BLOCK_FACE_BOTTOM:
            quad.uvs[0] = Eigen::Vector2f(1.0f, 1.0f);
            quad.uvs[1] = Eigen::Vector2f(1.0f, 0.0f);
            quad.uvs[2] = Eigen::Vector2f(0.0f, 0.0f);
            quad.uvs[3] = Eigen::Vector2f(0.0f, 1.0f);
            break;

        default:
            quad.uvs[0] = Eigen::Vector2f(1.0f, 0.0f);
            quad.uvs[1] = Eigen::Vector2f(0.0f, 0.0f);
            quad.uvs[2] = Eigen::Vector2f(0.0f, 1.0f);
            quad.uvs[3] = Eigen::Vector2f(1.0f, 1.0f);
            break;
    }
}

static void face_uv_extent(const Eigen::Vector3f& min, const Eigen::Vector3f& max, block_face face, Eigen::Vector4f& out_uv)
{
    switch(face) {
        case BLOCK_FACE_NORTH:
            out_uv = Eigen::Vector4f(1.0f - min.x(), 1.0f - max.y(), 1.0f - max.x(), 1.0f - min.y());
            break;

        case BLOCK_FACE_SOUTH:
            out_uv = Eigen::Vector4f(max.x(), 1.0f - max.y(), min.x(), 1.0f - min.y());
            break;

        case BLOCK_FACE_EAST:
            out_uv = Eigen::Vector4f(1.0f - min.z(), 1.0f - max.y(), 1.0f - max.z(), 1.0f - min.y());
            break;

        case BLOCK_FACE_WEST:
            out_uv = Eigen::Vector4f(max.z(), 1.0f - max.y(), min.z(), 1.0f - min.y());
            break;

        case BLOCK_FACE_TOP:
        case BLOCK_FACE_BOTTOM:
            out_uv = Eigen::Vector4f(min.x(), min.z(), max.x(), max.z());
            break;
    }
}

static void apply_face_uv(const BlockModel_Face* face, const Eigen::Vector3f& min, const Eigen::Vector3f& max, block_face face_type,
    bool rescale, CachedBlockModel_Quad& quad)
{
    if(face->uv.has_value() || !rescale) {
        float u_min, v_min, u_max, v_max;

        if(face->uv.has_value()) {
            u_min = face->uv->x();
            v_min = face->uv->y();
            u_max = face->uv->z();
            v_max = face->uv->w();
        }
        else {
            Eigen::Vector4f uv;
            face_uv_extent(min, max, face_type, uv);
            u_min = uv.x();
            v_min = uv.y();
            u_max = uv.z();
            v_max = uv.w();
        }

        switch(face_type) {
            case BLOCK_FACE_TOP:
                quad.uvs[0] = Eigen::Vector2f(u_min, v_min);
                quad.uvs[1] = Eigen::Vector2f(u_min, v_max);
                quad.uvs[2] = Eigen::Vector2f(u_max, v_max);
                quad.uvs[3] = Eigen::Vector2f(u_max, v_min);
                break;

            case BLOCK_FACE_BOTTOM:
                quad.uvs[0] = Eigen::Vector2f(u_max, v_max);
                quad.uvs[1] = Eigen::Vector2f(u_max, v_min);
                quad.uvs[2] = Eigen::Vector2f(u_min, v_min);
                quad.uvs[3] = Eigen::Vector2f(u_min, v_max);
                break;

            default:
                quad.uvs[0] = Eigen::Vector2f(u_min, v_min);
                quad.uvs[1] = Eigen::Vector2f(u_max, v_min);
                quad.uvs[2] = Eigen::Vector2f(u_max, v_max);
                quad.uvs[3] = Eigen::Vector2f(u_min, v_max);
                break;
        }
    }

    auto steps = static_cast<std::ptrdiff_t>(face->uv_rotation / 90);

    std::rotate(quad.uvs.begin(), quad.uvs.begin() + steps, quad.uvs.end());
}

static void rotate_aabb(const Eigen::Matrix3f& rot, const Eigen::Vector3f& center, const Eigen::Vector3f& min, const Eigen::Vector3f& max,
    Eigen::Vector3f& out_min, Eigen::Vector3f& out_max)
{
    constexpr static auto CORNERS = 8;

    out_min = Eigen::Vector3f::Constant(std::numeric_limits<float>::max());
    out_max = Eigen::Vector3f::Constant(std::numeric_limits<float>::lowest());

    for(int i = 0; i < CORNERS; ++i) {
        Eigen::Vector3f corner;

        if(i & 1) {
            corner.x() = max.x();
        }
        else {
            corner.x() = min.x();
        }

        if(i & 2) {
            corner.y() = max.y();
        }
        else {
            corner.y() = min.y();
        }

        if(i & 4) {
            corner.z() = max.z();
        }
        else {
            corner.z() = min.z();
        }

        Eigen::Vector3f rotated = center + rot * (corner - center);
        out_min = out_min.cwiseMin(rotated);
        out_max = out_max.cwiseMax(rotated);
    }
}

static bool is_covering(const Eigen::Vector3f& min, const Eigen::Vector3f& max, block_face face, bool axis_aligned)
{
    auto result = true;

    switch(face) {
        case BLOCK_FACE_NORTH:
            result = result && min.x() <= 0.0f && max.x() >= 1.0f;
            result = result && min.y() <= 0.0f && max.y() >= 1.0f;
            result = result && min.z() <= 0.0f;
            break;

        case BLOCK_FACE_SOUTH:
            result = result && min.x() <= 0.0f && max.x() >= 1.0f;
            result = result && min.y() <= 0.0f && max.y() >= 1.0f;
            result = result && max.z() >= 1.0f;
            break;

        case BLOCK_FACE_EAST:
            result = result && min.z() <= 0.0f && max.z() >= 1.0f;
            result = result && min.y() <= 0.0f && max.y() >= 1.0f;
            result = result && max.x() >= 1.0f;
            break;

        case BLOCK_FACE_WEST:
            result = result && min.z() <= 0.0f && max.z() >= 1.0f;
            result = result && min.y() <= 0.0f && max.y() >= 1.0f;
            result = result && min.x() <= 0.0f;
            break;

        case BLOCK_FACE_TOP:
            result = result && min.x() <= 0.0f && max.x() >= 1.0f;
            result = result && min.z() <= 0.0f && max.z() >= 1.0f;
            result = result && max.y() >= 1.0f;
            break;

        case BLOCK_FACE_BOTTOM:
            result = result && min.x() <= 0.0f && max.x() >= 1.0f;
            result = result && min.z() <= 0.0f && max.z() >= 1.0f;
            result = result && min.y() <= 0.0f;
            break;

        default:
            result = false;
            break;
    }

    return result && axis_aligned;
}

static std::unique_ptr<CachedBlockModel> bake_model(const BlockDefinition& def)
{
    if(def.is_stem || def.model_name.is_empty()) {
        return nullptr;
    }

    auto model = res::load<BlockModel>(def.model_name, "models/block", ".json");

    if(model == nullptr) {
        msg::log_warning("{}: load failed", def.model_name.full_string());
        return nullptr;
    }

    auto baked = std::make_unique<CachedBlockModel>();
    baked->fully_covered.fill(false);

    auto facing_rot = facing_rotation(def.model_facing);
    auto center = Eigen::Vector3f::Constant(0.5f);

    for(auto& element : model->elements) {
        auto element_rot = element_rotation(element.rotation_angles);

        for(auto face : ALL_FACES) {
            if(!element.faces[face].has_value()) {
                continue;
            }

            auto& face_def = element.faces[face].value();
            auto rotated_face = rotate_face(face, def.model_facing);
            auto model_locked = face_def.uv.has_value();
            auto texture_face = &face_def;

            if(!model_locked && element.faces[rotated_face].has_value()) {
                texture_face = &element.faces[rotated_face].value();
            }

            auto albedo_frames = def.resolve_albedo_slot(texture_face->texture_slot);

            if(!albedo_frames.has_value()) {
                msg::log_warning("{}: {}: missing albedo textures", def.model_name.full_string(), texture_face->texture_slot);
                continue;
            }

            auto albedo_strip = block_atlas::find(albedo_frames.value());

            if(albedo_strip == nullptr) {
                msg::log_warning("{}: {}: albedo strip not found", def.model_name.full_string(), texture_face->texture_slot);
                continue;
            }

            CachedBlockModel_Quad quad {};

            if(model_locked) {
                make_face_geometry(element.min, element.max, face, quad);
                apply_face_uv(texture_face, element.min, element.max, face, element.rescale, quad);

                for(auto& position : quad.positions) {
                    position = center + facing_rot * (position - center);
                }
            }
            else {
                Eigen::Vector3f used_min = element.min;
                Eigen::Vector3f used_max = element.max;
                auto used_face = face;

                if(def.model_facing != BLOCK_FACE_NORTH) {
                    rotate_aabb(facing_rot, center, element.min, element.max, used_min, used_max);
                    used_face = rotated_face;
                }

                make_face_geometry(used_min, used_max, used_face, quad);
                apply_face_uv(texture_face, used_min, used_max, used_face, element.rescale, quad);
            }

            auto face_tint = tint_registry::find(texture_face->tint_name);
            auto mask_id = def.resolve_mask_slot(texture_face->texture_slot);

            quad.albedo_strip = static_cast<std::uint32_t>(albedo_strip->index);
            quad.albedo_frames = static_cast<std::uint32_t>(albedo_strip->frame_count);
            quad.tint = face_tint;
            quad.animated = def.animated;
            quad.shade = element.shade;

            auto mask_strip = block_atlas::stub_white;

            if(mask_id.has_value()) {
                auto mask_frames = std::array { mask_id.value() };

                mask_strip = block_atlas::find(mask_frames);

                if(mask_strip == nullptr) {
                    mask_strip = block_atlas::stub_white;
                }
            }

            quad.mask_frame = static_cast<std::uint32_t>(mask_strip->frame_base);

            for(auto& position : quad.positions) {
                position = element_rot * (position - element.rotation_origin) + element.rotation_origin;
                position += def.model_offset;
            }

            auto normal = face_normal(face);
            auto transformed_normal = facing_rot * (element_rot * normal);

            quad.packed_normal = pack_normal_2_10_10_10(transformed_normal);

            if(face_def.cull_face.has_value()) {
                baked->face_quads[rotated_face].push_back(quad);

                if(is_covering(element.min, element.max, face, element.rotation_angles.isZero())) {
                    baked->fully_covered[rotated_face] = true;
                }
            }
            else {
                baked->unculled_quads.push_back(quad);
            }
        }
    }

    return baked;
}

void bmodel_cache::init_late(void)
{
    auto definitions = block_registry::all_definitions();

    s_models.clear();
    s_models.resize(definitions.size());

    for(block_id_type id = 0; id < definitions.size(); id += 1) {
        s_models[id] = bake_model(definitions[id]);
    }
}

void bmodel_cache::shutdown(void)
{
    s_models.clear();
}

const CachedBlockModel* bmodel_cache::find(block_id_type id)
{
    if(id == BLOCK_ID_NULL || id >= s_models.size()) {
        return nullptr;
    }

    return s_models[id].get();
}

block_face bmodel_cache::face_from_normal(std::uint32_t packed_normal)
{
    auto nx = unpack_normal_component(packed_normal, 0);
    auto ny = unpack_normal_component(packed_normal, 10);
    auto nz = unpack_normal_component(packed_normal, 20);

    auto ax = std::abs(nx);
    auto ay = std::abs(ny);
    auto az = std::abs(nz);

    if(ay >= ax && ay >= az) {
        if(ny > 0) {
            return BLOCK_FACE_TOP;
        }

        return BLOCK_FACE_BOTTOM;
    }

    if(ax >= az) {
        if(nx > 0) {
            return BLOCK_FACE_EAST;
        }

        return BLOCK_FACE_WEST;
    }

    if(nz > 0) {
        return BLOCK_FACE_SOUTH;
    }

    return BLOCK_FACE_NORTH;
}
