#include "core/pch.hh"

#include "core/utils/yaml.hh"

static c4::csubstr as_csubstr(std::string_view text)
{
    return c4::csubstr(text.data(), text.size());
}

ryml::ConstNodeRef utils::find_child(ryml::ConstNodeRef parent, std::string_view key)
{
    if(parent.invalid() || !parent.is_map()) {
        return {};
    }

    return parent.find_child(as_csubstr(key));
}

std::optional<std::string_view> utils::parse_string(ryml::ConstNodeRef node)
{
    if(node.invalid() || !node.has_val()) {
        return std::nullopt;
    }

    auto value = node.val();

    return std::string_view(value.str, value.len);
}

std::optional<std::string_view> utils::parse_string(ryml::ConstNodeRef parent, std::string_view key)
{
    return parse_string(find_child(parent, key));
}

template<vx::arithmetic T>
std::optional<T> utils::parse_arithmetic(ryml::ConstNodeRef node)
{
    auto value = parse_string(node);

    if(!value.has_value() || value->empty()) {
        return std::nullopt;
    }

    auto begin = value->data();
    auto end = begin + value->size();
    auto parsed = 0.0;

    auto [ptr, error] = std::from_chars(begin, end, parsed);

    if(error != std::errc {} || ptr != end) {
        return std::nullopt;
    }

    return static_cast<T>(parsed);
}

template<vx::arithmetic T>
std::optional<T> utils::parse_arithmetic(ryml::ConstNodeRef parent, std::string_view key)
{
    return parse_arithmetic<T>(find_child(parent, key));
}

template<std::integral T>
std::optional<T> utils::parse_integer(ryml::ConstNodeRef node)
{
    auto value = parse_arithmetic<double>(node);

    if(!value.has_value()) {
        return std::nullopt;
    }

    auto parsed = value.value();
    auto valid = true;
    valid = valid && std::isfinite(parsed);
    valid = valid && parsed == std::trunc(parsed);
    valid = valid && parsed >= static_cast<double>(std::numeric_limits<T>::lowest());
    valid = valid && parsed < 1.0 + static_cast<double>(std::numeric_limits<T>::max());

    if(valid) {
        return static_cast<T>(parsed);
    }

    return std::nullopt;
}

template<std::integral T>
std::optional<T> utils::parse_integer(ryml::ConstNodeRef parent, std::string_view key)
{
    return parse_integer<T>(find_child(parent, key));
}

std::optional<bool> utils::parse_boolean(ryml::ConstNodeRef node)
{
    auto value = parse_string(node);

    if(!value.has_value()) {
        return std::nullopt;
    }

    auto parsed = value.value();

    if(parsed == "true") {
        return true;
    }

    if(parsed == "false") {
        return false;
    }

    return std::nullopt;
}

std::optional<bool> utils::parse_boolean(ryml::ConstNodeRef parent, std::string_view key)
{
    return parse_boolean(find_child(parent, key));
}

template std::optional<float> utils::parse_arithmetic<float>(ryml::ConstNodeRef);
template std::optional<float> utils::parse_arithmetic<float>(ryml::ConstNodeRef, std::string_view);

template std::optional<double> utils::parse_arithmetic<double>(ryml::ConstNodeRef);
template std::optional<double> utils::parse_arithmetic<double>(ryml::ConstNodeRef, std::string_view);

template std::optional<long long> utils::parse_integer<long long>(ryml::ConstNodeRef);
template std::optional<long long> utils::parse_integer<long long>(ryml::ConstNodeRef, std::string_view);
