#include "client/pch.hh"

#include "client/camera.hh"

#include "core/camera.hh"
#include "core/cvar/host.hh"
#include "core/utils/angles.hh"

#include "shared/component/head.hh"
#include "shared/component/transform.hh"
#include "shared/component/velocity.hh"
#include "shared/globals.hh"
#include "shared/utils/coord.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/fluid_registry.hh"
#include "shared/world/world.hh"

#include "client/constant.hh"
#include "client/globals.hh"
#include "client/gui/container.hh"
#include "client/gui/slider.hh"
#include "client/settings.hh"

// Below this horizontal speed the loader ignores the direction of travel
constexpr static float MOVING_SPEED = 1.0f;

cvar::Number<float> client_camera::vertical_fov { 70.0f, 45.0f, 110.0f };
cvar::Number<unsigned> client_camera::view_distance { 8, 1, 32 };
cvar::Number<unsigned> client_camera::cloud_range { 16, 8, 256 };

static gui::SliderFloat s_vertical_fov_gui;
static gui::SliderUnsigned s_view_distance_gui;
static gui::SliderUnsigned s_cloud_range_gui;

Camera client_camera::instance;

Eigen::Vector3f client_camera::angles;
Eigen::Vector3f client_camera::forward;
Eigen::Vector3f client_camera::right;
Eigen::Vector3f client_camera::up;
Eigen::Vector3f client_camera::local;
ChunkPos client_camera::chunk;

float client_camera::z_near = 0.01f;
float client_camera::z_far = 1.0f;

block_id_type client_camera::inside_block;
fluid_id_type client_camera::inside_fluid;

static LocalPos s_eye_lpos;
static BlockPos s_eye_bpos;

static void reset_camera(void)
{
    client_camera::angles.setZero();

    client_camera::forward = Eigen::Vector3f::UnitZ();
    client_camera::right = Eigen::Vector3f::UnitX();
    client_camera::up = Eigen::Vector3f::UnitY();

    client_camera::local.setZero();
    client_camera::chunk.setZero();

    client_camera::inside_block = BLOCK_ID_NULL;
    client_camera::inside_fluid = FLUID_ID_NULL;

    s_eye_lpos.setZero();
    s_eye_bpos.setZero();
}

Eigen::Vector3f client_camera::lookahead(float chunks)
{
    auto& registry = globals::world->registry();

    if(!registry.valid(globals::player) || !registry.all_of<Velocity_Intr>(globals::player)) {
        return Eigen::Vector3f::Zero();
    }

    const auto& velocity = registry.get<Velocity_Intr>(globals::player);
    auto direction = Eigen::Vector2f(velocity.value.x(), velocity.value.z());
    auto speed = direction.norm();

    // Standing still has to load what is around the player, not what
    // happens to be in front of the camera: the two differ right after a spawn
    if(speed <= MOVING_SPEED) {
        return Eigen::Vector3f::Zero();
    }

    direction /= speed;

    return Eigen::Vector3f(direction.x(), 0.0f, direction.y()) * chunks;
}

void client_camera::init(void)
{
    auto& r_fov_binding = cvar::bind_var(vertical_fov, "r_fov");
    r_fov_binding.set_help("Vertical field of view");
    r_fov_binding.set_flags(FCVAR_ARCHIVE);

    s_vertical_fov_gui.set_range(45.0f, 110.0f);
    s_vertical_fov_gui.set_format("%.0f");
    s_vertical_fov_gui.enable_tooltip();
    s_vertical_fov_gui.bind("r_fov");
    settings::general.add_child(s_vertical_fov_gui, 1);

    auto& r_view_distance_binding = cvar::bind_var(view_distance, "r_view_distance");
    r_view_distance_binding.set_help("View distance, in chunks");
    r_view_distance_binding.set_flags(FCVAR_ARCHIVE);

    s_view_distance_gui.set_range(1, 32);
    s_view_distance_gui.bind("r_view_distance");
    settings::video.add_child(s_view_distance_gui, 3);

    auto& r_cloud_range_binding = cvar::bind_var(cloud_range, "r_cloud_range");
    r_cloud_range_binding.set_help("Cloud render range, in chunks");
    r_cloud_range_binding.set_flags(FCVAR_ARCHIVE);

    s_cloud_range_gui.set_range(8, 256);
    s_cloud_range_gui.bind("r_cloud_range");
    settings::video_atmosphere.add_child(s_cloud_range_gui, 0);

    z_near = 0.01f;
    z_far = 1.25f * static_cast<float>(constant::CHUNK_SIZE * view_distance.value());

    reset_camera();
}

void client_camera::update(void)
{
    auto& registry = globals::world->registry();

    if(!registry.valid(globals::player)) {
        reset_camera();
        return;
    }

    const auto& head = registry.get<Head_Intr>(globals::player);
    const auto& transform = registry.get<Transform_Intr>(globals::player);

    angles = head.angles;
    chunk = transform.chunk;
    local = transform.local + head.offset;

    z_near = 0.01f;
    z_far = 1.25f * static_cast<float>(constant::CHUNK_SIZE * view_distance.value());

    auto fov_radians = utils::radians(vertical_fov.value());
    instance.set_perspective(fov_radians, globals::aspect, z_near, z_far);
    instance.set_view(local, angles);
    instance.update();

    forward = instance.forward_vector();
    right = instance.right_vector();
    up = instance.up_vector();

    auto local_array = local.array();
    auto floored = local_array.floor();
    s_eye_lpos = floored.cast<LocalPos::value_type>();
    s_eye_bpos = utils::to_block(chunk, s_eye_lpos);

    inside_block = globals::world->get_block(s_eye_bpos);
    inside_fluid = FLUID_ID_NULL;

    auto block_def = block_registry::find_definition(inside_block);

    if(block_def && block_def->fluid && block_def->fluid_level) {
        auto fluid_def = fluid_registry::find_definition(block_def->fluid);

        if(fluid_def) {
            auto frac_y = static_cast<float>(local.y() - static_cast<float>(s_eye_lpos.y()));
            auto height = 0.0625f * static_cast<float>(block_def->fluid_level);
            auto above = s_eye_bpos;

            if(fluid_def->gravity == FLUID_GRAVITY_DOWN) {
                above.y() += 1;
            }
            else {
                above.y() -= 1;
            }

            auto above_def = block_registry::find_definition(globals::world->get_block(above));

            if(above_def && above_def->fluid == block_def->fluid) {
                height = 1.0f;
            }

            if(fluid_def->gravity == FLUID_GRAVITY_DOWN) {
                if(frac_y <= height) {
                    inside_fluid = block_def->fluid;
                }
            }
            else {
                if(frac_y >= (1.0f - height)) {
                    inside_fluid = block_def->fluid;
                }
            }
        }
    }
}