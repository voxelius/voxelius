#ifndef D291C9C4_41FE_47DB_8620_6B7001383B16
#define D291C9C4_41FE_47DB_8620_6B7001383B16

#include "shared/world/block.hh"
#include "shared/world/tint.hh"

struct BlockModelFace final {
    std::string texture_slot;
    std::optional<Eigen::Vector4f> uv;
    unsigned uv_rotation;
    std::optional<block_face> cull_face;
    Identifier tint_name;
};

struct BlockModelElement final {
    Eigen::Vector3f min;
    Eigen::Vector3f max;
    Eigen::Vector3f rotation_origin;
    Eigen::Vector3f rotation_angles;
    bool rescale;
    bool shade;

    std::array<std::optional<BlockModelFace>, 6> faces;
};

struct BlockModel final {
    static void register_resource(void);

    std::vector<std::string> texture_slots;
    std::vector<BlockModelElement> elements;
};

#endif /* D291C9C4_41FE_47DB_8620_6B7001383B16 */
