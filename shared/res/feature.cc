#include "shared/pch.hh"

#include "shared/res/feature.hh"

#include "core/res/resource.hh"
#include "core/utils/json.hh"
#include "core/utils/physfs.hh"

#include "shared/world/block_registry.hh"

constexpr static std::array BLOCK_TAG_MAPPING = {
    std::make_pair(std::string_view("gas"), static_cast<unsigned>(BLOCK_TAG_GAS)),
    std::make_pair(std::string_view("rock"), static_cast<unsigned>(BLOCK_TAG_ROCK)),
    std::make_pair(std::string_view("soil"), static_cast<unsigned>(BLOCK_TAG_SOIL)),
    std::make_pair(std::string_view("turf"), static_cast<unsigned>(BLOCK_TAG_TURF)),
    std::make_pair(std::string_view("xfoil"), static_cast<unsigned>(BLOCK_TAG_XFOIL)),
    std::make_pair(std::string_view("sfoil"), static_cast<unsigned>(BLOCK_TAG_SFOIL)),
    std::make_pair(std::string_view("wood"), static_cast<unsigned>(BLOCK_TAG_WOOD)),
};

static block_id_type parse_palette_entry(const JSON_Object* object)
{
    auto id_raw = json_object_get_string(object, "block");

    if(id_raw == nullptr) {
        return BLOCK_ID_NULL;
    }

    auto id = Identifier::from_string(id_raw);

    if(!id.is_valid()) {
        return BLOCK_ID_NULL;
    }

    auto family = block_registry::find_family(id);

    if(family == nullptr) {
        return block_registry::find(id);
    }

    vx::hash_map<blockstate_key_type, blockstate_val_type> states;

    auto json_states = json_object_get_object(object, "states");
    auto json_states_count = json_object_get_count(json_states);

    if(json_states == nullptr || json_states_count == 0) {
        return family->default_variant;
    }

    for(std::size_t i = 0; i < json_states_count; ++i) {
        auto key_raw = json_object_get_name(json_states, i);
        auto value_raw = json_object_get_value_at(json_states, i);
        auto value_str = json_value_get_string(value_raw);

        if(key_raw == nullptr || value_str == nullptr) {
            return BLOCK_ID_NULL;
        }

        auto key = family->state_hash(key_raw);
        auto value = family->state_hash(value_str);

        states.insert_or_assign(blockstate_key_type(key), blockstate_val_type(value));
    }

    return block_registry::resolve_variant(family->stem_id, states);
}

static std::optional<std::vector<block_id_type>> parse_palette(const JSON_Array* array)
{
    auto count = json_array_get_count(array);

    if(array == nullptr || count == 0) {
        return std::nullopt;
    }

    std::vector<block_id_type> palette;
    palette.reserve(count);

    for(std::size_t i = 0; i < count; ++i) {
        auto value = json_array_get_object(array, i);

        if(value == nullptr) {
            return std::nullopt;
        }

        auto entry = parse_palette_entry(value);

        if(entry == BLOCK_ID_NULL) {
            return std::nullopt;
        }

        palette.emplace_back(entry);
    }

    return palette;
}

static std::optional<Feature_Part> parse_part(const JSON_Object* object, std::span<const block_id_type> palette)
{
    if(object == nullptr) {
        return std::nullopt;
    }

    auto block_index = utils::parse_arithmetic<unsigned>(object, "block");

    if(!block_index.has_value() || block_index.value() >= palette.size()) {
        return std::nullopt;
    }

    auto offset = utils::parse_vector<int, 3>(object, "offset");

    if(!offset.has_value()) {
        return std::nullopt;
    }

    auto overwrite = utils::parse_bitmask<unsigned>(object, "overwrite", BLOCK_TAG_MAPPING);

    Feature_Part part {};
    part.block = palette[block_index.value()];
    part.offset = offset.value();
    part.overwrite = static_cast<block_tag_bit>(overwrite.value_or(0));

    return part;
}

static std::optional<std::vector<Feature_Part>> parse_parts(const JSON_Array* array, std::span<const block_id_type> palette,
    Eigen::AlignedBox3i& bounds)
{
    auto count = json_array_get_count(array);

    if(array == nullptr || count == 0) {
        return std::nullopt;
    }

    std::vector<Feature_Part> parts;
    parts.reserve(count);

    for(std::size_t i = 0; i < count; ++i) {
        auto value = json_array_get_object(array, i);
        auto part = parse_part(value, palette);

        if(!part.has_value()) {
            return std::nullopt;
        }

        bounds.extend(part->offset);
        parts.emplace_back(std::move(part.value()));
    }

    return parts;
}

static const void* feature_load_fn(const char* path, std::uint32_t)
{
    std::string source;

    if(!utils::read_file(path, source)) {
        msg::log_warning("{}: {}", path, utils::physfs_error());
        return nullptr;
    }

    auto jsonv = json_parse_string(source.c_str());
    auto json = json_value_get_object(jsonv);

    if(jsonv == nullptr) {
        msg::log_warning("{}: parse error", path);
        return nullptr;
    }

    if(json == nullptr) {
        msg::log_warning("{}: malformed JSON", path);
        json_value_free(jsonv);
        return nullptr;
    }

    auto palette_array = json_object_get_array(json, "palette");
    auto palette = parse_palette(palette_array);

    if(!palette.has_value()) {
        msg::log_warning("{}: invalid or missing palette", path);
        json_value_free(jsonv);
        return nullptr;
    }

    Eigen::AlignedBox3i bounds;

    auto parts_array = json_object_get_array(json, "parts");
    auto parts = parse_parts(parts_array, palette.value(), bounds);

    if(!parts.has_value()) {
        msg::log_warning("{}: invalid or missing parts", path);
        json_value_free(jsonv);
        return nullptr;
    }

    auto feature = new Feature {};
    feature->parts = std::move(parts.value());
    feature->bounds = std::move(bounds);
    return feature;
}

static void feature_free_fn(const void* ptr)
{
    delete reinterpret_cast<const Feature*>(ptr);
}

void Feature::register_resource(void)
{
    res::register_loader<Feature>(&feature_load_fn, &feature_free_fn);
}
