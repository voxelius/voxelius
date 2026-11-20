#include "shared/pch.hh"

#include "shared/cvar/gamerules.hh"

#include "core/cvar/host.hh"

cvar::Boolean gamerules::allow_building { true };
cvar::Boolean gamerules::daylight_cycle { true };
cvar::Boolean gamerules::weather { true };
cvar::Number<float> gamerules::reach_distance { 8.0f, 0.0f, 32.0f };

void gamerules::init(void)
{
    auto& allow_building_binding = cvar::bind_var(allow_building, "gr_allow_building");
    allow_building_binding.set_help("Allow players to break and place blocks");
    allow_building_binding.set_flags(FCVAR_REPLICATED | FCVAR_ARCHIVE | FCVAR_ADMIN);

    auto& daylight_cycle_binding = cvar::bind_var(daylight_cycle, "gr_daylight_cycle");
    daylight_cycle_binding.set_help("Advance the day and night cycle");
    daylight_cycle_binding.set_flags(FCVAR_REPLICATED | FCVAR_ARCHIVE | FCVAR_ADMIN);

    auto& weather_binding = cvar::bind_var(weather, "gr_weather_cycle");
    weather_binding.set_help("Advance the weather simulation");
    weather_binding.set_flags(FCVAR_REPLICATED | FCVAR_ARCHIVE | FCVAR_ADMIN);

    auto& reach_distance_binding = cvar::bind_var(reach_distance, "gr_reach_distance");
    reach_distance_binding.set_help("Maximum block interaction distance");
    reach_distance_binding.set_flags(FCVAR_REPLICATED | FCVAR_ARCHIVE | FCVAR_ADMIN);
}
