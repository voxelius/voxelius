#include "shared/pch.hh"

#include "shared/cvar/server.hh"

#include "core/cvar/host.hh"

#include "shared/constant.hh"

constexpr static unsigned DAY_LENGTH_MIN = 100;
constexpr static unsigned DAY_LENGTH_MAX = 1000000;

cvar::Number<unsigned> server_vars::day_length { static_cast<unsigned>(constant::DAY_LENGTH_TICKS), DAY_LENGTH_MIN, DAY_LENGTH_MAX };

std::uint32_t server_vars::day_length_ticks(void)
{
    return static_cast<std::uint32_t>(day_length.value());
}

void server_vars::init(void)
{
    auto& day_length_binding = cvar::bind_var(day_length, "sv_day_length");
    day_length_binding.set_help("Length of a full day, in ticks");
    day_length_binding.set_flags(FCVAR_REPLICATED | FCVAR_ARCHIVE | FCVAR_ADMIN);
}
