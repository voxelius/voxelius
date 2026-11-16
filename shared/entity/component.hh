#ifndef A0C7CDBF_AEB2_4D28_AC4D_1B1D7550C5AA
#define A0C7CDBF_AEB2_4D28_AC4D_1B1D7550C5AA

class ReadBuffer;
class WriteBuffer;

using component_id_type = std::uint16_t;
constexpr static component_id_type COMPONENT_ID_NULL = 0;
constexpr static component_id_type COMPONENT_ID_MAX = std::numeric_limits<component_id_type>::max();

struct ComponentTable final {
    std::shared_ptr<lua_State> lua;
    int table_index;
};

template<typename T>
struct Component final {
    static std::any prepare(lua_State* L, int config_idx);
    static void attach(entt::entity entity, const std::any& config);
    static bool patch(entt::entity entity, lua_State* L, int kv_idx);
    static void encode_dat(entt::entity entity, WriteBuffer& buffer);
    static void decode_net(entt::entity entity, ReadBuffer& buffer);
    static void encode_net(entt::entity entity, WriteBuffer& buffer);
    static void decode_dat(entt::entity entity, ReadBuffer& buffer);
    static void interpolate(entt::entity entity, float alpha) = delete;
};

#endif /* A0C7CDBF_AEB2_4D28_AC4D_1B1D7550C5AA */
