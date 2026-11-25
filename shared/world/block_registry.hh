#ifndef C35E197B_F82F_423A_8226_7EB5C9E7FEEC
#define C35E197B_F82F_423A_8226_7EB5C9E7FEEC

#include "shared/world/block.hh"

class ModContext;

using blockstate_key_type = std::uint64_t;
using blockstate_val_type = std::uint64_t;

struct BlockStateDecl final {
    blockstate_val_type default_value;
    std::vector<blockstate_val_type> hint; // load-time only
};

struct BlockOverridePatch final {
    static BlockDefinition apply(BlockDefinition base, const BlockOverridePatch& patch);

    BlockOverridePatch(void) = default;

    std::optional<block_render> render;
    std::optional<vx::hash_map<std::string, std::vector<Identifier>>> albedo;
    std::optional<vx::hash_map<std::string, Identifier>> masks;
    std::optional<bool> animated;

    std::optional<Identifier> model_name;
    std::optional<Eigen::Vector3f> model_offset;
    std::optional<block_face> model_facing;

    std::optional<Identifier> bcoll_name;
    std::optional<Eigen::Vector3f> bcoll_offset;
    std::optional<block_face> bcoll_facing;

    std::optional<Identifier> fluid_name;
    std::optional<unsigned> fluid_level;
    std::optional<std::uint32_t> fluid_faces;
    std::optional<bool> fluid_falling;

    std::optional<unsigned> health;

    std::optional<Identifier> sound_set;
    std::optional<block_light_type> emission;
    std::optional<block_light_type> dissipation;

    std::optional<bool> collidable;
    std::optional<bool> selectable;
    std::optional<float> friction;
    std::optional<float> speed;
    std::optional<Eigen::Vector3f> bounce;

    std::optional<block_tag_bit> tags;
    std::optional<bool> replaceable;

    std::optional<std::vector<BlockDrop>> drops;
};

struct BlockVariantRule final {
    BlockVariantRule(void) = default;

    vx::hash_map<blockstate_key_type, blockstate_val_type> when;
    BlockOverridePatch overrides;
};

struct BlockCallback final {
    std::shared_ptr<lua_State> state { nullptr };
    int callback_ref { LUA_NOREF };
};

struct BlockFamily final {
    BlockFamily(void) = default;

    blockstate_val_type state_hash(std::string_view string);
    std::string_view state_value(blockstate_val_type value);

    Identifier name;
    block_id_type stem_id;
    block_id_type default_variant;

    vx::hash_map<blockstate_key_type, BlockStateDecl> states;
    vx::hash_map<blockstate_val_type, std::string> state_values;
    std::vector<BlockVariantRule> variants;

    BlockCallback on_place;
    BlockCallback on_break;
    BlockCallback on_interact;
    BlockCallback on_tick;

    vx::hash_map<std::uint64_t, block_id_type> resolved_states;
    vx::hash_map<block_id_type, vx::hash_map<blockstate_key_type, blockstate_val_type>> id_states;
};

namespace block_registry
{
std::span<const BlockDefinition> all_definitions(void);
std::span<const BlockFamily> all_families(void);
std::uint64_t checksum(void);
} // namespace block_registry

namespace block_registry
{
void commit(ModContext& ctx);
void purge(void);
} // namespace block_registry

namespace block_registry
{
block_id_type find(const Identifier& id);
} // namespace block_registry

namespace block_registry
{
std::optional<Identifier> name_of(block_id_type id);
} // namespace block_registry

namespace block_registry
{
const BlockDefinition* find_definition(block_id_type id);
const BlockDefinition* find_definition(const Identifier& id);
BlockFamily* find_family(block_family_id_type id);
BlockFamily* find_family(const Identifier& id);
BlockFamily* find_family_of(block_id_type id);
} // namespace block_registry

namespace block_registry
{
bool has_tag_all(block_id_type id, block_tag_bit tag_bits);
bool has_tag_any(block_id_type id, block_tag_bit tag_bits);
} // namespace block_registry

namespace block_registry
{
block_id_type resolve_variant(block_id_type curr_id, const vx::hash_map<blockstate_key_type, blockstate_val_type>& map);
} // namespace block_registry

namespace block_registry
{
fluid_id_type preview_fluid(block_id_type id, blockstate_key_type state, blockstate_val_type value);
} // namespace block_registry

#endif /* C35E197B_F82F_423A_8226_7EB5C9E7FEEC */
