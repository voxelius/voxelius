#include "shared/pch.hh"

#include "shared/world/weather.hh"

#include "shared/constant.hh"
#include "shared/utils/weather.hh"
#include "shared/world/weather_registry.hh"

constexpr static float CLIMATE_A_SCALE = 1536.0f; // Blocks per noise cell of the first climate axis
constexpr static float CLIMATE_B_SCALE = 1024.0f; // Blocks per noise cell of the second climate axis
constexpr static float SCORE_FLOOR = 1.0e-4f;     // Keeps a sample that lands on a target finite
constexpr static float WIND_SPEED_MIN = 0.8f;     // Blocks per second the field drifts, at the slow end
constexpr static float WIND_SPEED_MAX = 2.4f;     // Blocks per second the field drifts, at the fast end
constexpr static float FORCE_MIN_SHARE = 0.5f;    // A forced type has to own at least this much of the point

constexpr static std::uint64_t FORCE_HORIZON_TICKS = 7 * 24 * 60 * 60 * constant::FIXED_TICKRATE;

constexpr static std::uint64_t SALT_A = UINT64_C(0xA1B2C3D4E5F60718);
constexpr static std::uint64_t SALT_B = UINT64_C(0x1122334455667788);
constexpr static std::uint64_t SALT_ANGLE = UINT64_C(0x0F1E2D3C4B5A6978);
constexpr static std::uint64_t SALT_SPEED = UINT64_C(0x8877665544332211);
constexpr static std::uint64_t SALT_STAR = UINT64_C(0x5EED5EED5EED5EED);

static std::uint64_t splitmix64(std::uint64_t value)
{
    value += UINT64_C(0x9E3779B97F4A7C15);
    value = (value ^ (value >> 30U)) * UINT64_C(0xBF58476D1CE4E5B9);
    value = (value ^ (value >> 27U)) * UINT64_C(0x94D049BB133111EB);

    return value ^ (value >> 31U);
}

// The high 24 bits are enough for a float and keep the result in [0, 1)
static float unit_float(std::uint64_t value)
{
    return static_cast<float>(value >> 40U) / static_cast<float>(UINT32_C(1) << 24U);
}

static bool is_allowed(const WeatherTypeDefinition& def, std::span<const weather_type_id> allowed)
{
    if(def.id == WEATHER_ID_NULL) {
        return false;
    }

    if(allowed.empty()) {
        return true;
    }

    return std::ranges::contains(allowed, def.id);
}

// Inverse of the weighted distance from the sample to the type's target:
// a type with a bigger weight reaches further, so it covers more of the world
static float score_of(const WeatherTypeDefinition& def, float a, float b)
{
    auto da = a - def.target.x();
    auto db = b - def.target.y();
    auto weight = std::max(def.weight, 1.0f);

    return std::max((da * da + db * db) / weight, SCORE_FLOOR);
}

// Share falls with the cube of the score, so a type that is even a little
// farther in climate space loses most of its presence: the blend is a narrow
// cross-fade around the nearest-target boundary, not a world-wide haze
static float share_of(const WeatherTypeDefinition& def, float a, float b)
{
    auto score = score_of(def, a, b);

    return 1.0f / (score * score * score);
}

std::uint32_t weather::star_seed(std::uint64_t weather_seed)
{
    return static_cast<std::uint32_t>(splitmix64(weather_seed ^ SALT_STAR) >> 32U);
}

void WeatherState::reseed(std::uint64_t weather_seed)
{
    m_seed = weather_seed;
    m_seed_a = static_cast<std::uint32_t>(splitmix64(weather_seed ^ SALT_A));
    m_seed_b = static_cast<std::uint32_t>(splitmix64(weather_seed ^ SALT_B));

    auto angle = unit_float(splitmix64(weather_seed ^ SALT_ANGLE)) * std::numbers::pi_v<float> * 2.0f;
    auto speed = WIND_SPEED_MIN + (WIND_SPEED_MAX - WIND_SPEED_MIN) * unit_float(splitmix64(weather_seed ^ SALT_SPEED));

    m_wind_direction = Eigen::Vector2f(std::cos(angle), std::sin(angle));
    m_wind_speed = speed;
}

Eigen::Vector2f WeatherState::drift(std::uint64_t weather_tick) const
{
    auto offset = drift_double(weather_tick);

    return Eigen::Vector2f(static_cast<float>(offset.x()), static_cast<float>(offset.y()));
}

Eigen::Vector2f WeatherState::wind_direction(void) const
{
    return m_wind_direction;
}

float WeatherState::wind_speed(void) const
{
    return m_wind_speed;
}

weather_type_id WeatherState::type_at(std::span<const weather_type_id> allowed, float x, float z, std::uint64_t weather_tick) const
{
    auto offset = drift_double(weather_tick);
    auto fx = static_cast<float>(static_cast<double>(x) - offset.x());
    auto fz = static_cast<float>(static_cast<double>(z) - offset.y());

    auto a = climate_a_in_field(fx, fz);
    auto b = climate_b_in_field(fx, fz);

    auto best = WEATHER_ID_NULL;
    auto best_score = std::numeric_limits<float>::max();

    for(const auto& def : weather_registry::all_definitions()) {
        if(!is_allowed(def, allowed)) {
            continue;
        }

        auto score = score_of(def, a, b);

        if(score < best_score) {
            best_score = score;
            best = def.id;
        }
    }

    return best;
}

float WeatherState::coverage_of(std::span<const weather_type_id> allowed, weather_type_id type, float x, float z,
    std::uint64_t weather_tick) const
{
    auto def = weather_registry::find_definition(type);

    if(def == nullptr) {
        return 0.0f;
    }

    auto share = share_at(allowed, type, x, z, weather_tick);
    auto intensity = std::clamp(def->intensity, 0.0f, 1.0f);

    return std::clamp(share * intensity, 0.0f, 1.0f);
}

float WeatherState::coverage(std::span<const weather_type_id> allowed, float x, float z, std::uint64_t weather_tick) const
{
    return coverage_of(allowed, type_at(allowed, x, z, weather_tick), x, z, weather_tick);
}

WeatherBlend WeatherState::blend(std::span<const weather_type_id> allowed, float x, float z, std::uint64_t weather_tick) const
{
    WeatherBlend result {};
    result.sky_tint = Eigen::Vector3f::Ones();
    result.sky_visibility = 1.0f;
    result.light_scale = 1.0f;

    auto offset = drift_double(weather_tick);
    auto fx = static_cast<float>(static_cast<double>(x) - offset.x());
    auto fz = static_cast<float>(static_cast<double>(z) - offset.y());

    auto a = climate_a_in_field(fx, fz);
    auto b = climate_b_in_field(fx, fz);

    auto total = 0.0f;

    for(const auto& def : weather_registry::all_definitions()) {
        if(is_allowed(def, allowed)) {
            total += share_of(def, a, b);
        }
    }

    if(total <= 0.0f) {
        return result;
    }

    Eigen::Vector3f tint = Eigen::Vector3f::Zero();
    auto visibility = 0.0f;
    auto light = 0.0f;
    auto coverage = 0.0f;

    for(const auto& def : weather_registry::all_definitions()) {
        if(!is_allowed(def, allowed)) {
            continue;
        }

        // The sky follows how much of the point the type owns; intensity
        // only says how strong the type is where it does cover the point
        auto amount = share_of(def, a, b) / total;

        tint += def.sky_tint * amount;
        visibility += def.sky_visibility * amount;
        light += def.light_scale * amount;
        coverage += amount * std::clamp(def.intensity, 0.0f, 1.0f);
    }

    // The shares add up to one, so the colour is already an average;
    // only the coverage still has to be clamped to a single layer
    coverage = std::min(coverage, 1.0f);

    // sky_tint is a target colour, not a multiplier: mixing towards
    // grey is what turns a blue sky into overcast grey instead of dark blue.
    // sky_visibility says how much of the real sky survives that mix
    result.coverage = coverage;
    result.sky_tint = tint;
    result.sky_visibility = std::clamp(visibility, 0.0f, 1.0f);
    result.light_scale = light;

    return result;
}

float WeatherState::coverage_in_field(std::span<const weather_type_id> allowed, weather_type_id type, float fx, float fz) const
{
    auto def = weather_registry::find_definition(type);

    if(def == nullptr) {
        return 0.0f;
    }

    auto share = share_in_field(allowed, type, fx, fz);
    auto intensity = std::clamp(def->intensity, 0.0f, 1.0f);

    return std::clamp(share * intensity, 0.0f, 1.0f);
}

std::optional<std::uint64_t> WeatherState::solve_force(std::span<const weather_type_id> allowed, weather_type_id type, float x, float z,
    std::uint64_t from_tick) const
{
    if(weather_registry::find_definition(type) == nullptr) {
        return std::nullopt;
    }

    auto last_tick = from_tick + FORCE_HORIZON_TICKS;

    for(auto tick = from_tick; tick < last_tick; tick += constant::FIXED_TICKRATE) {
        if(type_at(allowed, x, z, tick) != type) {
            continue;
        }

        if(share_at(allowed, type, x, z, tick) < FORCE_MIN_SHARE) {
            continue;
        }

        return tick;
    }

    return std::nullopt;
}

Eigen::Vector2d WeatherState::drift_double(std::uint64_t weather_tick) const
{
    auto seconds = static_cast<double>(weather_tick) / static_cast<double>(constant::FIXED_TICKRATE);
    auto distance = static_cast<double>(m_wind_speed) * seconds;

    return Eigen::Vector2d(static_cast<double>(m_wind_direction.x()) * distance, static_cast<double>(m_wind_direction.y()) * distance);
}

float WeatherState::climate_a_in_field(float fx, float fz) const
{
    return utils::weather_noise(fx / CLIMATE_A_SCALE, fz / CLIMATE_A_SCALE, m_seed_a);
}

float WeatherState::climate_b_in_field(float fx, float fz) const
{
    return utils::weather_noise(fx / CLIMATE_B_SCALE, fz / CLIMATE_B_SCALE, m_seed_b);
}

float WeatherState::share_in_field(std::span<const weather_type_id> allowed, weather_type_id type, float fx, float fz) const
{
    auto a = climate_a_in_field(fx, fz);
    auto b = climate_b_in_field(fx, fz);

    auto total = 0.0f;
    auto own = 0.0f;

    for(const auto& def : weather_registry::all_definitions()) {
        if(!is_allowed(def, allowed)) {
            continue;
        }

        auto share = share_of(def, a, b);
        total += share;

        if(def.id == type) {
            own = share;
        }
    }

    if(total <= 0.0f) {
        return 0.0f;
    }

    return own / total;
}

float WeatherState::share_at(std::span<const weather_type_id> allowed, weather_type_id type, float x, float z,
    std::uint64_t weather_tick) const
{
    auto offset = drift_double(weather_tick);
    auto fx = static_cast<float>(static_cast<double>(x) - offset.x());
    auto fz = static_cast<float>(static_cast<double>(z) - offset.y());

    return share_in_field(allowed, type, fx, fz);
}
