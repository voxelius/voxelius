#include "shared/pch.hh"

#include "shared/game.hh"

#include "shared/component/head.hh"
#include "shared/component/move_mode.hh"
#include "shared/component/player.hh"
#include "shared/component/transform.hh"
#include "shared/component/velocity.hh"
#include "shared/mod_loader.hh"
#include "shared/res/block_collision.hh"
#include "shared/res/block_model.hh"
#include "shared/res/feature.hh"
#include "shared/system/pmove.hh"
#include "shared/world/biome_map.hh"
#include "shared/world/block_collisions.hh"
#include "shared/world/light_material.hh"
#include "shared/world/lighting.hh"

void shared_game::init(void)
{
    BlockCollision::register_resource();
    BlockModel::register_resource();
    Feature::register_resource();

    Transform::register_component();
    Head::register_component();
    Velocity::register_component();
    Player::register_component();
    MoveMode::register_component();

    mod_loader::init();

    lighting::init();
}

void shared_game::init_late(void)
{
    block_collisions::init_late();
    light_material::init_late();
}

void shared_game::shutdown(void)
{
    lighting::shutdown();

    light_material::shutdown();
    block_collisions::shutdown();

    mod_loader::shutdown();
}

void shared_game::fixed_update(World* world)
{
    ZoneScoped;

    if(world) {
        lighting::fixed_update(world);
    }
}

void shared_game::fixed_update_late(void)
{
    ZoneScoped;

    // empty
}
