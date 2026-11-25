#include "client/pch.hh"

#include "client/fog.hh"

#include "core/cvar/host.hh"
#include "core/cvar/number.hh"

#include "shared/world/fluid_registry.hh"

#include "client/camera.hh"
#include "client/constant.hh"
#include "client/globals.hh"
#include "client/gui/container.hh"
#include "client/gui/stepper.hh"
#include "client/settings.hh"
#include "client/world/skybox.hh"

constexpr static float SMOOTH_RATE = 3.0f;
constexpr static float MAX_STEP_TIME = 0.1f;

cvar::Number<unsigned> fog::model { 0, 0, 2 };

static gui::StepperUnsigned s_fog_model_gui;

float fog::distance;
Eigen::Vector3f fog::color;
float fog::light_scale { 1.0f };
float fog::daylight { 1.0f };

static Eigen::Vector3f s_color;
static bool s_primed { false };

void fog::init(void)
{
    auto& r_fog_binding = cvar::bind_var(model, "r_fog");
    r_fog_binding.set_help("Fog style: 0 = off, 1 = simple, 2 = fancy");
    r_fog_binding.set_flags(FCVAR_ARCHIVE);

    s_fog_model_gui.set_range(0, 2, 1);
    s_fog_model_gui.bind("r_fog");
    settings::video_atmosphere.add_child(s_fog_model_gui, 3);
}

void fog::update(void)
{
    distance = static_cast<float>(constant::CHUNK_SIZE * client_camera::view_distance.value());

    // The atmosphere owns the sky color and the weather tint; the fog
    // only copies them so the terrain dissolves into the same horizon
    auto target_color = skybox::fog_color;

    daylight = skybox::daylight;
    light_scale = skybox::light_scale;

    if(auto def = fluid_registry::find_definition(client_camera::inside_fluid)) {
        distance /= def->fog_density;
        target_color = def->fog_color;
    }

    if(!s_primed) {
        s_color = target_color;
        s_primed = true;
    }

    auto step = 1.0f - std::exp(-SMOOTH_RATE * std::min(globals::window_frametime, MAX_STEP_TIME));

    s_color += (target_color - s_color) * step;

    color = s_color;
}

void fog::reset(void)
{
    s_primed = false;
}