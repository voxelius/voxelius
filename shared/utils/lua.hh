#ifndef B559A84C_2B66_435D_9730_506C892E8E29
#define B559A84C_2B66_435D_9730_506C892E8E29

#include "core/concepts.hh"

namespace utils
{
template<std::size_t N>
std::optional<Eigen::Vector<lua_Integer, N>> require_ivec(lua_State* L, int idx);
template<std::size_t N>
std::optional<Eigen::Vector<lua_Integer, N>> require_ivec(lua_State* L, int idx, const char* key);
} // namespace utils

namespace utils
{
template<std::size_t N>
std::optional<Eigen::Vector<lua_Integer, N>> opt_ivec(lua_State* L, int idx, Eigen::Vector<lua_Integer, N> default_value);
template<std::size_t N>
std::optional<Eigen::Vector<lua_Integer, N>> opt_ivec(lua_State* L, int idx, const char* key, Eigen::Vector<lua_Integer, N> default_value);
} // namespace utils

namespace utils
{
template<std::size_t N>
std::optional<Eigen::Vector<lua_Number, N>> require_fvec(lua_State* L, int idx);
template<std::size_t N>
std::optional<Eigen::Vector<lua_Number, N>> require_fvec(lua_State* L, int idx, const char* key);
} // namespace utils

namespace utils
{
template<std::size_t N>
std::optional<Eigen::Vector<lua_Number, N>> opt_fvec(lua_State* L, int idx, Eigen::Vector<lua_Number, N> default_value);
template<std::size_t N>
std::optional<Eigen::Vector<lua_Number, N>> opt_fvec(lua_State* L, int idx, const char* key, Eigen::Vector<lua_Number, N> default_value);
} // namespace utils

namespace utils
{
template<std::unsigned_integral T>
std::optional<T> require_bitmask(lua_State* L, int idx);
template<std::unsigned_integral T>
std::optional<T> require_bitmask(lua_State* L, int idx, const char* key);
} // namespace utils

namespace utils
{
template<std::unsigned_integral T>
std::optional<T> opt_bitmask(lua_State* L, int idx, T default_value);
template<std::unsigned_integral T>
std::optional<T> opt_bitmask(lua_State* L, int idx, const char* key, T default_value);
} // namespace utils

namespace utils
{
std::optional<bool> require_bool(lua_State* L, int idx);
std::optional<lua_Integer> require_integer(lua_State* L, int idx);
std::optional<lua_Number> require_number(lua_State* L, int idx);
std::optional<std::string_view> require_string(lua_State* L, int idx);
} // namespace utils

namespace utils
{
std::optional<bool> require_bool(lua_State* L, int idx, const char* key);
std::optional<lua_Integer> require_integer(lua_State* L, int idx, const char* key);
std::optional<lua_Number> require_number(lua_State* L, int idx, const char* key);
std::optional<std::string_view> require_string(lua_State* L, int idx, const char* key);
} // namespace utils

namespace utils
{
std::optional<bool> opt_bool(lua_State* L, int idx, bool default_value);
std::optional<lua_Integer> opt_integer(lua_State* L, int idx, lua_Integer default_value);
std::optional<lua_Number> opt_number(lua_State* L, int idx, lua_Number default_value);
std::optional<std::string_view> opt_string(lua_State* L, int idx, std::string_view default_value);
} // namespace utils

namespace utils
{
std::optional<bool> opt_bool(lua_State* L, int idx, const char* key, bool default_value);
std::optional<lua_Integer> opt_integer(lua_State* L, int idx, const char* key, lua_Integer default_value);
std::optional<lua_Number> opt_number(lua_State* L, int idx, const char* key, lua_Number default_value);
std::optional<std::string_view> opt_string(lua_State* L, int idx, const char* key, std::string_view default_value);
} // namespace utils

#endif /* B559A84C_2B66_435D_9730_506C892E8E29 */
