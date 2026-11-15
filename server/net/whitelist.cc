#include "server/pch.hh"

#include "server/net/whitelist.hh"

#include "core/cvar/host.hh"
#include "core/utils/crc64.hh"
#include "core/utils/physfs.hh"

constexpr static std::string_view WHITELIST_FILE = "whitelist.txt";

static vx::hash_map<std::uint64_t, ed25519::pkey_type> s_whitelist;

cvar::Boolean whitelist::enabled { false };

void whitelist::init(void)
{
    auto& sv_whitelist_binding = cvar::bind_var(enabled, "sv_whitelist");
    sv_whitelist_binding.set_help("Only allow whitelisted players");
    sv_whitelist_binding.set_flags(FCVAR_ARCHIVE);

    std::string line;
    std::istringstream stream;

    if(utils::read_file(WHITELIST_FILE, stream)) {
        while(std::getline(stream, line)) {
            auto pkey = ed25519::import_pkey(line);

            if(pkey.has_value()) {
                auto hash = utils::crc64(pkey.value());

                s_whitelist.insert_or_assign(hash, std::move(pkey.value()));
            }
        }
    }
}

void whitelist::shutdown(void)
{
    std::ostringstream stream;

    for(const auto it : s_whitelist) {
        auto xstr = ed25519::export_pkey(it.second);

        stream << xstr;
        stream << std::endl;
    }

    utils::write_file(WHITELIST_FILE, stream.str());
}

void whitelist::add(const ed25519::pkey_type& pkey)
{
    auto hash = utils::crc64(pkey);

    s_whitelist.insert_or_assign(hash, ed25519::pkey_type(pkey));
}

void whitelist::remove(const ed25519::pkey_type& pkey)
{
    auto hash = utils::crc64(pkey);

    s_whitelist.erase(hash);
}

bool whitelist::contains(const ed25519::pkey_type& pkey)
{
    auto hash = utils::crc64(pkey);

    return s_whitelist.contains(hash);
}
