#include "server/pch.hh"

#include "server/system/pmove_validator.hh"

#include "core/buffer.hh"
#include "core/cvar/host.hh"
#include "core/cvar/number.hh"

#include "shared/component/head.hh"
#include "shared/component/move_mode.hh"
#include "shared/component/transform.hh"
#include "shared/component/velocity.hh"
#include "shared/entity/component_map.hh"
#include "shared/net/packet_entity.hh"
#include "shared/net/protocol.hh"
#include "shared/utils/coord.hh"

#include "server/globals.hh"
#include "server/net/sessions.hh"

static cvar::Number<float> s_pos_threshold { 10.0f, 0.0f, 100.0f };
static cvar::Number<float> s_vel_threshold { 10.0f, 0.0f, 100.0f };

static void send_full_update(entt::entity entity)
{
    static packet::Entity_Patch packet;

    auto head_id = component_map::from_type<Head>();
    auto transform_id = component_map::from_type<Transform>();
    auto velocity_id = component_map::from_type<Velocity>();
    assert(head_id && transform_id && velocity_id);

    packet.entity = entity;
    packet.components.clear();

    for(auto id : std::array { head_id, transform_id, velocity_id }) {
        WriteBuffer buffer;
        component_map::encode_net(id, entity, buffer);

        packet::Entity_Patch::Component component;
        component.id = id;
        component.data = std::move(buffer.take());

        packet.components.emplace_back(std::move(component));
    }

    auto& ref = globals::registry.get<SessionRef>(entity);
    protocol::send(packet, ref.ptr->peer);
}

void pmove_validator::init(void)
{
    auto& pm_pos_threshold_binding = cvar::bind_var(s_pos_threshold, "pm_pos_threshold");
    pm_pos_threshold_binding.set_help("Maximum position desync before a full update");
    pm_pos_threshold_binding.set_flags(FCVAR_ARCHIVE);

    auto& pm_vel_threshold_binding = cvar::bind_var(s_vel_threshold, "pm_vel_threshold");
    pm_vel_threshold_binding.set_help("Maximum velocity desync before a full update");
    pm_vel_threshold_binding.set_flags(FCVAR_ARCHIVE);
}

void pmove_validator::fixed_update_late(void)
{
    auto view = globals::registry.view<MoveMode, SessionRef, SimulatedMoveData, Transform, Velocity>();

    for(const auto [entity, move_mode, ref, sim_data, transform, velocity] : view.each()) {
        BlockPos sim_bpos = utils::to_block(sim_data.chunk, sim_data.local.cast<LocalPos::value_type>());
        BlockPos cur_bpos = utils::to_block(transform.chunk, transform.local.cast<LocalPos::value_type>());

        Eigen::Vector3f delta_vel = sim_data.velocity - velocity.value;
        BlockPos delta_bpos = sim_bpos - cur_bpos;

        auto dist_vel = delta_vel.norm();
        auto dist_bpos = delta_bpos.cast<float>().norm();

        auto rubberband = false;
        rubberband = rubberband || dist_vel > s_vel_threshold.value();
        rubberband = rubberband || dist_bpos > s_pos_threshold.value();

        if(rubberband) {
            msg::log_warning("rubberbanding entity {}: dist_vel={} dist_bpos={}", static_cast<std::uint64_t>(entity), dist_vel, dist_bpos);

            send_full_update(entity);
        }
        else {
            globals::registry.patch<Head>(entity, [&](Head& phead) {
                phead.angles.x() = sim_data.angles.x();
                phead.angles.z() = sim_data.angles.z();
            });

            globals::registry.patch<Transform>(entity, [&](Transform& ptransform) {
                ptransform.chunk = sim_data.chunk;
                ptransform.local = sim_data.local;
                ptransform.angles.y() = sim_data.angles.y();
            });

            globals::registry.patch<Velocity>(entity, [&](Velocity& pvelocity) {
                pvelocity.value = sim_data.velocity;
            });
        }
    }
}
