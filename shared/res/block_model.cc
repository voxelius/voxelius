#include "shared/pch.hh"

#include "shared/res/block_model.hh"

#include "core/res/resource.hh"
#include "core/utils/json.hh"
#include "core/utils/physfs.hh"

constexpr static float ROTATION_STEP = 15.0f;
constexpr static float ROTATION_MIN = -180.0f;
constexpr static float ROTATION_MAX = +180.0f;

constexpr static std::array BLOCK_FACE_MAPPING = {
    std::make_pair(std::string_view("north"), unsigned(BLOCK_FACE_NORTH)),
    std::make_pair(std::string_view("south"), unsigned(BLOCK_FACE_SOUTH)),
    std::make_pair(std::string_view("east"), unsigned(BLOCK_FACE_EAST)),
    std::make_pair(std::string_view("west"), unsigned(BLOCK_FACE_WEST)),
    std::make_pair(std::string_view("top"), unsigned(BLOCK_FACE_TOP)),
    std::make_pair(std::string_view("bottom"), unsigned(BLOCK_FACE_BOTTOM)),
};

static float snap_rotation(float degrees)
{
    auto clamped = std::clamp(degrees, ROTATION_MIN, ROTATION_MAX);

    return std::round(clamped / ROTATION_STEP) * ROTATION_STEP;
}

static std::optional<std::vector<std::string>> parse_texture_slots(const JSON_Array* array)
{
    auto count = json_array_get_count(array);

    if(array == nullptr || count == 0) {
        return std::nullopt;
    }

    std::vector<std::string> texture_slots;
    texture_slots.reserve(count);

    for(std::size_t i = 0; i < count; ++i) {
        auto value = json_array_get_string(array, i);

        if(value == nullptr) {
            return std::nullopt;
        }

        texture_slots.emplace_back(value);
    }

    return texture_slots;
}

static std::optional<BlockModelFace> parse_face(const JSON_Object* object)
{
    if(object == nullptr) {
        return std::nullopt;
    }

    auto texture = json_object_get_string(object, "texture");

    if(texture == nullptr) {
        return std::nullopt;
    }

    auto uv_rotation = utils::parse_arithmetic<unsigned>(object, "uv_rotation");

    BlockModelFace face {};
    face.texture_slot = texture;
    face.uv_rotation = uv_rotation.value_or(0);

    auto uv = utils::parse_vector<float, 4>(object, "uv");
    auto cull_face = utils::parse_enum<unsigned>(object, "cullface", BLOCK_FACE_MAPPING);
    auto tint_name = json_object_get_string(object, "tint");

    if(uv.has_value()) {
        face.uv = 0.0625f * uv.value();
    }

    if(cull_face.has_value()) {
        face.cull_face = static_cast<block_face>(cull_face.value());
    }

    if(tint_name) {
        face.tint_name = Identifier::from_string(tint_name);
    }

    return face;
}

static std::optional<BlockModelElement> parse_element(const JSON_Object* object)
{
    if(object == nullptr) {
        return std::nullopt;
    }

    auto min = utils::parse_vector<float, 3>(object, "min");
    auto max = utils::parse_vector<float, 3>(object, "max");
    auto origin = utils::parse_vector<float, 3>(object, "origin");
    auto rotation = utils::parse_vector<float, 3>(object, "rotation");

    if(min.has_value() && max.has_value()) {
        BlockModelElement element {};
        element.min = 0.0625f * min.value();
        element.max = 0.0625f * max.value();
        element.rotation_angles = rotation.value_or(Eigen::Vector3f::Zero());
        element.rotation_angles[0] = snap_rotation(element.rotation_angles[0]);
        element.rotation_angles[1] = snap_rotation(element.rotation_angles[1]);
        element.rotation_angles[2] = snap_rotation(element.rotation_angles[2]);
        element.rescale = true;
        element.shade = true;

        if(origin.has_value()) {
            element.rotation_origin = 0.0625f * origin.value();
        }

        if(json_object_has_value(object, "rescale")) {
            element.rescale = json_object_get_boolean(object, "rescale");
        }

        if(json_object_has_value(object, "shade")) {
            element.shade = json_object_get_boolean(object, "shade");
        }

        element.faces.fill(std::nullopt);

        auto faces = json_object_get_object(object, "faces");
        auto num_faces = json_object_get_count(faces);

        for(std::size_t i = 0; i < num_faces; ++i) {
            auto face_name = json_object_get_name(faces, i);
            auto face_object = json_object_get_object(faces, face_name);

            if(face_object == nullptr) {
                return std::nullopt;
            }

            std::optional<block_face> face_index = std::nullopt;

            for(const auto& [name, index] : BLOCK_FACE_MAPPING) {
                if(face_name == name) {
                    face_index = static_cast<block_face>(index);
                    break;
                }
            }

            if(!face_index.has_value()) {
                return std::nullopt;
            }

            auto face = parse_face(face_object);

            if(!face.has_value()) {
                return std::nullopt;
            }

            element.faces[face_index.value()] = std::move(face.value());
        }

        return element;
    }

    return std::nullopt;
}

static const void* block_model_load_fn(const char* path, std::uint32_t)
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

    auto textures = json_object_get_array(json, "textures");
    auto slots = parse_texture_slots(textures);

    if(!slots.has_value()) {
        msg::log_warning("{}: invalid or missing textures", path);
        json_value_free(jsonv);
        return nullptr;
    }

    auto elements = json_object_get_array(json, "elements");
    auto num_elements = json_array_get_count(elements);

    if(elements == nullptr || num_elements == 0) {
        msg::log_warning("{}: invalid or missing elements", path);
        json_value_free(jsonv);
        return nullptr;
    }

    BlockModel model {};
    model.texture_slots = std::move(slots.value());
    model.elements.reserve(num_elements);

    for(std::size_t i = 0; i < num_elements; ++i) {
        auto element_object = json_array_get_object(elements, i);
        auto element = parse_element(element_object);

        if(!element.has_value()) {
            msg::log_warning("{}: invalid element at index {}", path, i);
            json_value_free(jsonv);
            return nullptr;
        }

        model.elements.push_back(std::move(element.value()));
    }

    json_value_free(jsonv);

    return new BlockModel(std::move(model));
}

static void block_model_free_fn(const void* data)
{
    delete reinterpret_cast<const BlockModel*>(data);
}

void BlockModel::register_resource(void)
{
    res::register_loader<BlockModel>(&block_model_load_fn, &block_model_free_fn);
}
