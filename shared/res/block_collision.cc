#include "shared/pch.hh"

#include "shared/res/block_collision.hh"

#include "core/res/resource.hh"
#include "core/utils/json.hh"
#include "core/utils/physfs.hh"

static std::optional<Eigen::AlignedBox3f> parse_aabb(const JSON_Object* object)
{
    if(object == nullptr) {
        return std::nullopt;
    }

    auto min = utils::parse_vector<float, 3>(object, "min");
    auto max = utils::parse_vector<float, 3>(object, "max");

    if(min.has_value() && max.has_value()) {
        Eigen::AlignedBox3f aabb {};
        aabb.min() = 0.0625f * min.value();
        aabb.max() = 0.0625f * max.value();

        return aabb;
    }

    return std::nullopt;
}

static const void* block_collision_load_fn(const char* path, std::uint32_t)
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

    auto elements = json_object_get_array(json, "elements");
    auto num_elements = json_array_get_count(elements);

    BlockCollision collision {};
    collision.elements.reserve(num_elements);

    for(std::size_t i = 0; i < num_elements; ++i) {
        auto element = json_array_get_object(elements, i);
        auto aabb = parse_aabb(element);

        if(!aabb.has_value()) {
            msg::log_warning("{}: invalid element at index {}", path, i);
            json_value_free(jsonv);
            return nullptr;
        }

        collision.bounds.extend(aabb.value());
        collision.elements.push_back(std::move(aabb.value()));
    }

    json_value_free(jsonv);

    return new BlockCollision(std::move(collision));
}

static void block_collision_free_fn(const void* resource)
{
    delete static_cast<const BlockCollision*>(resource);
}

void BlockCollision::register_resource(void)
{
    res::register_loader<BlockCollision>(&block_collision_load_fn, &block_collision_free_fn);
}
