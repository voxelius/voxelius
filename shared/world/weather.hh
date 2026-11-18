#ifndef A6B2C1D9_4E7F_4A3B_9C51_2F0D8E6B7A14
#define A6B2C1D9_4E7F_4A3B_9C51_2F0D8E6B7A14

#include "core/identifier.hh"

#include "shared/world/biome.hh"

namespace packet
{
struct World_Info;
struct World_Weather;
} // namespace packet

using weather_type_id = std::uint32_t;

constexpr static weather_type_id WEATHER_ID_NULL = 0;

constexpr static std::size_t WEATHER_MAX_LAYERS = 4;

enum class precipitation_kind : std::uint8_t {
    NONE = 0,
    RAIN,
    SNOW,
};

struct CloudLayer final {
    float height { 192.0f };  // World Y of the bottom of the volume, in blocks
    float thickness { 8.0f }; // How far the volume reaches up from the bottom, in blocks; the client divides it into whole voxels
    float opacity { 0.9f };   // How opaque the deck is where the field covers it
    float scale { 128.0f };   // Blocks per noise tile; controls the size of the cloud massesы
    float coverage { 0.0f };  // Base density, 0 to 1

    Eigen::Vector3f shadow_color { Eigen::Vector3f(0.55f, 0.57f, 0.62f) };
    Eigen::Vector3f lit_color { Eigen::Vector3f(0.80f, 0.82f, 0.86f) };

    std::uint32_t seed { 0 };
};

struct Precipitation final {
    precipitation_kind kind { precipitation_kind::NONE };
    float intensity { 0.0f };
    float speed { 18.0f };
    float drift { 0.0f };
    float size { 0.08f };

    Eigen::Vector3f color { Eigen::Vector3f(0.55f, 0.65f, 0.80f) };
};

struct WeatherCallback final {
    std::shared_ptr<lua_State> state { nullptr };
    int callback_ref { LUA_NOREF };
};

struct WeatherTypeDefinition final {
    Identifier name;
    weather_type_id id { WEATHER_ID_NULL };
    std::uint32_t realms_bits { 0 }; // A type is only ever selected in the realms these bits name

    std::vector<CloudLayer> layers;

    Precipitation precipitation;

    Eigen::Vector3f sky_tint { Eigen::Vector3f(1.0f, 1.0f, 1.0f) };
    float sky_visibility { 1.0f }; // 1 = the clear sky shows through, 0 = only sky_tint
    float light_scale { 1.0f };

    float intensity { 1.0f };
    float wind_speed { 0.0f };

    std::uint32_t weight { 1 }; // Relative size of this type's slice of the type grid

    float strikes_per_minute { 0.0f };
    float strike_radius { 96.0f };

    WeatherCallback on_tick;
    WeatherCallback on_strike;
};

struct WeatherField final {
    Eigen::Vector2f drift { Eigen::Vector2f::Zero() }; // Where the noise pattern has drifted to, in blocks; the wind moves it
    float scale { 512.0f };                            // Blocks per noise cell; far larger than a chunk on purpose
    std::uint32_t seed { 0 };
};

struct WeatherRealmState final {
    // One field picks the type, the other one drives how strong it
    // is right here. Both drift with the wind and are rebuilt from a
    // seed on both sides, so no per-cell data travels over the network
    WeatherField types;
    WeatherField detail;

    Eigen::Vector2f wind { Eigen::Vector2f::Zero() };
};

struct WeatherBlend final {
    float coverage;
    Eigen::Vector3f sky_tint;
    float sky_visibility { 1.0f };
    float light_scale;
};

namespace weather
{
std::span<WeatherRealmState, NUM_BIOME_REALMS> states(void);
} // namespace weather

namespace weather
{
WeatherRealmState& state(biome_realm realm);
} // namespace weather

namespace weather
{
void reset(void);
void advance(float delta_time);
void force_at(biome_realm realm, weather_type_id type, float x, float z);
} // namespace weather

namespace weather
{
float noise_at(float x, float z, std::uint32_t seed);
float cell_random(std::int32_t x, std::int32_t z, std::uint32_t seed);
} // namespace weather

namespace weather
{
weather_type_id type_at(biome_realm realm, float x, float z);
float type_weight(biome_realm realm, weather_type_id type, float x, float z);
float coverage_of(biome_realm realm, weather_type_id type, float x, float z);
float coverage(biome_realm realm, float x, float z);
} // namespace weather

namespace weather
{
float type_weight_in_field(const WeatherRealmState& state, biome_realm realm, weather_type_id type, float fx, float fz);
float coverage_in_field(const WeatherRealmState& state, biome_realm realm, weather_type_id type, float fx, float fz);
} // namespace weather

namespace weather
{
WeatherBlend blend(biome_realm realm, float x, float z);
WeatherBlend blend_overhead(float x, float y, float z);
WeatherBlend blend_region(float x, float y, float z, float radius);
} // namespace weather

namespace weather
{
void fill_info(packet::World_Info& packet);
void apply_info(const packet::World_Info& packet);
} // namespace weather

namespace weather
{
void fill_packet(packet::World_Weather& packet);
void apply_packet(const packet::World_Weather& packet, bool baseline);
} // namespace weather

#endif /* A6B2C1D9_4E7F_4A3B_9C51_2F0D8E6B7A14 */
