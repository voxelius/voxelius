#include "shared/pch.hh"

#include "shared/world/world_clock.hh"

#include "core/utils/interpolation.hh"

constexpr static float NIGHT_DAYLIGHT = 0.12f;
constexpr static float DAY_DAYLIGHT = 1.0f;

constexpr static float DAWN_EDGE = -0.12f;
constexpr static float DAY_EDGE = 0.10f;

// FIXME: move this to core as a utility function
// if it appears in the code for the third time, currently at two...
float world_clock::phase(std::uint64_t daylight_tick, std::uint32_t day_length)
{
    auto length = static_cast<std::uint64_t>(std::max<std::uint32_t>(day_length, 1));

    return static_cast<float>(daylight_tick % length) / static_cast<float>(length);
}

std::uint64_t world_clock::day(std::uint64_t daylight_tick, std::uint32_t day_length)
{
    auto length = static_cast<std::uint64_t>(std::max<std::uint32_t>(day_length, 1));

    return daylight_tick / length;
}

float world_clock::daylight(float phase)
{
    auto height = std::sin(phase * std::numbers::pi_v<float> * 2.0f);

    return NIGHT_DAYLIGHT + (DAY_DAYLIGHT - NIGHT_DAYLIGHT) * utils::smoothstep(DAWN_EDGE, DAY_EDGE, height);
}

Eigen::Vector3f world_clock::sun_direction(float phase)
{
    auto angle = phase * std::numbers::pi_v<float> * 2.0f;

    // Phase 0 is the east horizon and phase 0.5 the west one; the
    // client may tilt this circle, but by default it passes the zenith
    Eigen::Vector3f direction;
    direction.x() = std::cos(angle);
    direction.y() = std::sin(angle);
    direction.z() = 0.0f;

    return direction;
}
