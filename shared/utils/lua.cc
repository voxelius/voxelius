#include "shared/pch.hh"

#include "shared/utils/lua.hh"

template<std::size_t N>
std::optional<Eigen::Vector<lua_Integer, N>> utils::require_ivec(lua_State* L, int idx)
{
    if(!lua_istable(L, idx)) {
        lua_pushfstring(L, "expected a table, got %s", lua_typename(L, lua_type(L, idx)));
        return std::nullopt;
    }

    auto count = lua_rawlen(L, idx);

    if(count != N) {
        lua_pushfstring(L, "expected a table of length %d, got %d", static_cast<int>(N), static_cast<int>(count));
        return std::nullopt;
    }

    Eigen::Vector<lua_Integer, N> result = Eigen::Vector<lua_Integer, N>::Zero();

    for(std::size_t i = 0; i < N; ++i) {
        lua_rawgeti(L, idx, static_cast<lua_Integer>(i + 1));

        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return std::nullopt;
        }

        result[i] = value.value();

        lua_pop(L, 1);
    }

    return result;
}

template std::optional<Eigen::Vector<lua_Integer, 2>> utils::require_ivec(lua_State* L, int idx);
template std::optional<Eigen::Vector<lua_Integer, 3>> utils::require_ivec(lua_State* L, int idx);
template std::optional<Eigen::Vector<lua_Integer, 4>> utils::require_ivec(lua_State* L, int idx);

template<std::size_t N>
std::optional<Eigen::Vector<lua_Integer, N>> utils::require_ivec(lua_State* L, int idx, const char* key)
{
    lua_getfield(L, idx, key);

    auto result = require_ivec<N>(L, -1);

    if(result.has_value()) {
        lua_pop(L, 1);
    }

    return result;
}

template std::optional<Eigen::Vector<lua_Integer, 2>> utils::require_ivec(lua_State* L, int idx, const char* key);
template std::optional<Eigen::Vector<lua_Integer, 3>> utils::require_ivec(lua_State* L, int idx, const char* key);
template std::optional<Eigen::Vector<lua_Integer, 4>> utils::require_ivec(lua_State* L, int idx, const char* key);

template<std::size_t N>
std::optional<Eigen::Vector<lua_Integer, N>> utils::opt_ivec(lua_State* L, int idx, Eigen::Vector<lua_Integer, N> default_value)
{
    if(lua_isnoneornil(L, idx))
        return default_value;
    return require_ivec<N>(L, idx);
}

template std::optional<Eigen::Vector<lua_Integer, 2>> utils::opt_ivec(lua_State* L, int idx, Eigen::Vector<lua_Integer, 2> default_value);
template std::optional<Eigen::Vector<lua_Integer, 3>> utils::opt_ivec(lua_State* L, int idx, Eigen::Vector<lua_Integer, 3> default_value);
template std::optional<Eigen::Vector<lua_Integer, 4>> utils::opt_ivec(lua_State* L, int idx, Eigen::Vector<lua_Integer, 4> default_value);

template<std::size_t N>
std::optional<Eigen::Vector<lua_Integer, N>> utils::opt_ivec(lua_State* L, int idx, const char* key,
    Eigen::Vector<lua_Integer, N> default_value)
{
    lua_getfield(L, idx, key);

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);

        return default_value;
    }

    auto result = require_ivec<N>(L, -1);

    if(result.has_value()) {
        lua_pop(L, 1);
    }

    return result;
}

template std::optional<Eigen::Vector<lua_Integer, 2>> utils::opt_ivec(lua_State* L, int idx, const char* key,
    Eigen::Vector<lua_Integer, 2> default_value);
template std::optional<Eigen::Vector<lua_Integer, 3>> utils::opt_ivec(lua_State* L, int idx, const char* key,
    Eigen::Vector<lua_Integer, 3> default_value);
template std::optional<Eigen::Vector<lua_Integer, 4>> utils::opt_ivec(lua_State* L, int idx, const char* key,
    Eigen::Vector<lua_Integer, 4> default_value);

template<std::size_t N>
std::optional<Eigen::Vector<lua_Number, N>> utils::require_fvec(lua_State* L, int idx)
{
    if(!lua_istable(L, idx)) {
        lua_pushfstring(L, "expected a table, got %s", lua_typename(L, lua_type(L, idx)));
        return std::nullopt;
    }

    auto count = lua_rawlen(L, idx);

    if(count != N) {
        lua_pushfstring(L, "expected a table of length %d, got %d", static_cast<int>(N), static_cast<int>(count));
        return std::nullopt;
    }

    Eigen::Vector<lua_Number, N> result = Eigen::Vector<lua_Number, N>::Zero();

    for(std::size_t i = 0; i < N; ++i) {
        lua_rawgeti(L, idx, static_cast<lua_Integer>(i + 1));

        auto value = utils::require_number(L, -1);

        if(!value.has_value()) {
            return std::nullopt;
        }

        result[i] = value.value();

        lua_pop(L, 1);
    }

    return result;
}

template std::optional<Eigen::Vector<lua_Number, 2>> utils::require_fvec(lua_State* L, int idx);
template std::optional<Eigen::Vector<lua_Number, 3>> utils::require_fvec(lua_State* L, int idx);
template std::optional<Eigen::Vector<lua_Number, 4>> utils::require_fvec(lua_State* L, int idx);

template<std::size_t N>
std::optional<Eigen::Vector<lua_Number, N>> utils::require_fvec(lua_State* L, int idx, const char* key)
{
    lua_getfield(L, idx, key);

    auto result = require_fvec<N>(L, -1);

    if(result.has_value()) {
        lua_pop(L, 1);
    }

    return result;
}

template std::optional<Eigen::Vector<lua_Number, 2>> utils::require_fvec(lua_State* L, int idx, const char* key);
template std::optional<Eigen::Vector<lua_Number, 3>> utils::require_fvec(lua_State* L, int idx, const char* key);
template std::optional<Eigen::Vector<lua_Number, 4>> utils::require_fvec(lua_State* L, int idx, const char* key);

template<std::size_t N>
std::optional<Eigen::Vector<lua_Number, N>> utils::opt_fvec(lua_State* L, int idx, Eigen::Vector<lua_Number, N> default_value)
{
    if(lua_isnoneornil(L, idx))
        return default_value;
    return require_fvec<N>(L, idx);
}

template std::optional<Eigen::Vector<lua_Number, 2>> utils::opt_fvec(lua_State* L, int idx, Eigen::Vector<lua_Number, 2> default_value);
template std::optional<Eigen::Vector<lua_Number, 3>> utils::opt_fvec(lua_State* L, int idx, Eigen::Vector<lua_Number, 3> default_value);
template std::optional<Eigen::Vector<lua_Number, 4>> utils::opt_fvec(lua_State* L, int idx, Eigen::Vector<lua_Number, 4> default_value);

template<std::size_t N>
std::optional<Eigen::Vector<lua_Number, N>> utils::opt_fvec(lua_State* L, int idx, const char* key,
    Eigen::Vector<lua_Number, N> default_value)
{
    lua_getfield(L, idx, key);

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);

        return default_value;
    }

    auto result = require_fvec<N>(L, -1);

    if(result.has_value()) {
        lua_pop(L, 1);
    }

    return result;
}

template std::optional<Eigen::Vector<lua_Number, 2>> utils::opt_fvec(lua_State* L, int idx, const char* key,
    Eigen::Vector<lua_Number, 2> default_value);
template std::optional<Eigen::Vector<lua_Number, 3>> utils::opt_fvec(lua_State* L, int idx, const char* key,
    Eigen::Vector<lua_Number, 3> default_value);
template std::optional<Eigen::Vector<lua_Number, 4>> utils::opt_fvec(lua_State* L, int idx, const char* key,
    Eigen::Vector<lua_Number, 4> default_value);

template<std::unsigned_integral T>
std::optional<T> utils::require_bitmask(lua_State* L, int idx)
{
    if(!lua_istable(L, idx)) {
        lua_pushfstring(L, "expected a table, got %s", lua_typename(L, lua_type(L, idx)));
        return std::nullopt;
    }

    T mask = static_cast<T>(0);
    auto count = lua_rawlen(L, idx);

    for(lua_Integer i = 1; i <= static_cast<lua_Integer>(count); ++i) {
        lua_rawgeti(L, idx, i);

        auto value = utils::require_integer(L, -1);

        if(!value.has_value()) {
            return std::nullopt;
        }

        mask |= static_cast<T>(value.value());

        lua_pop(L, 1);
    }

    return mask;
}

template std::optional<unsigned char> utils::require_bitmask(lua_State* L, int idx);
template std::optional<unsigned short> utils::require_bitmask(lua_State* L, int idx);
template std::optional<unsigned> utils::require_bitmask(lua_State* L, int idx);
template std::optional<unsigned long> utils::require_bitmask(lua_State* L, int idx);
template std::optional<unsigned long long> utils::require_bitmask(lua_State* L, int idx);

template<std::unsigned_integral T>
std::optional<T> utils::require_bitmask(lua_State* L, int idx, const char* key)
{
    lua_getfield(L, idx, key);

    auto result = require_bitmask<T>(L, -1);

    if(result.has_value()) {
        lua_pop(L, 1);
    }

    return result;
}

template std::optional<unsigned char> utils::require_bitmask(lua_State* L, int idx, const char* key);
template std::optional<unsigned short> utils::require_bitmask(lua_State* L, int idx, const char* key);
template std::optional<unsigned> utils::require_bitmask(lua_State* L, int idx, const char* key);
template std::optional<unsigned long> utils::require_bitmask(lua_State* L, int idx, const char* key);
template std::optional<unsigned long long> utils::require_bitmask(lua_State* L, int idx, const char* key);

template<std::unsigned_integral T>
std::optional<T> utils::opt_bitmask(lua_State* L, int idx, T default_value)
{
    if(lua_isnoneornil(L, idx))
        return default_value;
    return require_bitmask<T>(L, idx);
}

template std::optional<unsigned char> utils::opt_bitmask(lua_State* L, int idx, unsigned char default_value);
template std::optional<unsigned short> utils::opt_bitmask(lua_State* L, int idx, unsigned short default_value);
template std::optional<unsigned> utils::opt_bitmask(lua_State* L, int idx, unsigned default_value);
template std::optional<unsigned long> utils::opt_bitmask(lua_State* L, int idx, unsigned long default_value);
template std::optional<unsigned long long> utils::opt_bitmask(lua_State* L, int idx, unsigned long long default_value);

template<std::unsigned_integral T>
std::optional<T> utils::opt_bitmask(lua_State* L, int idx, const char* key, T default_value)
{
    lua_getfield(L, idx, key);

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);

        return default_value;
    }

    auto result = require_bitmask<T>(L, -1);

    if(result.has_value()) {
        lua_pop(L, 1);
    }

    return result;
}

template std::optional<unsigned char> utils::opt_bitmask(lua_State* L, int idx, const char* key, unsigned char default_value);
template std::optional<unsigned short> utils::opt_bitmask(lua_State* L, int idx, const char* key, unsigned short default_value);
template std::optional<unsigned> utils::opt_bitmask(lua_State* L, int idx, const char* key, unsigned default_value);
template std::optional<unsigned long> utils::opt_bitmask(lua_State* L, int idx, const char* key, unsigned long default_value);
template std::optional<unsigned long long> utils::opt_bitmask(lua_State* L, int idx, const char* key, unsigned long long default_value);

std::optional<bool> utils::require_bool(lua_State* L, int idx)
{
    if(lua_isboolean(L, idx)) {
        return static_cast<bool>(lua_toboolean(L, idx));
    }

    lua_pushfstring(L, "expected a boolean, got %s", lua_typename(L, lua_type(L, idx)));

    return std::nullopt;
}

std::optional<lua_Integer> utils::require_integer(lua_State* L, int idx)
{
    int is_num = 0;
    auto value = lua_tointegerx(L, idx, &is_num);

    if(is_num) {
        return value;
    }

    lua_pushfstring(L, "expected an integer, got %s", lua_typename(L, lua_type(L, idx)));

    return std::nullopt;
}

std::optional<lua_Number> utils::require_number(lua_State* L, int idx)
{
    int is_num = 0;
    auto value = lua_tonumberx(L, idx, &is_num);

    if(is_num) {
        return value;
    }

    lua_pushfstring(L, "expected a number, got %s", lua_typename(L, lua_type(L, idx)));

    return std::nullopt;
}

std::optional<std::string_view> utils::require_string(lua_State* L, int idx)
{
    auto type = lua_type(L, idx);

    if(type == LUA_TSTRING || type == LUA_TNUMBER) {
        std::size_t length = 0;
        auto value = lua_tolstring(L, idx, &length);
        return std::string_view(value, length);
    }

    lua_pushfstring(L, "expected a string, got %s", lua_typename(L, type));

    return std::nullopt;
}

std::optional<bool> utils::require_bool(lua_State* L, int idx, const char* key)
{
    lua_getfield(L, idx, key);

    auto result = require_bool(L, -1);

    if(result.has_value()) {
        lua_pop(L, 1);
    }

    return result;
}

std::optional<lua_Integer> utils::require_integer(lua_State* L, int idx, const char* key)
{
    lua_getfield(L, idx, key);

    auto result = require_integer(L, -1);

    if(result.has_value()) {
        lua_pop(L, 1);
    }

    return result;
}

std::optional<lua_Number> utils::require_number(lua_State* L, int idx, const char* key)
{
    lua_getfield(L, idx, key);

    auto result = require_number(L, -1);

    if(result.has_value()) {
        lua_pop(L, 1);
    }

    return result;
}

std::optional<std::string_view> utils::require_string(lua_State* L, int idx, const char* key)
{
    lua_getfield(L, idx, key);

    auto result = require_string(L, -1);

    if(result.has_value()) {
        lua_pop(L, 1);
    }

    return result;
}

std::optional<bool> utils::opt_bool(lua_State* L, int idx, bool default_value)
{
    if(lua_isnoneornil(L, idx))
        return default_value;
    return require_bool(L, idx);
}

std::optional<lua_Integer> utils::opt_integer(lua_State* L, int idx, lua_Integer default_value)
{
    if(lua_isnoneornil(L, idx))
        return default_value;
    return require_integer(L, idx);
}

std::optional<lua_Number> utils::opt_number(lua_State* L, int idx, lua_Number default_value)
{
    if(lua_isnoneornil(L, idx))
        return default_value;
    return require_number(L, idx);
}

std::optional<std::string_view> utils::opt_string(lua_State* L, int idx, std::string_view default_value)
{
    if(lua_isnoneornil(L, idx))
        return default_value;
    return require_string(L, idx);
}

std::optional<bool> utils::opt_bool(lua_State* L, int idx, const char* key, bool default_value)
{
    lua_getfield(L, idx, key);

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);

        return default_value;
    }

    auto result = require_bool(L, -1);

    if(result.has_value()) {
        lua_pop(L, 1);
    }

    return result;
}

std::optional<lua_Integer> utils::opt_integer(lua_State* L, int idx, const char* key, lua_Integer default_value)
{
    lua_getfield(L, idx, key);

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);

        return default_value;
    }

    auto result = require_integer(L, -1);

    if(result.has_value()) {
        lua_pop(L, 1);
    }

    return result;
}

std::optional<lua_Number> utils::opt_number(lua_State* L, int idx, const char* key, lua_Number default_value)
{
    lua_getfield(L, idx, key);

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);

        return default_value;
    }

    auto result = require_number(L, -1);

    if(result.has_value()) {
        lua_pop(L, 1);
    }

    return result;
}

std::optional<std::string_view> utils::opt_string(lua_State* L, int idx, const char* key, std::string_view default_value)
{
    lua_getfield(L, idx, key);

    if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);

        return default_value;
    }

    auto result = require_string(L, -1);

    if(result.has_value()) {
        lua_pop(L, 1);
    }

    return result;
}
