#include "shared/pch.hh"

#include "shared/cvar/gamerules.hh"

#include "core/cvar/host.hh"

cvar::Boolean gamerules::allow_building { true };
cvar::Boolean gamerules::daylight_cycle { true };
cvar::Boolean gamerules::weather { true };
cvar::Number<float> gamerules::reach_distance { 8.0f, 0.0f, 32.0f };

void gamerules::init(void)
{
    auto& gr_allow_building_binding = cvar::bind_var(allow_building, "gr_allow_building");
    gr_allow_building_binding.set_help("Allow players to break and place blocks");
    gr_allow_building_binding.set_flags(FCVAR_REPLICATED | FCVAR_ARCHIVE | FCVAR_ADMIN);

    auto& gr_daylight_cycle_binding = cvar::bind_var(daylight_cycle, "gr_daylight_cycle");
    gr_daylight_cycle_binding.set_help("Advance the day and night cycle");
    gr_daylight_cycle_binding.set_flags(FCVAR_REPLICATED | FCVAR_ARCHIVE | FCVAR_ADMIN);

    auto& gr_weather_cycle_binding = cvar::bind_var(weather, "gr_weather_cycle");
    gr_weather_cycle_binding.set_help("Advance the weather simulation");
    gr_weather_cycle_binding.set_flags(FCVAR_REPLICATED | FCVAR_ARCHIVE | FCVAR_ADMIN);

    auto& gr_reach_distance_binding = cvar::bind_var(reach_distance, "gr_reach_distance");
    gr_reach_distance_binding.set_help("Maximum block interaction distance");
    gr_reach_distance_binding.set_flags(FCVAR_REPLICATED | FCVAR_ARCHIVE | FCVAR_ADMIN);
}
