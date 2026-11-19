#ifndef A2FDDFDB_9022_42F0_AA77_8AD362EE4147
#define A2FDDFDB_9022_42F0_AA77_8AD362EE4147

#include "core/identifier.hh"

#include "shared/world/fluid.hh"

enum block_render : unsigned {
    BLOCK_RENDER_NONE = 0,
    BLOCK_RENDER_SOLID,
    BLOCK_RENDER_ALPHA,
};

enum block_face : unsigned {
    BLOCK_FACE_NORTH = 0,
    BLOCK_FACE_SOUTH,
    BLOCK_FACE_EAST,
    BLOCK_FACE_WEST,
    BLOCK_FACE_TOP,
    BLOCK_FACE_BOTTOM,
};

enum block_tool_bit : unsigned {
    BLOCK_TOOL_NONE = 0x0000U,
    BLOCK_TOOL_BLADE = 0x0001U,
    BLOCK_TOOL_XBLADE = 0x0002U,
    BLOCK_TOOL_AXE = 0x0004U,
    BLOCK_TOOL_HOE = 0x0008U,
    BLOCK_TOOL_SHOVEL = 0x0010U,
    BLOCK_TOOL_HAMMER = 0x0020U,
    BLOCK_TOOL_PICK = 0x0040U,
};

enum block_touch : unsigned {
    BLOCK_TOUCH_NONE = 0,
    BLOCK_TOUCH_SOLID,
    BLOCK_TOUCH_BOUNCE,
    BLOCK_TOUCH_THROUGH,
};

enum block_tag_bit : std::uint32_t {
    BLOCK_TAG_GAS = 0x00000001U,
    BLOCK_TAG_ROCK = 0x00000002U,
    BLOCK_TAG_SOIL = 0x00000004U,
    BLOCK_TAG_TURF = 0x00000008U,
    BLOCK_TAG_XFOIL = 0x00000010U,
    BLOCK_TAG_SFOIL = 0x00000020U,
    BLOCK_TAG_WOOD = 0x00000040U,
    BLOCK_TAG_ALL = 0xFFFFFFFFU,
};

enum block_tick_source : unsigned {
    BLOCK_TICK_RANDOM,    ///< Random tick update
    BLOCK_TICK_NEIGHBOUR, ///< Neighbour block change
    BLOCK_TICK_SCRIPTED,  ///< Scripted update
};

using block_id_type = std::uint32_t;
constexpr static block_id_type BLOCK_ID_NULL = 0;
constexpr static block_id_type BLOCK_ID_MAX = std::numeric_limits<block_id_type>::max();

using block_light_type = std::uint8_t;
constexpr static block_light_type BLOCK_LIGHT_MIN = 0x00;
constexpr static block_light_type BLOCK_LIGHT_MAX = 0x0F;

using block_family_id_type = std::uint32_t;
constexpr static block_family_id_type BLOCK_FAMILY_ID_NULL = 0;
constexpr static block_family_id_type BLOCK_FAMILY_ID_MAX = std::numeric_limits<block_family_id_type>::max();

struct BlockDropItem final {
    Identifier name;
    unsigned count;
};

struct BlockDrop final {
    std::vector<BlockDropItem> items;
    std::vector<Identifier> cond_effects;
    block_tool_bit cond_tool_bits;
};

struct BlockDefinition final {
    BlockDefinition(void) = default;

    std::optional<std::span<const Identifier>> resolve_albedo_slot(std::string_view slot) const;
    std::optional<Identifier> resolve_mask_slot(std::string_view slot) const;

    block_render render;
    vx::hash_map<std::string, std::vector<Identifier>> albedo;
    vx::hash_map<std::string, Identifier> masks;
    bool animated;

    Identifier model_name;
    Eigen::Vector3f model_offset;
    block_face model_facing;

    Identifier bcoll_name;
    Eigen::Vector3f bcoll_offset;
    block_face bcoll_facing;

    fluid_id_type fluid;
    unsigned fluid_level;
    std::uint32_t fluid_faces;
    std::optional<Identifier> fluid_name;

    unsigned health;
    block_tool_bit tools; // big tool mask made from drops cond_tool_bits

    Identifier sound_set;
    block_light_type emission;
    block_light_type dissipation;

    block_touch touch;
    Eigen::Vector3f touch_coeffs;

    block_tag_bit tags;
    bool replaceable;

    std::vector<BlockDrop> drops;

    block_family_id_type family;

    bool is_stem;
};

#endif /* A2FDDFDB_9022_42F0_AA77_8AD362EE4147 */
