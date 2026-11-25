#include "core/pch.hh"

#include "core/utils/string.hh"

constexpr static char SINGLE_QUOTE_CHAR = '\'';
constexpr static char DOUBLE_QUOTE_CHAR = '\"';
constexpr static char BACKSLASH_CHAR = '\\';

constexpr static std::array<char, 7> WHITESPACE_CHARS = { ' ', '\t', '\n', '\v', '\f', '\r', 0 };

static bool is_whitespace_char(char character)
{
    return std::ranges::any_of(WHITESPACE_CHARS, [character](char test) {
        return character == test;
    });
}

bool utils::is_whitespace(std::string_view string)
{
    if(string.empty()) {
        return true;
    }

    if(string.size() == 1 && string[0] == 0) {
        return true;
    }

    return string.find_first_not_of(WHITESPACE_CHARS.data()) == std::string_view::npos;
}

bool utils::has_whitespace(std::string_view string)
{
    return string.find_first_of(WHITESPACE_CHARS.data()) != std::string_view::npos;
}

std::string_view utils::skip_whitespace(std::string_view string)
{
    std::size_t position = 0;

    while(position < string.size() && is_whitespace_char(string[position])) {
        position += 1;
    }

    return string.substr(position);
}

std::string_view utils::trim_whitespace(std::string_view string)
{
    auto start = string.find_first_not_of(WHITESPACE_CHARS.data());
    auto end = string.find_last_not_of(WHITESPACE_CHARS.data());

    if(start == std::string_view::npos || end == std::string_view::npos) {
        return {};
    }

    return string.substr(start, end - start + 1);
}

std::string_view utils::remove_comments(std::string_view string)
{
    auto comment_position = string.find_first_of('#');

    if(comment_position == std::string_view::npos) {
        return string;
    }

    return string.substr(0, comment_position);
}

std::vector<std::string_view> utils::tokenize(std::string_view string)
{
    std::vector<std::string_view> tokens;
    std::size_t position = 0;

    while(position < string.size()) {
        while(position < string.size() && is_whitespace_char(string[position])) {
            position += 1;
        }

        if(position >= string.size()) {
            break;
        }

        char quote_char = 0;

        if(string[position] == SINGLE_QUOTE_CHAR || string[position] == DOUBLE_QUOTE_CHAR) {
            quote_char = string[position];

            position += 1;
        }

        std::size_t token_start = position;

        if(quote_char) {
            while(position < string.size() && string[position] != quote_char) {
                position += 1;
            }

            tokens.push_back(string.substr(token_start, position - token_start));

            if(position < string.size()) {
                position += 1;
            }
        }
        else {
            while(position < string.size() && !is_whitespace_char(string[position])) {
                position += 1;
            }

            tokens.push_back(string.substr(token_start, position - token_start));
        }
    }

    return tokens;
}

std::vector<std::string> utils::tokenize_esc(std::string_view string)
{
    std::vector<std::string> tokens;
    std::size_t position = 0;

    while(position < string.size()) {
        while(position < string.size() && is_whitespace_char(string[position])) {
            position += 1;
        }

        if(position >= string.size()) {
            break;
        }

        char quote_char = 0;

        if(string[position] == SINGLE_QUOTE_CHAR || string[position] == DOUBLE_QUOTE_CHAR) {
            quote_char = string[position];

            position += 1;
        }

        std::string token;

        if(quote_char) {
            while(position < string.size() && string[position] != quote_char) {
                if(string[position] == BACKSLASH_CHAR && position + 1 < string.size()) {
                    token.push_back(string[position + 1]);
                    position += 2;
                }
                else {
                    token.push_back(string[position]);
                    position += 1;
                }
            }

            tokens.push_back(std::move(token));

            if(position < string.size()) {
                position += 1;
            }
        }
        else {
            std::size_t token_start = position;

            while(position < string.size() && !is_whitespace_char(string[position])) {
                position += 1;
            }

            tokens.push_back(std::string(string.substr(token_start, position - token_start)));
        }
    }

    return tokens;
}

std::string utils::join(std::span<const std::string> parts, std::string_view separator)
{
    if(parts.empty()) {
        return {};
    }

    auto total_length = separator.size() * (parts.size() - 1);

    for(const auto& part : parts) {
        total_length += part.size();
    }

    std::string result;
    result.reserve(total_length);

    auto it = parts.begin();
    result.append(*it);
    it = std::next(it);

    while(true) {
        if(it == parts.end()) {
            break;
        }

        result.append(separator);
        result.append(*it);

        it = std::next(it);
    }

    return result;
}

std::string utils::join(std::span<const std::string_view> parts, std::string_view separator)
{
    if(parts.empty()) {
        return {};
    }

    auto total_length = separator.size() * (parts.size() - 1);

    for(const auto& part : parts) {
        total_length += part.size();
    }

    std::string result;
    result.reserve(total_length);

    auto it = parts.begin();
    result.append(*it);
    it = std::next(it);

    while(true) {
        if(it == parts.end()) {
            break;
        }

        result.append(separator);
        result.append(*it);

        it = std::next(it);
    }

    return result;
}

std::string utils::escape_quotations(std::string_view string)
{
    std::string result;
    result.reserve(string.size());

    for(const auto& character : string) {
        if(character == SINGLE_QUOTE_CHAR || character == DOUBLE_QUOTE_CHAR) {
            result.push_back(BACKSLASH_CHAR);
        }

        result.push_back(character);
    }

    return result;
}
