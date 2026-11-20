#include "shared/pch.hh"

#include "shared/world/weather.hh"

#include "core/utils/interpolation.hh"

#include "shared/cvar/gamerules.hh"
#include "shared/globals.hh"
#include "shared/net/packet_world.hh"
#include "shared/world/weather_registry.hh"
#include "shared/world/world.hh"

static WeatherState& active_state(void)
{
    static WeatherState fallback {};

    if(globals::world) {
        return globals::world->weather();
    }

    return fallback;
}

static std::uint32_t lattice_hash(std::int32_t x, std::int32_t z, std::uint32_t seed)
{
    std::uint32_t h = static_cast<std::uint32_t>(x) * UINT32_C(374761393);
    h += static_cast<std::uint32_t>(z) * UINT32_C(668265263);
    h += seed * UINT32_C(2246822519);

    h ^= h >> 13;
    h *= UINT32_C(1274126177);
    h ^= h >> 16;

    return h;
}

// The field-space pair below takes coordinates that already
// have the drift removed. A caller that walks the field cell by
// cell (the cloud renderer) uses them so that its sample points do
// not depend on how far the field has drifted: subtracting
// two large world-space floats rounds differently every time
static float type_noise_in_field(const WeatherState& state, float fx, float fz)
{
    auto scale = std::max(state.types.scale, 1.0f);

    return weather::noise_at(fx / scale, fz / scale, state.types.seed);
}

static std::span<const weather_type_id> allowed_weather(void)
{
    if(globals::world == nullptr) {
        return {};
    }

    return globals::world->definition().weather;
}

static float type_noise(float x, float z)
{
    auto& state = weather::state();

    return type_noise_in_field(state, x - state.types.drift.x(), z - state.types.drift.y());
}

static float interval_weight(weather_type_id type, float roll)
{
    // The band is what turns a hard slice boundary into a gradual fade
    constexpr static float BAND = 0.05f;

    auto allowed = allowed_weather();

    if(auto interval = weather_registry::type_interval(allowed, type)) {
        auto [start, end] = interval.value();

        auto fade_in = utils::smoothstep(start - BAND, start + BAND, roll);
        auto fade_out = 1.0f - utils::smoothstep(end - BAND, end + BAND, roll);

        return fade_in * fade_out;
    }

    return 0.0f;
}

static float detail_coverage_in_field(const WeatherState& state, float intensity, float fx, float fz)
{
    auto scale = std::max(state.detail.scale, 1.0f);
    auto value = weather::noise_at(fx / scale, fz / scale, state.detail.seed);

    auto clamped = std::clamp(intensity, 0.0f, 1.0f);
    auto threshold = 1.0f - clamped;

    return utils::smoothstep(threshold, threshold + 0.2f, value);
}

static float detail_coverage(float intensity, float x, float z)
{
    auto& state = weather::state();

    return detail_coverage_in_field(state, intensity, x - state.detail.drift.x(), z - state.detail.drift.y());
}

WeatherState& weather::state(void)
{
    return active_state();
}

void weather::reset(void)
{
    active_state() = WeatherState {};
}

void weather::advance(float delta_time)
{
    if(!gamerules::weather.value()) {
        return;
    }

    auto& state = active_state();

    state.types.drift += state.wind * delta_time;
    state.detail.drift += state.wind * delta_time;
}

bool weather::force_at(weather_type_id type, float x, float z)
{
    auto& state = weather::state();
    auto type_scale = std::max(state.types.scale, 1.0f);

    auto interval = weather_registry::type_interval(allowed_weather(), type);

    if(!interval) {
        return false;
    }

    auto [interval_start, interval_end] = interval.value();

    // Put the roll at the middle of the type's slice,
    // then slide the field so that point lands on the players
    auto target = (interval_start + interval_end) * 0.5f;
    auto base_x = (x - state.types.drift.x()) / type_scale;
    auto base_z = (z - state.types.drift.y()) / type_scale;
    auto cell_x = static_cast<std::int32_t>(std::floor(base_x));
    auto cell_z = static_cast<std::int32_t>(std::floor(base_z));

    auto best_distance = std::numeric_limits<float>::max();
    auto best_x = static_cast<float>(cell_x) + 0.5f;
    auto best_z = static_cast<float>(cell_z) + 0.5f;

    for(std::int32_t dz = -8; dz <= 8; dz += 1) {
        for(std::int32_t dx = -8; dx <= 8; dx += 1) {
            auto sample_x = static_cast<float>(cell_x + dx) + 0.5f;
            auto sample_z = static_cast<float>(cell_z + dz) + 0.5f;
            auto distance = std::abs(weather::noise_at(sample_x, sample_z, state.types.seed) - target);

            if(distance < best_distance) {
                best_distance = distance;
                best_x = sample_x;
                best_z = sample_z;
            }
        }
    }

    state.types.drift = Eigen::Vector2f(x - best_x * type_scale, z - best_z * type_scale);

    auto detail_scale = std::max(state.detail.scale, 1.0f);
    auto detail_base_x = (x - state.detail.drift.x()) / detail_scale;
    auto detail_base_z = (z - state.detail.drift.y()) / detail_scale;
    auto detail_cell_x = static_cast<std::int32_t>(std::floor(detail_base_x));
    auto detail_cell_z = static_cast<std::int32_t>(std::floor(detail_base_z));

    auto best_value = -1.0f;
    auto detail_best_x = detail_cell_x;
    auto detail_best_z = detail_cell_z;

    for(std::int32_t dz = -4; dz <= 4; dz += 1) {
        for(std::int32_t dx = -4; dx <= 4; dx += 1) {
            auto sample_x = static_cast<float>(detail_cell_x + dx) + 0.5f;
            auto sample_z = static_cast<float>(detail_cell_z + dz) + 0.5f;
            auto value = weather::noise_at(sample_x, sample_z, state.detail.seed);

            if(value > best_value) {
                best_value = value;
                detail_best_x = detail_cell_x + dx;
                detail_best_z = detail_cell_z + dz;
            }
        }
    }

    state.detail.drift.x() = x - (static_cast<float>(detail_best_x) + 0.5f) * detail_scale;
    state.detail.drift.y() = z - (static_cast<float>(detail_best_z) + 0.5f) * detail_scale;

    return true;
}

float weather::noise_at(float x, float z, std::uint32_t seed)
{
    auto x0 = static_cast<std::int32_t>(std::floor(x));
    auto z0 = static_cast<std::int32_t>(std::floor(z));
    auto fx = x - static_cast<float>(x0);
    auto fz = z - static_cast<float>(z0);
    auto ux = fx * fx * (3.0f - 2.0f * fx);
    auto uz = fz * fz * (3.0f - 2.0f * fz);

    auto sample = [seed](std::int32_t sx, std::int32_t sz) {
        return static_cast<float>(lattice_hash(sx, sz, seed) & UINT32_C(0xFFFFFF)) / static_cast<float>(UINT32_C(0xFFFFFF));
    };

    auto a = sample(x0, z0);
    auto b = sample(x0 + 1, z0);
    auto c = sample(x0, z0 + 1);
    auto d = sample(x0 + 1, z0 + 1);

    auto top = a * (1.0f - ux) + b * ux;
    auto bottom = c * (1.0f - ux) + d * ux;

    return top * (1.0f - uz) + bottom * uz;
}

float weather::cell_random(std::int32_t x, std::int32_t z, std::uint32_t seed)
{
    return static_cast<float>(lattice_hash(x, z, seed) & UINT32_C(0xFFFFFF)) / static_cast<float>(UINT32_C(0xFFFFFF));
}

weather_type_id weather::type_at(float x, float z)
{
    auto roll = type_noise(x, z);
    auto best = WEATHER_ID_NULL;
    auto best_weight = 0.0f;

    for(const auto& def : weather_registry::all_definitions()) {
        if(def.id == WEATHER_ID_NULL) {
            continue;
        }

        auto weight = interval_weight(def.id, roll);

        if(weight > best_weight) {
            best_weight = weight;
            best = def.id;
        }
    }

    return best;
}

float weather::type_weight(weather_type_id type, float x, float z)
{
    return interval_weight(type, type_noise(x, z));
}

float weather::type_weight_in_field(const WeatherState& state, weather_type_id type, float fx, float fz)
{
    return interval_weight(type, type_noise_in_field(state, fx, fz));
}

float weather::coverage_of(weather_type_id type, float x, float z)
{
    auto def = weather_registry::find_definition(type);

    if(def == nullptr) {
        return 0.0f;
    }

    auto weight = weather::type_weight(type, x, z);

    if(weight <= 0.0f) {
        return 0.0f;
    }

    return weight * detail_coverage(def->intensity, x, z);
}

float weather::coverage_in_field(const WeatherState& state, weather_type_id type, float fx, float fz)
{
    auto def = weather_registry::find_definition(type);

    if(def == nullptr) {
        return 0.0f;
    }

    auto weight = weather::type_weight_in_field(state, type, fx, fz);

    if(weight <= 0.0f) {
        return 0.0f;
    }

    return weight * detail_coverage_in_field(state, def->intensity, fx, fz);
}

float weather::coverage(float x, float z)
{
    return weather::coverage_of(weather::type_at(x, z), x, z);
}

WeatherBlend weather::blend(float x, float z)
{
    WeatherBlend result {};
    result.sky_tint = Eigen::Vector3f::Ones();
    result.sky_visibility = 1.0f;
    result.light_scale = 1.0f;

    auto roll = type_noise(x, z);

    Eigen::Vector3f tint = Eigen::Vector3f::Zero();
    auto visibility = 0.0f;
    auto light = 0.0f;
    auto coverage = 0.0f;

    for(const auto& def : weather_registry::all_definitions()) {
        if(def.id == WEATHER_ID_NULL) {
            continue;
        }

        auto weight = interval_weight(def.id, roll);

        if(weight <= 0.0f) {
            continue;
        }

        auto amount = weight * detail_coverage(def.intensity, x, z);

        tint += def.sky_tint * amount;
        visibility += def.sky_visibility * amount;
        light += def.light_scale * amount;
        coverage += amount;
    }

    if(coverage <= 0.0f) {
        return result;
    }

    tint /= coverage;
    visibility /= coverage;
    light /= coverage;
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

WeatherBlend weather::blend_overhead(float x, float y, float z)
{
    (void)y;

    return weather::blend(x, z);
}

WeatherBlend weather::blend_region(float x, float y, float z, float radius)
{
    (void)y;
    (void)radius;

    return weather::blend(x, z);
}

void weather::fill_info(packet::World_Info& packet)
{
    auto& state = active_state();

    packet.types_seed = state.types.seed;
    packet.types_scale = state.types.scale;
    packet.detail_seed = state.detail.seed;
    packet.detail_scale = state.detail.scale;
}

void weather::apply_info(const packet::World_Info& packet)
{
    auto& state = active_state();

    state.types.seed = packet.types_seed;
    state.types.scale = packet.types_scale;
    state.detail.seed = packet.detail_seed;
    state.detail.scale = packet.detail_scale;
}

void weather::fill_packet(packet::World_Weather& packet)
{
    auto& state = active_state();

    packet.types_drift = state.types.drift;
    packet.detail_drift = state.detail.drift;
    packet.wind = state.wind;
}

void weather::apply_packet(const packet::World_Weather& packet, bool baseline)
{
    // Both sides advance the drift between snapshots, so a snapshot
    // that lands a little behind the prediction is just latency. Taking
    // it as it is would twitch the whole field once a second; only
    // a real jump, a forced storm for example, is worth snapping to.
    // The first snapshot of a session is always taken whole
    constexpr static float DRIFT_SNAP = 64.0f;

    auto& target = active_state();

    if(baseline || (packet.types_drift - target.types.drift).norm() > DRIFT_SNAP) {
        target.types.drift = packet.types_drift;
    }

    if(baseline || (packet.detail_drift - target.detail.drift).norm() > DRIFT_SNAP) {
        target.detail.drift = packet.detail_drift;
    }

    target.wind = packet.wind;
}
