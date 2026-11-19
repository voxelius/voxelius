#include "server/pch.hh"

#include "server/world/lod_color.hh"

#include "core/res/image.hh"
#include "core/res/resource.hh"

#include "shared/res/block_model.hh"
#include "shared/world/biome_registry.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/fluid_registry.hh"
#include "shared/world/tint_registry.hh"

struct BaseColor final {
    Eigen::Vector3f rgb;
    tint_id_type tint;
    bool apply_tint;
};

static std::mutex s_mutex;
static vx::hash_map<block_id_type, BaseColor> s_colors;

static bool average_images(std::span<const Identifier> textures, const char* subdirectory, Eigen::Vector3f& out)
{
    Eigen::Vector3f sum = Eigen::Vector3f::Zero();
    auto count = std::size_t { 0 };

    for(const auto& id : textures) {
        auto image = res::load<Image>(id, subdirectory);

        if(image == nullptr) {
            continue;
        }

        auto pixels = image->pixels;
        auto pixel_count = static_cast<std::size_t>(image->width) * static_cast<std::size_t>(image->height);

        for(std::size_t i = 0; i < pixel_count; ++i) {
            auto* pixel = pixels + i * 4;
            sum += Eigen::Vector3f(static_cast<float>(pixel[0]), static_cast<float>(pixel[1]), static_cast<float>(pixel[2]));
        }

        count += pixel_count;
    }

    if(count == 0) {
        return false;
    }

    out = sum / (255.0f * static_cast<float>(count));
    return true;
}

static bool find_model_color(const BlockDefinition& def, Eigen::Vector3f& out, tint_id_type& tint)
{
    if(def.model_name.is_empty()) {
        return false;
    }

    auto model = res::load<BlockModel>(def.model_name, "models/block", ".json");

    if(model == nullptr) {
        return false;
    }

    const BlockModel_Face* chosen = nullptr;

    for(const auto& element : model->elements) {
        auto& top = element.faces[BLOCK_FACE_TOP];

        if(!top.has_value()) {
            continue;
        }

        if(chosen == nullptr || !top->tint_name.is_empty()) {
            chosen = &top.value();
        }
    }

    if(chosen == nullptr) {
        for(const auto& element : model->elements) {
            for(const auto& face : element.faces) {
                if(face.has_value()) {
                    chosen = &face.value();
                    break;
                }
            }

            if(chosen) {
                break;
            }
        }
    }

    if(chosen == nullptr) {
        return false;
    }

    auto frames = def.resolve_albedo_slot(chosen->texture_slot);

    if(!frames.has_value()) {
        return false;
    }

    if(!average_images(frames.value(), "textures/block", out)) {
        return false;
    }

    tint = tint_registry::find(chosen->tint_name);
    return true;
}

static bool find_fluid_color(const BlockDefinition& def, Eigen::Vector3f& out, tint_id_type& tint)
{
    auto fluid = fluid_registry::find_definition(def.fluid);

    if(fluid == nullptr) {
        return false;
    }

    if(!average_images(fluid->albedo_still, "textures/block", out)) {
        return false;
    }

    tint = fluid->tint;
    return true;
}

static BaseColor resolve(block_id_type id)
{
    BaseColor result {};
    result.rgb = Eigen::Vector3f::Constant(0.5f);
    result.tint = TINT_ID_NULL;
    result.apply_tint = false;

    auto def = block_registry::find_definition(id);

    if(def == nullptr) {
        return result;
    }

    if(def->lod_color.has_value()) {
        result.rgb = def->lod_color.value();
        return result;
    }

    auto is_fluid = def->fluid && def->fluid_level > 0;

    if(def->render == BLOCK_RENDER_NONE && !is_fluid) {
        return result;
    }

    Eigen::Vector3f color = Eigen::Vector3f::Zero();
    auto tint = TINT_ID_NULL;
    if(find_model_color(*def, color, tint) || find_fluid_color(*def, color, tint)) {
        result.rgb = color;
        result.tint = tint;
        result.apply_tint = static_cast<bool>(tint);
    }

    return result;
}

void lod_color::init(void)
{
    // Resolve every definition here, on the main thread: the resource loader
    // is not thread-safe and chunk LOD tasks run on the pool.
    auto definitions = block_registry::all_definitions();

    std::scoped_lock lock(s_mutex);
    s_colors.clear();

    for(block_id_type id = 0; id < definitions.size(); id += 1) {
        s_colors.insert_or_assign(id, resolve(id));
    }
}

void lod_color::shutdown(void)
{
    // The palette is immutable after init and stays in place: a LOD task that
    // is still winding down may read it, and it owns nothing outside itself.
}

Eigen::Vector3f lod_color::block_color(block_id_type id, biome_id_type biome)
{
    // The cache is resolved once on the main thread and never written again;
    // LOD workers only read it.
    auto it = s_colors.find(id);

    if(it == s_colors.end()) {
        return Eigen::Vector3f::Constant(0.5f);
    }

    const auto& base = it->second;

    if(!base.apply_tint) {
        return base.rgb;
    }

    Eigen::Vector3f tint = Eigen::Vector3f::Ones();

    if(auto tint_def = tint_registry::find_definition(base.tint)) {
        tint = tint_def->default_color;
    }

    if(biome) {
        if(auto biome_def = biome_registry::find_definition(biome)) {
            if(base.tint < biome_def->tint_colors.size()) {
                tint = biome_def->tint_colors[base.tint];
            }
        }
    }

    return base.rgb.cwiseProduct(tint);
}
