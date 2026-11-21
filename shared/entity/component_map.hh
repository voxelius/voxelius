#ifndef AFD5DBFC_92AA_4C82_85A9_F6FC3D3C4ADE
#define AFD5DBFC_92AA_4C82_85A9_F6FC3D3C4ADE

#include "shared/entity/component.hh"

struct DirtyMarker final {
    std::vector<bool> markers;
};

namespace component_map::detail
{
struct FunctionTable final {
    std::any (*prepare)(lua_State* L, int config_idx);
    void (*attach)(entt::registry& registry, entt::entity entity, const std::any& config);
    bool (*patch)(entt::registry& registry, entt::entity entity, lua_State* L, int kv_idx);
    void (*encode_net)(entt::registry& registry, entt::entity entity, WriteBuffer& buffer);
    void (*decode_net)(entt::registry& registry, entt::entity entity, ReadBuffer& buffer);
    void (*encode_dat)(entt::registry& registry, entt::entity entity, WriteBuffer& buffer);
    void (*decode_dat)(entt::registry& registry, entt::entity entity, ReadBuffer& buffer);
    void (*interpolate)(entt::registry& registry, entt::entity entity, float alpha);
    void (*connect)(entt::registry& registry);
    void (*hash_config)(std::uint64_t& hash, const std::any& config);
};
} // namespace component_map::detail

namespace component_map::detail
{
component_id_type add(const std::type_info& type, std::string_view name, FunctionTable functions);
component_id_type from_type(const std::type_info& type);
} // namespace component_map::detail

namespace component_map::detail
{
void mark_dirty(entt::registry& registry, component_id_type id, entt::entity entity);
} // namespace component_map::detail

namespace component_map
{
template<typename T>
void on_update(entt::registry& registry, entt::entity entity);
} // namespace component_map

namespace component_map
{
template<typename T>
component_id_type add(std::string_view name);
template<typename T>
component_id_type from_type(void);
component_id_type from_name(std::string_view name);
} // namespace component_map

namespace component_map
{
std::string_view name_of(component_id_type id);
} // namespace component_map

namespace component_map
{
std::any prepare(component_id_type id, lua_State* L, int config_idx);
bool attach(entt::registry& registry, component_id_type id, entt::entity entity, const std::any& config);
bool patch(entt::registry& registry, component_id_type id, entt::entity entity, lua_State* L, int kv_idx);
void encode_net(entt::registry& registry, component_id_type id, entt::entity entity, WriteBuffer& buffer);
void decode_net(entt::registry& registry, component_id_type id, entt::entity entity, ReadBuffer& buffer);
void encode_dat(entt::registry& registry, component_id_type id, entt::entity entity, WriteBuffer& buffer);
void decode_dat(entt::registry& registry, component_id_type id, entt::entity entity, ReadBuffer& buffer);
void hash_config(std::uint64_t& hash, component_id_type id, const std::any& config);
} // namespace component_map

namespace component_map
{
void connect_all(entt::registry& registry);
} // namespace component_map

namespace component_map
{
void mark_dirty(entt::registry& registry, component_id_type id, entt::entity entity);
} // namespace component_map

namespace component_map
{
void interpolate(entt::registry& registry, entt::entity entity, float alpha);
} // namespace component_map

template<typename T>
void component_map::on_update(entt::registry& registry, entt::entity entity)
{
    static auto id = from_type<T>();

    detail::mark_dirty(registry, id, entity);
}

template<typename T>
component_id_type component_map::add(std::string_view name)
{
    detail::FunctionTable functions {};
    functions.prepare = &Component<T>::prepare;
    functions.attach = &Component<T>::attach;
    functions.patch = &Component<T>::patch;
    functions.encode_net = &Component<T>::encode_net;
    functions.decode_net = &Component<T>::decode_net;
    functions.encode_dat = &Component<T>::encode_dat;
    functions.decode_dat = &Component<T>::decode_dat;
    functions.connect = &Component<T>::connect;

    if constexpr(requires { &Component<T>::interpolate; }) {
        functions.interpolate = &Component<T>::interpolate;
    }

    if constexpr(requires { &Component<T>::hash_config; }) {
        functions.hash_config = &Component<T>::hash_config;
    }

    return detail::add(typeid(T), std::move(name), std::move(functions));
}

template<typename T>
component_id_type component_map::from_type(void)
{
    return detail::from_type(typeid(T));
}

#endif /* AFD5DBFC_92AA_4C82_85A9_F6FC3D3C4ADE */
