#include "core/pch.hh"

#include "core/cmdline.hh"

constexpr static char OPTION_PREFIX = '-';

static vx::hash_map<std::string, std::string> s_options;

static bool is_option_argv(std::string_view argv_string)
{
    if(argv_string.find_last_of(OPTION_PREFIX) >= (argv_string.size() - 1)) {
        return false;
    }

    return argv_string[0] == OPTION_PREFIX;
}

static std::string_view get_option(std::string_view argv_string)
{
    std::size_t i;

    for(i = 0; argv_string[i] == OPTION_PREFIX; ++i) {
        // empty
    }

    return argv_string.substr(i);
}

void cmdline::create(int argc, char** argv)
{
    for(int i = 1; i < argc; ++i) {
        std::string_view argv_string(argv[i]);

        if(is_option_argv(argv_string)) {
            auto option = get_option(argv_string);
            auto next_index = i + 1;

            if(next_index < argc) {
                std::string_view next_argv(argv[next_index]);

                if(!is_option_argv(next_argv)) {
                    insert_option(option, next_argv);
                    i = next_index;
                    continue;
                }
            }

            insert_option(option);
        }
        else {
            msg::log_warning("invalid option: {}", argv_string);
        }
    }
}

void cmdline::insert_option(std::string_view option)
{
    s_options.insert_or_assign(std::string(option), std::string());
}

void cmdline::insert_option(std::string_view option, std::string_view value)
{
    s_options.insert_or_assign(std::string(option), std::string(value));
}

bool cmdline::contains(std::string_view option)
{
    return s_options.contains(option);
}

std::optional<std::string_view> cmdline::value(std::string_view option)
{
    auto iter = s_options.find(option);

    if(iter == s_options.cend()) {
        return std::nullopt;
    }

    return std::string_view(iter->second);
}

std::string_view cmdline::value_or(std::string_view option, std::string_view default_value)
{
    auto iter = s_options.find(option);

    if(iter == s_options.cend() || iter->second.empty()) {
        return default_value;
    }

    return iter->second;
}

const char* cmdline::value_cstr(std::string_view option)
{
    auto iter = s_options.find(option);

    if(iter == s_options.cend()) {
        return nullptr;
    }

    return iter->second.c_str();
}

const char* cmdline::value_or_cstr(std::string_view option, const char* default_value)
{
    auto iter = s_options.find(option);

    if(iter == s_options.cend() || iter->second.empty()) {
        return default_value;
    }

    return iter->second.c_str();
}
