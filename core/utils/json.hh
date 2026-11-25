#ifndef B229B29C_CA21_4217_81CB_8DD0C8FC4BC4
#define B229B29C_CA21_4217_81CB_8DD0C8FC4BC4

#include "core/concepts.hh"

namespace utils
{
template<vx::arithmetic T, std::size_t N>
std::optional<Eigen::Vector<T, N>> parse_vector(const JSON_Value* value);
template<vx::arithmetic T, std::size_t N>
std::optional<Eigen::Vector<T, N>> parse_vector(const JSON_Object* object, const char* name);
} // namespace utils

namespace utils
{
template<vx::arithmetic T>
std::optional<T> parse_arithmetic(const JSON_Value* value);
template<vx::arithmetic T>
std::optional<T> parse_arithmetic(const JSON_Object* object, const char* name);
} // namespace utils

namespace utils
{
template<std::unsigned_integral T>
std::optional<T> parse_bitmask(const JSON_Value* value, std::span<const std::pair<std::string_view, T>> mapping);
template<std::unsigned_integral T>
std::optional<T> parse_bitmask(const JSON_Object* object, const char* name, std::span<const std::pair<std::string_view, T>> mapping);
} // namespace utils

namespace utils
{
template<std::integral T>
std::optional<T> parse_enum(const JSON_Value* value, std::span<const std::pair<std::string_view, T>> mapping);
template<std::integral T>
std::optional<T> parse_enum(const JSON_Object* object, const char* name, std::span<const std::pair<std::string_view, T>> mapping);
} // namespace utils

namespace utils
{
template<std::integral T>
std::optional<T> parse_integer(const JSON_Value* value);
template<std::integral T>
std::optional<T> parse_integer(const JSON_Object* object, const char* name);
} // namespace utils

#endif /* B229B29C_CA21_4217_81CB_8DD0C8FC4BC4 */
