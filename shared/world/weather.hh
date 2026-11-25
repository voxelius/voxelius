#ifndef A6B2C1D9_4E7F_4A3B_9C51_2F0D8E6B7A14
#define A6B2C1D9_4E7F_4A3B_9C51_2F0D8E6B7A14

#include "core/identifier.hh"

using weather_type_id = std::uint32_t;
using precip_type_id = std::uint32_t;

constexpr static weather_type_id WEATHER_ID_NULL = 0;
constexpr static precip_type_id PRECIP_ID_NULL = 0;

struct CloudBody final {
    float height { 192.0f };
    float thickness { 8.0f };
    float opacity { 0.9f };
    float scale { 128.0f };
    float coverage { 0.0f };

    Eigen::Vector3f shadow_color { Eigen::Vector3f(0.55f, 0.57f, 0.62f) };
    Eigen::Vector3f lit_color { Eigen::Vector3f(0.80f, 0.82f, 0.86f) };

    std::uint32_t seed { 0 };
};

// A front names several precip types, and the local climate picks
// one of them per point. The ids are resolved by weather_registry::commit
// after the precip registry has committed all of this mod's definitions
struct PrecipFront final {
    std::vector<Identifier> type_names;
    std::vector<precip_type_id> types;

    float intensity { 0.0f };
};

struct WeatherCallback final {
    std::shared_ptr<lua_State> state { nullptr };
    int callback_ref { LUA_NOREF };
};

struct WeatherTypeDefinition final {
    Identifier name;
    weather_type_id id { WEATHER_ID_NULL };

    CloudBody cloud;

    PrecipFront precip;

    Eigen::Vector3f sky_tint { Eigen::Vector3f(1.0f, 1.0f, 1.0f) };
    float sky_visibility { 1.0f };
    float light_scale { 1.0f };

    float intensity { 1.0f };

    Eigen::Vector2f target { Eigen::Vector2f(0.5f, 0.5f) };
    float weight { 1.0f };

    float strikes_per_minute { 0.0f };
    float strike_radius { 96.0f };

    WeatherCallback on_tick;
    WeatherCallback on_strike;
};

struct WeatherBlend final {
    float coverage;
    Eigen::Vector3f sky_tint;
    float sky_visibility { 1.0f };
    float light_scale;
};

namespace weather
{
std::uint32_t star_seed(std::uint64_t weather_seed);
} // namespace weather

// Weather is a pure function of the world's weather seed, the world's
// weather tick and the world position. Two noise grids form the two axes
// of a climate square; every type owns a target point in it and a type
// covers the points closest to its target, so the field needs no state
// and travels over the network as one tick value
class WeatherState final {
public:
    void reseed(std::uint64_t weather_seed);

    constexpr std::uint64_t seed(void) const;

    Eigen::Vector2f drift(std::uint64_t weather_tick) const;

    Eigen::Vector2f wind_direction(void) const;
    float wind_speed(void) const;

    // An empty allowed span disables the filter
    weather_type_id type_at(std::span<const weather_type_id> allowed, float x, float z, std::uint64_t weather_tick) const;
    float coverage_of(std::span<const weather_type_id> allowed, weather_type_id type, float x, float z, std::uint64_t weather_tick) const;
    float coverage(std::span<const weather_type_id> allowed, float x, float z, std::uint64_t weather_tick) const;
    WeatherBlend blend(std::span<const weather_type_id> allowed, float x, float z, std::uint64_t weather_tick) const;

    float coverage_in_field(std::span<const weather_type_id> allowed, weather_type_id type, float fx, float fz) const;

    // A forced front is a jump of the weather clock: the first tick at or
    // after from_tick where the type covers the point wins, so both sides
    // keep deriving the same field from the same tick. Returns that tick,
    // or nothing when no tick within the horizon brings the type there
    std::optional<std::uint64_t> solve_force(std::span<const weather_type_id> allowed, weather_type_id type, float x, float z,
        std::uint64_t from_tick) const;

private:
    Eigen::Vector2d drift_double(std::uint64_t weather_tick) const;

    float climate_a_in_field(float fx, float fz) const;
    float climate_b_in_field(float fx, float fz) const;
    float share_in_field(std::span<const weather_type_id> allowed, weather_type_id type, float fx, float fz) const;
    float share_at(std::span<const weather_type_id> allowed, weather_type_id type, float x, float z, std::uint64_t weather_tick) const;

    std::uint64_t m_seed { 0 };
    std::uint32_t m_seed_a { 0 };
    std::uint32_t m_seed_b { 0 };
    Eigen::Vector2f m_wind_direction { Eigen::Vector2f::Zero() };
    float m_wind_speed { 0.0f };
};

constexpr std::uint64_t WeatherState::seed(void) const
{
    return m_seed;
}

#endif /* A6B2C1D9_4E7F_4A3B_9C51_2F0D8E6B7A14 */
