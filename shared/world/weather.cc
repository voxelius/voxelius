#include "shared/pch.hh"

#include "shared/world/weather.hh"

#include "core/utils/interpolation.hh"

#include "shared/cvar/gamerules.hh"
#include "shared/net/packet_world.hh"
#include "shared/utils/weather.hh"
#include "shared/world/weather_registry.hh"

static float interval_weight(std::span<const weather_type_id> allowed, weather_type_id type, float roll)
{
    // The band is what turns a hard slice boundary into a gradual fade
    constexpr static float BAND = 0.05f;

    if(auto interval = weather_registry::type_interval(allowed, type)) {
        auto [start, end] = interval.value();

        auto fade_in = utils::smoothstep(start - BAND, start + BAND, roll);
        auto fade_out = 1.0f - utils::smoothstep(end - BAND, end + BAND, roll);

        return fade_in * fade_out;
    }

    return 0.0f;
}

void WeatherState::reset(void)
{
    types = {};
    detail = {};
    wind = Eigen::Vector2f::Zero();
    wind_direction = Eigen::Vector2f::Zero();
    strike_budget = 0.0f;
}

void WeatherState::advance(float delta_time)
{
    if(!gamerules::weather.value()) {
        return;
    }

    types.drift += wind * delta_time;
    detail.drift += wind * delta_time;
}

bool WeatherState::force_at(std::span<const weather_type_id> allowed, weather_type_id type, float x, float z)
{
    auto type_scale = std::max(types.scale, 1.0f);

    auto interval = weather_registry::type_interval(allowed, type);

    if(!interval.has_value()) {
        return false;
    }

    auto [interval_start, interval_end] = interval.value();

    // Put the roll at the middle of the type's slice,
    // then slide the field so that point lands on the players
    auto target = (interval_start + interval_end) * 0.5f;
    auto base_x = (x - types.drift.x()) / type_scale;
    auto base_z = (z - types.drift.y()) / type_scale;
    auto cell_x = static_cast<std::int32_t>(std::floor(base_x));
    auto cell_z = static_cast<std::int32_t>(std::floor(base_z));

    auto best_distance = std::numeric_limits<float>::max();
    auto best_x = static_cast<float>(cell_x) + 0.5f;
    auto best_z = static_cast<float>(cell_z) + 0.5f;

    for(std::int32_t dz = -8; dz <= 8; dz += 1) {
        for(std::int32_t dx = -8; dx <= 8; dx += 1) {
            auto sample_x = static_cast<float>(cell_x + dx) + 0.5f;
            auto sample_z = static_cast<float>(cell_z + dz) + 0.5f;
            auto distance = std::abs(utils::weather_noise(sample_x, sample_z, types.seed) - target);

            if(distance < best_distance) {
                best_distance = distance;
                best_x = sample_x;
                best_z = sample_z;
            }
        }
    }

    types.drift = Eigen::Vector2f(x - best_x * type_scale, z - best_z * type_scale);

    auto detail_scale = std::max(detail.scale, 1.0f);
    auto detail_base_x = (x - detail.drift.x()) / detail_scale;
    auto detail_base_z = (z - detail.drift.y()) / detail_scale;
    auto detail_cell_x = static_cast<std::int32_t>(std::floor(detail_base_x));
    auto detail_cell_z = static_cast<std::int32_t>(std::floor(detail_base_z));

    auto best_value = -1.0f;
    auto detail_best_x = detail_cell_x;
    auto detail_best_z = detail_cell_z;

    for(std::int32_t dz = -4; dz <= 4; dz += 1) {
        for(std::int32_t dx = -4; dx <= 4; dx += 1) {
            auto sample_x = static_cast<float>(detail_cell_x + dx) + 0.5f;
            auto sample_z = static_cast<float>(detail_cell_z + dz) + 0.5f;
            auto value = utils::weather_noise(sample_x, sample_z, detail.seed);

            if(value > best_value) {
                best_value = value;
                detail_best_x = detail_cell_x + dx;
                detail_best_z = detail_cell_z + dz;
            }
        }
    }

    detail.drift.x() = x - (static_cast<float>(detail_best_x) + 0.5f) * detail_scale;
    detail.drift.y() = z - (static_cast<float>(detail_best_z) + 0.5f) * detail_scale;

    return true;
}

weather_type_id WeatherState::type_at(std::span<const weather_type_id> allowed, float x, float z) const
{
    auto roll = type_noise(x, z);
    auto best = WEATHER_ID_NULL;
    auto best_weight = 0.0f;

    for(const auto& def : weather_registry::all_definitions()) {
        if(def.id == WEATHER_ID_NULL) {
            continue;
        }

        auto weight = interval_weight(allowed, def.id, roll);

        if(weight > best_weight) {
            best_weight = weight;
            best = def.id;
        }
    }

    return best;
}

float WeatherState::type_weight(std::span<const weather_type_id> allowed, weather_type_id type, float x, float z) const
{
    return interval_weight(allowed, type, type_noise(x, z));
}

float WeatherState::type_weight_in_field(std::span<const weather_type_id> allowed, weather_type_id type, float fx, float fz) const
{
    return interval_weight(allowed, type, type_noise_in_field(fx, fz));
}

float WeatherState::coverage_of(std::span<const weather_type_id> allowed, weather_type_id type, float x, float z) const
{
    auto def = weather_registry::find_definition(type);

    if(def == nullptr) {
        return 0.0f;
    }

    auto weight = type_weight(allowed, type, x, z);

    if(weight <= 0.0f) {
        return 0.0f;
    }

    return weight * detail_coverage(def->intensity, x, z);
}

float WeatherState::coverage_in_field(std::span<const weather_type_id> allowed, weather_type_id type, float fx, float fz) const
{
    auto def = weather_registry::find_definition(type);

    if(def == nullptr) {
        return 0.0f;
    }

    auto weight = type_weight_in_field(allowed, type, fx, fz);

    if(weight <= 0.0f) {
        return 0.0f;
    }

    return weight * detail_coverage_in_field(def->intensity, fx, fz);
}

float WeatherState::coverage(std::span<const weather_type_id> allowed, float x, float z) const
{
    return coverage_of(allowed, type_at(allowed, x, z), x, z);
}

WeatherBlend WeatherState::blend(std::span<const weather_type_id> allowed, float x, float z) const
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

        auto weight = interval_weight(allowed, def.id, roll);

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

WeatherBlend WeatherState::blend_overhead(std::span<const weather_type_id> allowed, float x, float, float z) const
{
    return blend(allowed, x, z);
}

WeatherBlend WeatherState::blend_region(std::span<const weather_type_id> allowed, float x, float, float z, float) const
{
    return blend(allowed, x, z);
}

void WeatherState::fill_info(packet::World_Info& packet) const
{
    packet.types_seed = types.seed;
    packet.types_scale = types.scale;
    packet.detail_seed = detail.seed;
    packet.detail_scale = detail.scale;
}

void WeatherState::apply_info(const packet::World_Info& packet)
{
    types.seed = packet.types_seed;
    types.scale = packet.types_scale;
    detail.seed = packet.detail_seed;
    detail.scale = packet.detail_scale;
}

void WeatherState::fill_packet(packet::World_Weather& packet) const
{
    packet.types_drift = types.drift;
    packet.detail_drift = detail.drift;
    packet.wind = wind;
}

void WeatherState::apply_packet(const packet::World_Weather& packet, bool baseline)
{
    // Both sides advance the drift between snapshots, so a snapshot
    // that lands a little behind the prediction is just latency. Taking
    // it as it is would twitch the whole field once a second; only
    // a real jump, a forced storm for example, is worth snapping to.
    // The first snapshot of a session is always taken whole
    constexpr static float DRIFT_SNAP = 64.0f;

    if(baseline || (packet.types_drift - types.drift).norm() > DRIFT_SNAP) {
        types.drift = packet.types_drift;
    }

    if(baseline || (packet.detail_drift - detail.drift).norm() > DRIFT_SNAP) {
        detail.drift = packet.detail_drift;
    }

    wind = packet.wind;
}

float WeatherState::type_noise(float x, float z) const
{
    return type_noise_in_field(x - types.drift.x(), z - types.drift.y());
}

// The field-space helpers take coordinates that already have the drift
// removed. A caller that walks the field cell by cell (the cloud renderer)
// uses them so that its sample points do not depend on how far the field has
// drifted: subtracting two large world-space floats rounds differently
// every time
float WeatherState::type_noise_in_field(float fx, float fz) const
{
    auto scale = std::max(types.scale, 1.0f);

    return utils::weather_noise(fx / scale, fz / scale, types.seed);
}

float WeatherState::detail_coverage(float intensity, float x, float z) const
{
    return detail_coverage_in_field(intensity, x - detail.drift.x(), z - detail.drift.y());
}

float WeatherState::detail_coverage_in_field(float intensity, float fx, float fz) const
{
    auto scale = std::max(detail.scale, 1.0f);
    auto value = utils::weather_noise(fx / scale, fz / scale, detail.seed);

    auto clamped = std::clamp(intensity, 0.0f, 1.0f);
    auto threshold = 1.0f - clamped;

    return utils::smoothstep(threshold, threshold + 0.2f, value);
}
