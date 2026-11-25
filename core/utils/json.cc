#include "core/pch.hh"

#include "core/utils/json.hh"

template<vx::arithmetic T, std::size_t N>
std::optional<Eigen::Vector<T, N>> utils::parse_vector(const JSON_Value* value)
{
    Eigen::Vector<T, N> vector;
    vector.setZero();

    const auto array = json_value_get_array(value);

    if(array == nullptr) {
        return std::nullopt;
    }

    if(json_array_get_count(array) < N) {
        return std::nullopt;
    }

    for(std::size_t i = 0; i < N; ++i) {
        const auto element = json_array_get_value(array, i);

        if(element == nullptr || json_value_get_type(element) != JSONNumber) {
            return std::nullopt;
        }

        vector[i] = static_cast<T>(json_value_get_number(element));
    }

    return vector;
}

template<vx::arithmetic T, std::size_t N>
std::optional<Eigen::Vector<T, N>> utils::parse_vector(const JSON_Object* object, const char* name)
{
    const auto value = json_object_get_value(object, name);

    if(value == nullptr) {
        return std::nullopt;
    }

    return parse_vector<T, N>(value);
}

template<vx::arithmetic T>
std::optional<T> utils::parse_arithmetic(const JSON_Value* value)
{
    if(value == nullptr || json_value_get_type(value) != JSONNumber) {
        return std::nullopt;
    }

    return static_cast<T>(json_value_get_number(value));
}

template<vx::arithmetic T>
std::optional<T> utils::parse_arithmetic(const JSON_Object* object, const char* name)
{
    const auto value = json_object_get_value(object, name);

    if(value == nullptr) {
        return std::nullopt;
    }

    return parse_arithmetic<T>(value);
}

template<std::unsigned_integral T>
std::optional<T> utils::parse_bitmask(const JSON_Value* value, std::span<const std::pair<std::string_view, T>> mapping)
{
    if(value == nullptr || json_value_get_type(value) != JSONArray) {
        return std::nullopt;
    }

    const auto array = json_value_get_array(value);
    const auto count = json_array_get_count(array);

    auto result = static_cast<T>(0);

    for(std::size_t i = 0; i < count; ++i) {
        const auto raw = json_array_get_string(array, i);

        if(raw == nullptr) {
            return std::nullopt;
        }

        for(const auto& [name, bit] : mapping) {
            if(name == raw) {
                result |= bit;
                break;
            }
        }
    }

    return result;
}

template<std::unsigned_integral T>
std::optional<T> utils::parse_bitmask(const JSON_Object* object, const char* name, std::span<const std::pair<std::string_view, T>> mapping)
{
    const auto value = json_object_get_value(object, name);

    if(value == nullptr) {
        return std::nullopt;
    }

    return parse_bitmask<T>(value, mapping);
}

template<std::integral T>
std::optional<T> utils::parse_enum(const JSON_Value* value, std::span<const std::pair<std::string_view, T>> mapping)
{
    if(value == nullptr || json_value_get_type(value) != JSONString) {
        return std::nullopt;
    }

    const auto raw = json_value_get_string(value);

    for(const auto& [name, enum_value] : mapping) {
        if(name == raw) {
            return enum_value;
        }
    }

    return std::nullopt;
}

template<std::integral T>
std::optional<T> utils::parse_enum(const JSON_Object* object, const char* name, std::span<const std::pair<std::string_view, T>> mapping)
{
    const auto value = json_object_get_value(object, name);

    if(value == nullptr) {
        return std::nullopt;
    }

    return parse_enum<T>(value, mapping);
}

template<std::integral T>
std::optional<T> utils::parse_integer(const JSON_Value* value)
{
    auto number = parse_arithmetic<double>(value);

    if(!number.has_value()) {
        return std::nullopt;
    }

    auto integer = number.value();

    auto is_valid = true;
    is_valid = is_valid && std::isfinite(integer);
    is_valid = is_valid && integer == std::trunc(integer);
    is_valid = is_valid && integer >= static_cast<double>(std::numeric_limits<T>::lowest());
    is_valid = is_valid && integer < 1.0 + static_cast<double>(std::numeric_limits<T>::max());

    if(is_valid) {
        return static_cast<T>(integer);
    }

    return std::nullopt;
}

template<std::integral T>
std::optional<T> utils::parse_integer(const JSON_Object* object, const char* name)
{
    return parse_integer<T>(json_object_get_value(object, name));
}

template std::optional<Eigen::Vector<int, 2>> utils::parse_vector(const JSON_Value* value);
template std::optional<Eigen::Vector<int, 3>> utils::parse_vector(const JSON_Value* value);
template std::optional<Eigen::Vector<int, 4>> utils::parse_vector(const JSON_Value* value);

template std::optional<Eigen::Vector<float, 2>> utils::parse_vector(const JSON_Value* value);
template std::optional<Eigen::Vector<float, 3>> utils::parse_vector(const JSON_Value* value);
template std::optional<Eigen::Vector<float, 4>> utils::parse_vector(const JSON_Value* value);

template std::optional<Eigen::Vector<double, 2>> utils::parse_vector(const JSON_Value* value);
template std::optional<Eigen::Vector<double, 3>> utils::parse_vector(const JSON_Value* value);
template std::optional<Eigen::Vector<double, 4>> utils::parse_vector(const JSON_Value* value);

template std::optional<Eigen::Vector<int, 2>> utils::parse_vector(const JSON_Object* object, const char* name);
template std::optional<Eigen::Vector<int, 3>> utils::parse_vector(const JSON_Object* object, const char* name);
template std::optional<Eigen::Vector<int, 4>> utils::parse_vector(const JSON_Object* object, const char* name);

template std::optional<Eigen::Vector<float, 2>> utils::parse_vector(const JSON_Object* object, const char* name);
template std::optional<Eigen::Vector<float, 3>> utils::parse_vector(const JSON_Object* object, const char* name);
template std::optional<Eigen::Vector<float, 4>> utils::parse_vector(const JSON_Object* object, const char* name);

template std::optional<Eigen::Vector<double, 2>> utils::parse_vector(const JSON_Object* object, const char* name);
template std::optional<Eigen::Vector<double, 3>> utils::parse_vector(const JSON_Object* object, const char* name);
template std::optional<Eigen::Vector<double, 4>> utils::parse_vector(const JSON_Object* object, const char* name);

template std::optional<signed char> utils::parse_arithmetic(const JSON_Value* value);
template std::optional<short> utils::parse_arithmetic(const JSON_Value* value);
template std::optional<int> utils::parse_arithmetic(const JSON_Value* value);
template std::optional<long> utils::parse_arithmetic(const JSON_Value* value);
template std::optional<long long> utils::parse_arithmetic(const JSON_Value* value);
template std::optional<float> utils::parse_arithmetic(const JSON_Value* value);
template std::optional<double> utils::parse_arithmetic(const JSON_Value* value);

template std::optional<int> utils::parse_integer(const JSON_Value* value);
template std::optional<long> utils::parse_integer(const JSON_Value* value);
template std::optional<long long> utils::parse_integer(const JSON_Value* value);

template std::optional<int> utils::parse_integer(const JSON_Object* object, const char* name);
template std::optional<long> utils::parse_integer(const JSON_Object* object, const char* name);
template std::optional<long long> utils::parse_integer(const JSON_Object* object, const char* name);

template std::optional<unsigned char> utils::parse_arithmetic(const JSON_Object* object, const char* name);
template std::optional<unsigned short> utils::parse_arithmetic(const JSON_Object* object, const char* name);
template std::optional<unsigned> utils::parse_arithmetic(const JSON_Object* object, const char* name);
template std::optional<unsigned long> utils::parse_arithmetic(const JSON_Object* object, const char* name);
template std::optional<unsigned long long> utils::parse_arithmetic(const JSON_Object* object, const char* name);

template std::optional<signed char> utils::parse_arithmetic(const JSON_Object* object, const char* name);
template std::optional<short> utils::parse_arithmetic(const JSON_Object* object, const char* name);
template std::optional<int> utils::parse_arithmetic(const JSON_Object* object, const char* name);
template std::optional<long> utils::parse_arithmetic(const JSON_Object* object, const char* name);
template std::optional<long long> utils::parse_arithmetic(const JSON_Object* object, const char* name);
template std::optional<float> utils::parse_arithmetic(const JSON_Object* object, const char* name);
template std::optional<double> utils::parse_arithmetic(const JSON_Object* object, const char* name);

template std::optional<unsigned char> utils::parse_bitmask(const JSON_Value* value,
    std::span<const std::pair<std::string_view, unsigned char>> mapping);
template std::optional<unsigned short> utils::parse_bitmask(const JSON_Value* value,
    std::span<const std::pair<std::string_view, unsigned short>> mapping);
template std::optional<unsigned> utils::parse_bitmask(const JSON_Value* value,
    std::span<const std::pair<std::string_view, unsigned>> mapping);
template std::optional<unsigned long> utils::parse_bitmask(const JSON_Value* value,
    std::span<const std::pair<std::string_view, unsigned long>> mapping);
template std::optional<unsigned long long> utils::parse_bitmask(const JSON_Value* value,
    std::span<const std::pair<std::string_view, unsigned long long>> mapping);

template std::optional<unsigned char> utils::parse_bitmask(const JSON_Object* object, const char* name,
    std::span<const std::pair<std::string_view, unsigned char>> mapping);
template std::optional<unsigned short> utils::parse_bitmask(const JSON_Object* object, const char* name,
    std::span<const std::pair<std::string_view, unsigned short>> mapping);
template std::optional<unsigned> utils::parse_bitmask(const JSON_Object* object, const char* name,
    std::span<const std::pair<std::string_view, unsigned>> mapping);
template std::optional<unsigned long> utils::parse_bitmask(const JSON_Object* object, const char* name,
    std::span<const std::pair<std::string_view, unsigned long>> mapping);
template std::optional<unsigned long long> utils::parse_bitmask(const JSON_Object* object, const char* name,
    std::span<const std::pair<std::string_view, unsigned long long>> mapping);

template std::optional<signed char> utils::parse_enum(const JSON_Value* value,
    std::span<const std::pair<std::string_view, signed char>> mapping);
template std::optional<short> utils::parse_enum(const JSON_Value* value, std::span<const std::pair<std::string_view, short>> mapping);
template std::optional<int> utils::parse_enum(const JSON_Value* value, std::span<const std::pair<std::string_view, int>> mapping);
template std::optional<long> utils::parse_enum(const JSON_Value* value, std::span<const std::pair<std::string_view, long>> mapping);
template std::optional<long long> utils::parse_enum(const JSON_Value* value,
    std::span<const std::pair<std::string_view, long long>> mapping);

template std::optional<signed char> utils::parse_enum(const JSON_Object* object, const char* name,
    std::span<const std::pair<std::string_view, signed char>> mapping);
template std::optional<short> utils::parse_enum(const JSON_Object* object, const char* name,
    std::span<const std::pair<std::string_view, short>> mapping);
template std::optional<int> utils::parse_enum(const JSON_Object* object, const char* name,
    std::span<const std::pair<std::string_view, int>> mapping);
template std::optional<long> utils::parse_enum(const JSON_Object* object, const char* name,
    std::span<const std::pair<std::string_view, long>> mapping);
template std::optional<long long> utils::parse_enum(const JSON_Object* object, const char* name,
    std::span<const std::pair<std::string_view, long long>> mapping);

template std::optional<unsigned char> utils::parse_enum(const JSON_Value* value,
    std::span<const std::pair<std::string_view, unsigned char>> mapping);
template std::optional<unsigned short> utils::parse_enum(const JSON_Value* value,
    std::span<const std::pair<std::string_view, unsigned short>> mapping);
template std::optional<unsigned> utils::parse_enum(const JSON_Value* value, std::span<const std::pair<std::string_view, unsigned>> mapping);
template std::optional<unsigned long> utils::parse_enum(const JSON_Value* value,
    std::span<const std::pair<std::string_view, unsigned long>> mapping);
template std::optional<unsigned long long> utils::parse_enum(const JSON_Value* value,
    std::span<const std::pair<std::string_view, unsigned long long>> mapping);

template std::optional<unsigned char> utils::parse_enum(const JSON_Object* object, const char* name,
    std::span<const std::pair<std::string_view, unsigned char>> mapping);
template std::optional<unsigned short> utils::parse_enum(const JSON_Object* object, const char* name,
    std::span<const std::pair<std::string_view, unsigned short>> mapping);
template std::optional<unsigned> utils::parse_enum(const JSON_Object* object, const char* name,
    std::span<const std::pair<std::string_view, unsigned>> mapping);
template std::optional<unsigned long> utils::parse_enum(const JSON_Object* object, const char* name,
    std::span<const std::pair<std::string_view, unsigned long>> mapping);
template std::optional<unsigned long long> utils::parse_enum(const JSON_Object* object, const char* name,
    std::span<const std::pair<std::string_view, unsigned long long>> mapping);
