#include "client/pch.hh"

#include "client/fog.hh"

#include "core/config/ref.hh"
#include "core/cvar/host.hh"

#include "shared/utils/coord.hh"
#include "shared/world/fluid_registry.hh"
#include "shared/world/weather.hh"

#include "client/camera.hh"
#include "client/constant.hh"
#include "client/globals.hh"
#include "client/gui/slider.hh"
#include "client/world/skybox.hh"

constexpr static float SMOOTH_RATE = 3.0f;
constexpr static float MAX_STEP_TIME = 0.1f;

float fog::distance;
Eigen::Vector3f fog::color;
float fog::light_scale { 1.0f };

cvar::Number<float> fog::daylight { 1.0f, 0.0f, 1.0f };

static Eigen::Vector3f s_color;
static float s_light { 1.0f };
static bool s_primed { false };

void fog::init(void)
{
    auto& daylight_binding = cvar::bind_var(daylight, "r_daylight");
    daylight_binding.set_help("Brightness of the daylight: 1 is noon, lower is dusk");
    daylight_binding.set_flags(FCVAR_ARCHIVE);
}

void fog::update(void)
{
    distance = static_cast<float>(constant::CHUNK_SIZE * camera::view_distance.value());
    auto target_color = skybox::sky_color;
    auto target_light = 1.0f;

    // The storm is only visible where the front is, so take
    // the tint at the camera rather than from the realm as a whole
    auto world = utils::to_fvec(camera::chunk) + camera::local;
    auto blend = weather::blend_region(world.x(), world.y(), world.z(), 256.0f);
    auto weight = std::sqrt(std::clamp(blend.coverage, 0.0f, 1.0f));

    // Under a fluid the colour comes from the fluid itself; the weather may dim it, but the sky tint would erase it
    if(auto def = fluid_registry::find_definition(camera::inside_fluid)) {
        distance /= def->fog_density;
        target_color = def->fog_color;
    }
    else {
        target_color = target_color * (1.0f - weight) + blend.sky_tint * weight;
    }

    target_light = 1.0f * (1.0f - weight) + blend.light_scale * weight;

    // The day dims the weather tint too, not only the clear sky it replaces
    target_color *= daylight.value();

    if(!s_primed) {
        s_color = target_color;
        s_light = target_light;
        s_primed = true;
    }

    auto step = 1.0f - std::exp(-SMOOTH_RATE * std::min(globals::window_frametime, MAX_STEP_TIME));

    s_color += (target_color - s_color) * step;
    s_light += (target_light - s_light) * step;

    color = s_color;
    light_scale = s_light;
}

void fog::reset(void)
{
    s_primed = false;
}
