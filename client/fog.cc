#include "client/pch.hh"

#include "client/fog.hh"

#include "core/config/ref.hh"
#include "core/cvar/host.hh"

#include "shared/world/fluid_registry.hh"

#include "client/camera.hh"
#include "client/constant.hh"
#include "client/gui/slider.hh"
#include "client/world/skybox.hh"

float fog::distance;
Eigen::Vector3f fog::color;

cvar::Number<float> fog::daylight { 1.0f, 0.0f, 1.0f };

void fog::init(void)
{
    auto& daylight_binding = cvar::bind_var(daylight, "r_daylight");
    daylight_binding.set_help("Brightness of the daylight: 1 is noon, lower is dusk");
    daylight_binding.set_flags(FCVAR_ARCHIVE);
}

void fog::update(void)
{
    distance = static_cast<float>(constant::CHUNK_SIZE * camera::view_distance.value());
    color = skybox::sky_color;

    if(auto def = fluid_registry::find_definition(camera::inside_fluid)) {
        distance /= def->fog_density;
        color = def->fog_color;
    }

    color *= daylight.value();
}
