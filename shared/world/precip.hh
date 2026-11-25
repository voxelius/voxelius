#ifndef E4A7C2D1_9B3F_4C58_A6D2_7F1E3B9C5D40
#define E4A7C2D1_9B3F_4C58_A6D2_7F1E3B9C5D40

#include "core/identifier.hh"

#include "shared/world/weather.hh"

class World;

struct PrecipDefinition final {
    Identifier name;
    precip_type_id id { PRECIP_ID_NULL };

    float temperature_min { -1000.0f };
    float temperature_max { 1000.0f };
    float temperature_fade { 0.0f };

    float humidity_min { 0.0f };
    float humidity_max { 1.0f };
    float humidity_fade { 0.0f };

    float priority { 1.0f };

    Identifier texture;
    float quad_width { 1.0f };
    float band_half_height { 12.0f };
    float fall_speed { 4.0f };  // Tiles per second along V
    float draw_chance { 1.0f }; // A value above one draws every column
    float opacity { 0.6f };
    Eigen::Vector3f color { Eigen::Vector3f::Ones() };
};

struct PrecipContext final {
    weather_type_id front { WEATHER_ID_NULL };
    const WeatherTypeDefinition* definition { nullptr };
    float coverage { 0.0f };
    float effective_intensity { 0.0f }; // Coverage times the front's intensity
};

struct PrecipSelection final {
    precip_type_id type { PRECIP_ID_NULL };
    float effective_intensity { 0.0f };
};

namespace precip
{
PrecipContext context(const World* world, std::span<const weather_type_id> allowed, float x, float z);
PrecipSelection select(const World* world, const PrecipContext& ctx, float x, float y, float z);
PrecipSelection select(const PrecipContext& ctx, float temperature, float humidity);
} // namespace precip

#endif /* E4A7C2D1_9B3F_4C58_A6D2_7F1E3B9C5D40 */
