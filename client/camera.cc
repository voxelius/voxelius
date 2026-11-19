#include "client/pch.hh"

#include "client/camera.hh"

#include "core/camera.hh"
#include "core/cvar/host.hh"
#include "core/utils/angles.hh"

#include "shared/component/head.hh"
#include "shared/component/transform.hh"
#include "shared/component/velocity.hh"
#include "shared/utils/coord.hh"
#include "shared/world/block_registry.hh"
#include "shared/world/fluid_registry.hh"
#include "shared/world/world.hh"

#include "client/constant.hh"
#include "client/globals.hh"
#include "client/gui/container.hh"
#include "client/gui/slider.hh"
#include "client/settings.hh"
#include "client/world/lod_manager.hh"

// Below this horizontal speed the loader ignores the direction of travel
constexpr static float MOVING_SPEED = 1.0f;

cvar::Number<float> camera::vertical_fov { 70.0f, 45.0f, 110.0f };
cvar::Number<unsigned> camera::view_distance { 8, 1, 32 };
cvar::Number<unsigned> camera::cloud_range { 16, 8, 256 };

static gui::SliderFloat s_vertical_fov_gui;
static gui::SliderUnsigned s_view_distance_gui;
static gui::SliderUnsigned s_cloud_range_gui;

Camera camera::instance;

Eigen::Vector3f camera::angles;
Eigen::Vector3f camera::forward;
Eigen::Vector3f camera::right;
Eigen::Vector3f camera::up;
Eigen::Vector3f camera::local;
ChunkPos camera::chunk;

float camera::z_near = 0.01f;
float camera::z_far = 1.0f;

block_id_type camera::inside_block;
fluid_id_type camera::inside_fluid;

static LocalPos s_eye_lpos;
static BlockPos s_eye_bpos;

static float far_distance(void)
{
    auto chunk_distance = std::max(camera::view_distance.value(), static_cast<unsigned>(lod_manager::range_chunks()));
    return 1.25f * static_cast<float>(constant::CHUNK_SIZE * chunk_distance);
}

static float near_distance(void)
{
    // An extended horizon would lose too much depth precision behind a
    // centimetre near plane; collision keeps the camera well outside this.
    return lod_manager::range_chunks() > 0 ? 0.1f : 0.01f;
}

static void reset_camera(void)
{
    camera::angles.setZero();

    camera::forward = Eigen::Vector3f::UnitZ();
    camera::right = Eigen::Vector3f::UnitX();
    camera::up = Eigen::Vector3f::UnitY();

    camera::local.setZero();
    camera::chunk.setZero();

    camera::inside_block = BLOCK_ID_NULL;
    camera::inside_fluid = FLUID_ID_NULL;

    s_eye_lpos.setZero();
    s_eye_bpos.setZero();
}

Eigen::Vector3f camera::lookahead(float chunks)
{
    auto& registry = globals::registry;

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

void camera::init(void)
{
    auto& vertical_fov_binding = cvar::bind_var(vertical_fov, "r_fov");
    vertical_fov_binding.set_help("Vertical field of view");
    vertical_fov_binding.set_flags(FCVAR_ARCHIVE);

    s_vertical_fov_gui.set_range(45.0f, 110.0f).set_format("%.0f").enable_tooltip();
    s_vertical_fov_gui.bind("r_fov");
    settings::general.add_child(s_vertical_fov_gui, 1);

    auto& view_distance_binding = cvar::bind_var(view_distance, "r_view_distance");
    view_distance_binding.set_help("View distance, in chunks");
    view_distance_binding.set_flags(FCVAR_ARCHIVE);

    s_view_distance_gui.set_range(1, 32);
    s_view_distance_gui.bind("r_view_distance");
    settings::video_lod.add_child(s_view_distance_gui, 0);

    auto& cloud_range_binding = cvar::bind_var(cloud_range, "r_cloud_range");
    cloud_range_binding.set_help("Cloud render range, in chunks");
    cloud_range_binding.set_flags(FCVAR_ARCHIVE);

    s_cloud_range_gui.set_range(8, 256);
    s_cloud_range_gui.bind("r_cloud_range");
    settings::video_atmosphere.add_child(s_cloud_range_gui, 0);

    camera::z_near = near_distance();
    camera::z_far = far_distance();

    reset_camera();
}

void camera::update(void)
{
    if(!globals::registry.valid(globals::player)) {
        reset_camera();
        return;
    }

    const auto& head = globals::registry.get<Head_Intr>(globals::player);
    const auto& transform = globals::registry.get<Transform_Intr>(globals::player);

    camera::angles = head.angles;
    camera::chunk = transform.chunk;
    camera::local = transform.local + head.offset;

    camera::z_near = near_distance();
    camera::z_far = far_distance();

    instance.set_perspective(utils::radians(vertical_fov.value()), globals::aspect, camera::z_near, camera::z_far);
    instance.set_view(camera::local, camera::angles);
    instance.update();

    forward = instance.forward_vector();
    right = instance.right_vector();
    up = instance.up_vector();

    s_eye_lpos = camera::local.array().floor().cast<LocalPos::value_type>();
    s_eye_bpos = utils::to_block(camera::chunk, s_eye_lpos);

    inside_block = world::get_block(s_eye_bpos);
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

            auto above_def = block_registry::find_definition(world::get_block(above));

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
