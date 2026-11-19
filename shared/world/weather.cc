#include "shared/pch.hh"

#include "shared/world/weather.hh"

#include "core/utils/interpolation.hh"

#include "shared/cvar/gamerules.hh"
#include "shared/net/packet_world.hh"
#include "shared/utils/biome.hh"
#include "shared/world/weather_registry.hh"

static std::array<WeatherRealmState, NUM_BIOME_REALMS> s_states {};

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
static float type_noise_in_field(const WeatherRealmState& state, float fx, float fz)
{
    auto scale = std::max(state.types.scale, 1.0f);

    return weather::noise_at(fx / scale, fz / scale, state.types.seed);
}

static float type_noise(biome_realm realm, float x, float z)
{
    auto& state = weather::state(realm);

    return type_noise_in_field(state, x - state.types.drift.x(), z - state.types.drift.y());
}

static float interval_weight(biome_realm realm, weather_type_id type, float roll)
{
    // The band is what turns a hard slice boundary into a gradual fade
    constexpr static float BAND = 0.05f;

    float start = 0.0f;
    float end = 0.0f;

    if(!weather_registry::type_interval(realm, type, start, end)) {
        return 0.0f;
    }

    auto fade_in = utils::smoothstep(start - BAND, start + BAND, roll);
    auto fade_out = 1.0f - utils::smoothstep(end - BAND, end + BAND, roll);

    return fade_in * fade_out;
}

static float detail_coverage_in_field(const WeatherRealmState& state, float intensity, float fx, float fz)
{
    auto scale = std::max(state.detail.scale, 1.0f);
    auto value = weather::noise_at(fx / scale, fz / scale, state.detail.seed);

    auto clamped = std::clamp(intensity, 0.0f, 1.0f);
    auto threshold = 1.0f - clamped;

    return utils::smoothstep(threshold, threshold + 0.2f, value);
}

static float detail_coverage(biome_realm realm, float intensity, float x, float z)
{
    auto& state = weather::state(realm);

    return detail_coverage_in_field(state, intensity, x - state.detail.drift.x(), z - state.detail.drift.y());
}

static WeatherBlend blend_at_height(float x, float y, float z)
{
    // Realms are stacked vertically; the fade band hides
    // the seam where one stops providing the ambient tint
    constexpr static float FADE = 24.0f;

    struct Boundary final {
        float y;
        biome_realm below;
        biome_realm above;
    };

    constexpr static std::array BOUNDARIES = {
        Boundary { -256.0f, BIOME_REALM_THE_DEPTHS, BIOME_REALM_UNDERGROUND },
        Boundary { -64.0f, BIOME_REALM_UNDERGROUND, BIOME_REALM_SURFACE },
        Boundary { 256.0f, BIOME_REALM_SURFACE, BIOME_REALM_SKY },
    };

    for(const auto& boundary : BOUNDARIES) {
        auto distance = y - boundary.y;

        if(distance <= -FADE || distance >= FADE) {
            continue;
        }

        auto t = std::clamp(0.5f + distance / (2.0f * FADE), 0.0f, 1.0f);
        auto below = weather::blend(boundary.below, x, z);
        auto above = weather::blend(boundary.above, x, z);

        WeatherBlend result {};
        result.coverage = below.coverage * (1.0f - t) + above.coverage * t;
        result.sky_tint = below.sky_tint * (1.0f - t) + above.sky_tint * t;
        result.sky_visibility = below.sky_visibility * (1.0f - t) + above.sky_visibility * t;
        result.light_scale = below.light_scale * (1.0f - t) + above.light_scale * t;
        return result;
    }

    return weather::blend(utils::realm_of_y(y), x, z);
}

std::span<WeatherRealmState, NUM_BIOME_REALMS> weather::states(void)
{
    return s_states;
}

WeatherRealmState& weather::state(biome_realm realm)
{
    static WeatherRealmState fallback {};

    if(realm >= BIOME_REALM_COUNT) {
        return fallback;
    }

    return s_states.at(static_cast<std::size_t>(realm));
}

void weather::reset(void)
{
    for(auto& state : s_states) {
        state = WeatherRealmState {};
    }
}

void weather::advance(float delta_time)
{
    if(!gamerules::weather.value()) {
        return;
    }

    for(auto& state : s_states) {
        state.types.drift += state.wind * delta_time;
        state.detail.drift += state.wind * delta_time;
    }
}

void weather::force_at(biome_realm realm, weather_type_id type, float x, float z)
{
    if(realm >= BIOME_REALM_COUNT) {
        return;
    }

    auto& state = weather::state(realm);
    auto type_scale = std::max(state.types.scale, 1.0f);

    float interval_start = 0.0f;
    float interval_end = 0.0f;

    if(!weather_registry::type_interval(realm, type, interval_start, interval_end)) {
        return;
    }

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

    state.detail.drift = Eigen::Vector2f(x - (static_cast<float>(detail_best_x) + 0.5f) * detail_scale,
        z - (static_cast<float>(detail_best_z) + 0.5f) * detail_scale);
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

weather_type_id weather::type_at(biome_realm realm, float x, float z)
{
    if(realm >= BIOME_REALM_COUNT) {
        return WEATHER_ID_NULL;
    }

    auto roll = type_noise(realm, x, z);
    auto best = WEATHER_ID_NULL;
    auto best_weight = 0.0f;

    for(const auto& def : weather_registry::all_definitions()) {
        if(def.id == WEATHER_ID_NULL) {
            continue;
        }

        auto weight = interval_weight(realm, def.id, roll);

        if(weight > best_weight) {
            best_weight = weight;
            best = def.id;
        }
    }

    return best;
}

float weather::type_weight(biome_realm realm, weather_type_id type, float x, float z)
{
    if(realm >= BIOME_REALM_COUNT) {
        return 0.0f;
    }

    return interval_weight(realm, type, type_noise(realm, x, z));
}

float weather::type_weight_in_field(const WeatherRealmState& state, biome_realm realm, weather_type_id type, float fx, float fz)
{
    if(realm >= BIOME_REALM_COUNT) {
        return 0.0f;
    }

    return interval_weight(realm, type, type_noise_in_field(state, fx, fz));
}

float weather::coverage_of(biome_realm realm, weather_type_id type, float x, float z)
{
    auto def = weather_registry::find_definition(type);

    if(def == nullptr) {
        return 0.0f;
    }

    auto weight = weather::type_weight(realm, type, x, z);

    if(weight <= 0.0f) {
        return 0.0f;
    }

    return weight * detail_coverage(realm, def->intensity, x, z);
}

float weather::coverage_in_field(const WeatherRealmState& state, biome_realm realm, weather_type_id type, float fx, float fz)
{
    auto def = weather_registry::find_definition(type);

    if(def == nullptr) {
        return 0.0f;
    }

    auto weight = weather::type_weight_in_field(state, realm, type, fx, fz);

    if(weight <= 0.0f) {
        return 0.0f;
    }

    return weight * detail_coverage_in_field(state, def->intensity, fx, fz);
}

float weather::coverage(biome_realm realm, float x, float z)
{
    return weather::coverage_of(realm, weather::type_at(realm, x, z), x, z);
}

WeatherBlend weather::blend(biome_realm realm, float x, float z)
{
    WeatherBlend result {};
    result.sky_tint = Eigen::Vector3f::Ones();
    result.sky_visibility = 1.0f;
    result.light_scale = 1.0f;

    if(realm >= BIOME_REALM_COUNT) {
        return result;
    }

    auto roll = type_noise(realm, x, z);

    Eigen::Vector3f tint = Eigen::Vector3f::Zero();
    auto visibility = 0.0f;
    auto light = 0.0f;
    auto coverage = 0.0f;

    for(const auto& def : weather_registry::all_definitions()) {
        if(def.id == WEATHER_ID_NULL) {
            continue;
        }

        auto weight = interval_weight(realm, def.id, roll);

        if(weight <= 0.0f) {
            continue;
        }

        auto amount = weight * detail_coverage(realm, def.intensity, x, z);

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
    auto result = blend_at_height(x, y, z);
    auto camera_realm = utils::realm_of_y(y);

    for(std::size_t i = 0; i < NUM_BIOME_REALMS; ++i) {
        auto realm = static_cast<biome_realm>(i);

        if(realm == camera_realm) {
            continue;
        }

        auto type = weather_registry::find_definition(weather::type_at(realm, x, z));

        if(type == nullptr) {
            continue;
        }

        if(type->cloud.coverage <= 0.02f || type->cloud.height <= y + 4.0f) {
            continue;
        }

        auto candidate = weather::blend(realm, x, z);

        if(candidate.coverage > result.coverage) {
            result = candidate;
        }
    }

    return result;
}

WeatherBlend weather::blend_region(float x, float y, float z, float radius)
{
    auto base = weather::blend_overhead(x, y, z);

    // The coverage this function reports is the strongest
    // cell in the region, so the tint has to come from that same
    // cell. A plain average lets a barely touched corner speak
    // for a cell that is fully under the front, and the
    // sky takes the colour of weather that is almost not there
    constexpr static int STEPS = 5;

    Eigen::Vector3f tint = Eigen::Vector3f::Zero();
    auto visibility = 0.0f;
    auto light = 0.0f;
    auto coverage = 0.0f;
    auto total = 0.0f;
    auto realm = utils::realm_of_y(y);

    for(int iz = 0; iz < STEPS; iz += 1) {
        for(int ix = 0; ix < STEPS; ix += 1) {
            auto px = x + (static_cast<float>(ix) / static_cast<float>(STEPS - 1) - 0.5f) * 2.0f * radius;
            auto pz = z + (static_cast<float>(iz) / static_cast<float>(STEPS - 1) - 0.5f) * 2.0f * radius;

            auto sample = weather::blend(realm, px, pz);
            auto amount = std::clamp(sample.coverage, 0.0f, 1.0f);
            auto weight = amount * amount * amount * amount;

            tint += sample.sky_tint * weight;
            visibility += sample.sky_visibility * weight;
            light += sample.light_scale * weight;
            total += weight;
            coverage = std::max(coverage, amount);
        }
    }

    WeatherBlend result {};
    result.coverage = coverage;
    result.sky_tint = Eigen::Vector3f::Ones();
    result.sky_visibility = 1.0f;
    result.light_scale = 1.0f;

    if(total > 0.0f) {
        result.sky_tint = tint / total;
        result.sky_visibility = std::clamp(visibility / total, 0.0f, 1.0f);
        result.light_scale = light / total;
    }

    if(base.coverage > coverage) {
        result.coverage = base.coverage;
        result.sky_tint = base.sky_tint;
        result.sky_visibility = base.sky_visibility;
        result.light_scale = base.light_scale;
    }

    return result;
}

void weather::fill_info(packet::World_Info& packet)
{
    for(std::size_t i = 0; i < NUM_BIOME_REALMS; ++i) {
        packet.types_seed[i] = s_states[i].types.seed;
        packet.types_scale[i] = s_states[i].types.scale;
        packet.detail_seed[i] = s_states[i].detail.seed;
        packet.detail_scale[i] = s_states[i].detail.scale;
    }
}

void weather::apply_info(const packet::World_Info& packet)
{
    for(std::size_t i = 0; i < NUM_BIOME_REALMS; ++i) {
        s_states[i].types.seed = packet.types_seed[i];
        s_states[i].types.scale = packet.types_scale[i];
        s_states[i].detail.seed = packet.detail_seed[i];
        s_states[i].detail.scale = packet.detail_scale[i];
    }
}

void weather::fill_packet(packet::World_Weather& packet)
{
    for(std::size_t i = 0; i < NUM_BIOME_REALMS; ++i) {
        packet.types_drift[i] = s_states[i].types.drift;
        packet.detail_drift[i] = s_states[i].detail.drift;
        packet.wind[i] = s_states[i].wind;
    }
}

void weather::apply_packet(const packet::World_Weather& packet, bool baseline)
{
    // Both sides advance the drift between snapshots, so a snapshot
    // that lands a little behind the prediction is just latency. Taking
    // it as it is would twitch the whole field once a second; only
    // a real jump, a forced storm for example, is worth snapping to.
    // The first snapshot of a session is always taken whole
    constexpr static float DRIFT_SNAP = 64.0f;

    for(std::size_t i = 0; i < NUM_BIOME_REALMS; ++i) {
        auto& target = s_states[i];

        if(baseline || (packet.types_drift[i] - target.types.drift).norm() > DRIFT_SNAP) {
            target.types.drift = packet.types_drift[i];
        }

        if(baseline || (packet.detail_drift[i] - target.detail.drift).norm() > DRIFT_SNAP) {
            target.detail.drift = packet.detail_drift[i];
        }

        target.wind = packet.wind[i];
    }
}
