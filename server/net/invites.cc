#include "server/pch.hh"

#include "server/net/invites.hh"

#include "server/net/whitelist.hh"

static vx::hash_set<std::uint64_t> s_invites;

static std::uint64_t generate_token(void)
{
    auto seed = ed25519::generate_seed();

    std::uint64_t token;
    std::memcpy(&token, seed.data(), sizeof(token));

    return token;
}

void invites::init(void)
{
    // empty
}

void invites::shutdown(void)
{
    s_invites.clear();
}

std::uint64_t invites::issue(void)
{
    // Zero on the wire means "no invite", so it is never issued.
    std::uint64_t token = generate_token();

    while(token == 0 || s_invites.contains(token)) {
        token = generate_token();
    }

    s_invites.emplace(token);
    return token;
}

bool invites::consume(std::uint64_t token, const ed25519::pkey_type& pkey)
{
    if(s_invites.contains(token)) {
        s_invites.erase(token);
        whitelist::add(pkey);
        return true;
    }

    return false;
}
