#include "server/pch.hh"

#include "server/net/operators.hh"

#include "core/utils/crc64.hh"
#include "core/utils/physfs.hh"
#include "core/utils/string.hh"

constexpr static std::string_view OPERATORS_FILE = "operators.txt";

static vx::hash_map<std::uint64_t, ed25519::pkey_type> s_operators;

static void write_operators(void)
{
    std::ostringstream stream;

    for(const auto it : s_operators) {
        auto xstr = ed25519::export_pkey(it.second);

        stream << xstr;
        stream << std::endl;
    }

    utils::write_file(OPERATORS_FILE, stream.str());
}

void operators::init(void)
{
    std::string line;
    std::istringstream stream;

    if(utils::read_file(OPERATORS_FILE, stream)) {
        while(std::getline(stream, line)) {
            auto trimmed = utils::trim_whitespace(utils::remove_comments(line));

            if(trimmed.empty()) {
                continue;
            }

            auto pkey = ed25519::import_pkey(trimmed);

            if(pkey.has_value()) {
                auto hash = utils::crc64(pkey.value());

                s_operators.insert_or_assign(hash, std::move(pkey.value()));
            }
            else {
                msg::log_warning("{}: malformed public key {}", OPERATORS_FILE, trimmed);
            }
        }
    }
}

void operators::shutdown(void)
{
    write_operators();
}

void operators::add(const ed25519::pkey_type& pkey)
{
    auto hash = utils::crc64(pkey);

    s_operators.insert_or_assign(hash, ed25519::pkey_type(pkey));

    write_operators();
}

void operators::remove(const ed25519::pkey_type& pkey)
{
    auto hash = utils::crc64(pkey);

    s_operators.erase(hash);

    write_operators();
}

bool operators::contains(const ed25519::pkey_type& pkey)
{
    auto hash = utils::crc64(pkey);

    return s_operators.contains(hash);
}
