#include "client/pch.hh"

#include "client/world/skybox.hh"

#include "core/cvar/host.hh"
#include "core/cvar/number.hh"
#include "core/utils/interpolation.hh"

#include "shared/constant.hh"
#include "shared/cvar/gamerules.hh"
#include "shared/cvar/server.hh"
#include "shared/globals.hh"
#include "shared/utils/coord.hh"
#include "shared/world/weather.hh"
#include "shared/world/world_clock.hh"

#include "client/camera.hh"
#include "client/globals.hh"
#include "client/gui/container.hh"
#include "client/gui/slider.hh"
#include "client/settings.hh"

constexpr static float SMOOTH_RATE = 3.0f;
constexpr static float MAX_STEP_TIME = 0.1f;
constexpr static float WEATHER_RADIUS = 256.0f;

// The phase runs on the client's own clock and only corrects
// towards the server, so a packet that arrives late or in a
// burst cannot step it. A quarter-day or more is a real jump
constexpr static float PHASE_CORRECTION_RATE = 1.0f;
constexpr static float PHASE_SNAP_THRESHOLD = 0.25f;

constexpr static float TWILIGHT_HEIGHT = 0.35f;
constexpr static std::uint32_t MOON_PHASE_COUNT = 8;

// A cheap Rayleigh model: the air mass the sunlight crosses grows as
// the sun sinks, and blue light is scattered away first, so the light
// that reaches the eye at sunset is orange and then red. The constants
// are tuned for looks, not for a physical atmosphere
constexpr static float AIRMASS_FLOOR = 0.15f;
constexpr static float RAYLEIGH_RED = 0.04f;
constexpr static float RAYLEIGH_GREEN = 0.18f;
constexpr static float RAYLEIGH_BLUE = 0.55f;

static const Eigen::Vector3f DAY_HORIZON { 0.64f, 0.76f, 0.94f };
static const Eigen::Vector3f DAY_ZENITH { 0.26f, 0.46f, 0.85f };
static const Eigen::Vector3f NIGHT_HORIZON { 0.020f, 0.030f, 0.060f };
static const Eigen::Vector3f NIGHT_ZENITH { 0.004f, 0.007f, 0.018f };
static const Eigen::Vector3f MOON_COLOR { 0.86f, 0.90f, 1.0f };

Eigen::Vector3f skybox::horizon_color;
Eigen::Vector3f skybox::sun_horizon_color;
Eigen::Vector3f skybox::fog_color;
Eigen::Vector3f skybox::zenith_color;

Eigen::Vector3f skybox::sun_direction;
Eigen::Vector3f skybox::sun_tint;
Eigen::Vector3f skybox::cloud_tint { Eigen::Vector3f::Ones() };
float skybox::sun_strength;

Eigen::Vector3f skybox::moon_direction;
Eigen::Vector3f skybox::moon_tint;
float skybox::moon_strength;
std::uint32_t skybox::moon_phase;

float skybox::sun_tilt;
float skybox::twilight;

float skybox::phase;
float skybox::daylight;
float skybox::light_scale { 1.0f };
float skybox::star_visibility;
std::uint32_t skybox::star_seed;

static cvar::Number<float> s_sun_angle { 0.0f, -90.0f, 90.0f };
static gui::SliderFloat s_sun_angle_gui;

static float s_phase;
static Eigen::Vector3f s_horizon;
static Eigen::Vector3f s_warm_horizon;
static Eigen::Vector3f s_zenith;
static float s_light { 1.0f };
static bool s_primed;
static bool s_snap;

// The shortest way from one phase to another, so the sun
// never spins backwards when the day counter wraps
static float wrapped_delta(float from, float to)
{
    auto delta = to - from;

    if(delta > 0.5f) {
        return delta - 1.0f;
    }

    if(delta < -0.5f) {
        return delta + 1.0f;
    }

    return delta;
}

// Rotating the sun's circle about the east-west axis keeps both
// ends on the horizon, so zero tilt still rises due east
static Eigen::Vector3f tilt_about_x(const Eigen::Vector3f& direction, float tilt)
{
    auto c = std::cos(tilt);
    auto s = std::sin(tilt);

    Eigen::Vector3f result;
    result.x() = direction.x();
    result.y() = direction.y() * c - direction.z() * s;
    result.z() = direction.y() * s + direction.z() * c;

    return result;
}

void skybox::init(void)
{
    auto& sun_angle_binding = cvar::bind_var(s_sun_angle, "r_sun_angle");
    sun_angle_binding.set_help("Tilt of the sun's path in degrees; 0 rises due east and sets due west");
    sun_angle_binding.set_flags(FCVAR_ARCHIVE);

    s_sun_angle_gui.set_range(-90.0f, 90.0f);
    s_sun_angle_gui.set_format("%.0f");
    s_sun_angle_gui.bind("r_sun_angle");
    settings::video_atmosphere.add_child(s_sun_angle_gui, 2);

    reset();

    s_phase = 0.25f;
    s_horizon = DAY_HORIZON;
    s_zenith = DAY_ZENITH;

    horizon_color = DAY_HORIZON;
    zenith_color = DAY_ZENITH;
    daylight = 1.0f;
    light_scale = 1.0f;
}

void skybox::update(void)
{
    auto day_length = server_vars::day_length_ticks();
    auto target_phase = world_clock::phase(globals::daylight_tick, day_length);
    auto snap_state = !s_primed || s_snap;

    if(snap_state) {
        s_phase = target_phase;
        s_snap = false;
    }
    else {
        auto delta_time = std::min(globals::window_frametime, MAX_STEP_TIME);

        // Advance at the fixed tickrate on our own, so the phase
        // keeps moving even between two network ticks. A stopped
        // day cycle stops the local clock too, and only correction runs
        if(gamerules::daylight_cycle.value()) {
            auto day_seconds = static_cast<float>(day_length) / static_cast<float>(constant::FIXED_TICKRATE);

            if(day_seconds > 0.0f) {
                s_phase += delta_time / day_seconds;
            }
        }

        auto correction = wrapped_delta(s_phase, target_phase);

        if(std::abs(correction) > PHASE_SNAP_THRESHOLD) {
            s_phase = target_phase;
        }
        else {
            auto step = 1.0f - std::exp(-PHASE_CORRECTION_RATE * delta_time);

            s_phase += correction * step;
        }

        s_phase -= std::floor(s_phase);
    }

    auto height = std::sin(s_phase * std::numbers::pi_v<float> * 2.0f);
    auto light = world_clock::daylight(s_phase);
    auto tilt = s_sun_angle.value() * std::numbers::pi_v<float> / 180.0f;

    auto sun_vector = tilt_about_x(world_clock::sun_direction(s_phase), tilt);
    auto moon_vector = tilt_about_x(world_clock::sun_direction(s_phase + 0.5f), tilt);

    Eigen::Vector3f world = utils::to_fvec(camera::chunk) + camera::local;
    auto blend = weather::blend_region(world.x(), world.y(), world.z(), WEATHER_RADIUS);
    auto visibility = std::clamp(blend.sky_visibility, 0.0f, 1.0f);

    // The sunlight crosses more air as the sun sinks, losing blue
    // first; the normalised result is the colour of the setting sun
    auto airmass = 1.0f / (std::max(height, 0.0f) + AIRMASS_FLOOR);

    Eigen::Vector3f transmittance;
    transmittance.x() = std::exp(-RAYLEIGH_RED * airmass);
    transmittance.y() = std::exp(-RAYLEIGH_GREEN * airmass);
    transmittance.z() = std::exp(-RAYLEIGH_BLUE * airmass);

    Eigen::Vector3f tint = transmittance / transmittance.maxCoeff();
    auto twilight = std::clamp(1.0f - std::abs(height) / TWILIGHT_HEIGHT, 0.0f, 1.0f);

    Eigen::Vector3f base_horizon = NIGHT_HORIZON + (DAY_HORIZON - NIGHT_HORIZON) * light;
    Eigen::Vector3f base_zenith = NIGHT_ZENITH + (DAY_ZENITH - NIGHT_ZENITH) * light;

    // The clear horizon is neutral away from the sun and warms towards
    // the reddened sunlight on the sun's side; the zenith stays blue
    Eigen::Vector3f warm_horizon = tint * (0.65f + 0.35f * light);
    Eigen::Vector3f clear_horizon = base_horizon;
    Eigen::Vector3f clear_zenith = base_zenith;

    Eigen::Vector3f weather_horizon = blend.sky_tint * light;
    Eigen::Vector3f weather_zenith = blend.sky_tint * (0.55f * light);

    // A type with sky_visibility 0 replaces the sky with its own
    // colour; 1 leaves the Rayleigh gradient fully in charge
    Eigen::Vector3f target_horizon = weather_horizon * (1.0f - visibility) + clear_horizon * visibility;
    Eigen::Vector3f target_warm = weather_horizon * (1.0f - visibility) + warm_horizon * visibility;
    Eigen::Vector3f target_zenith = weather_zenith * (1.0f - visibility) + clear_zenith * visibility;

    if(snap_state) {
        s_horizon = target_horizon;
        s_warm_horizon = target_warm;
        s_zenith = target_zenith;
        s_light = blend.light_scale;
    }
    else {
        auto step = 1.0f - std::exp(-SMOOTH_RATE * std::min(globals::window_frametime, MAX_STEP_TIME));

        s_horizon += (target_horizon - s_horizon) * step;
        s_warm_horizon += (target_warm - s_warm_horizon) * step;
        s_zenith += (target_zenith - s_zenith) * step;
        s_light += (blend.light_scale - s_light) * step;
    }

    s_primed = true;

    // The fog takes the horizon colour in the direction the camera
    // faces, so turning towards the sunset warms the haze as well
    auto camera_flat = Eigen::Vector2f(camera::forward.x(), camera::forward.z());
    auto sun_flat = Eigen::Vector2f(sun_vector.x(), sun_vector.z());
    auto camera_length = camera_flat.norm();
    auto sun_length = sun_flat.norm();
    auto alignment = 0.0f;

    if(camera_length > 1.0e-4f && sun_length > 1.0e-4f) {
        alignment = camera_flat.dot(sun_flat) / (camera_length * sun_length);
    }

    auto warmth = std::pow(std::max(alignment, 0.0f), 2.0f) * twilight * visibility;

    skybox::horizon_color = s_horizon;
    skybox::sun_horizon_color = s_warm_horizon;
    skybox::fog_color = s_horizon + (s_warm_horizon - s_horizon) * warmth;
    skybox::zenith_color = s_zenith;
    skybox::phase = s_phase;
    skybox::daylight = light;
    skybox::light_scale = s_light;

    skybox::sun_tilt = tilt;
    skybox::sun_tint = tint;
    skybox::sun_direction = sun_vector;
    skybox::sun_strength = utils::smoothstep(-0.05f, 0.05f, height) * visibility;

    skybox::moon_direction = moon_vector;
    skybox::moon_tint = MOON_COLOR + (tint - MOON_COLOR) * (twilight * 0.85f);
    skybox::moon_strength = utils::smoothstep(-0.05f, 0.05f, -height) * visibility * 0.85f;
    skybox::moon_phase = static_cast<std::uint32_t>(globals::day % MOON_PHASE_COUNT);

    skybox::cloud_tint = Eigen::Vector3f::Ones() + (tint - Eigen::Vector3f::Ones()) * (twilight * visibility * 0.85f);
    skybox::twilight = twilight * visibility;
    skybox::star_visibility = (1.0f - light) * visibility;
}

void skybox::reset(void)
{
    s_primed = false;
    s_snap = false;
    star_seed = 0;
}

void skybox::snap(void)
{
    s_snap = true;
}

void skybox::set_star_seed(std::uint32_t seed)
{
    star_seed = seed;
}
