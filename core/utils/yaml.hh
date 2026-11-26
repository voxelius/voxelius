#ifndef F3B7D1A9_5C2E_4B86_9D4F_1A7C3E5B9D28
#define F3B7D1A9_5C2E_4B86_9D4F_1A7C3E5B9D28

#include "core/concepts.hh"

namespace utils
{
ryml::ConstNodeRef find_child(ryml::ConstNodeRef parent, std::string_view key);
} // namespace utils

namespace utils
{
// The view refers to the parsed tree and is valid while the tree lives
std::optional<std::string_view> parse_string(ryml::ConstNodeRef node);
std::optional<std::string_view> parse_string(ryml::ConstNodeRef parent, std::string_view key);
std::optional<bool> parse_boolean(ryml::ConstNodeRef node);
std::optional<bool> parse_boolean(ryml::ConstNodeRef parent, std::string_view key);
} // namespace utils

namespace utils
{
template<vx::arithmetic T>
std::optional<T> parse_arithmetic(ryml::ConstNodeRef node);
template<vx::arithmetic T>
std::optional<T> parse_arithmetic(ryml::ConstNodeRef parent, std::string_view key);
} // namespace utils

namespace utils
{
template<std::integral T>
std::optional<T> parse_integer(ryml::ConstNodeRef node);
template<std::integral T>
std::optional<T> parse_integer(ryml::ConstNodeRef parent, std::string_view key);
} // namespace utils

#endif /* F3B7D1A9_5C2E_4B86_9D4F_1A7C3E5B9D28 */
