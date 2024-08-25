#pragma once
#include <entt/entity/entity.hpp>
#include <game/shared/chunk.hh>

struct Session;

namespace server_send
{
void chunk(Session *session, entt::entity entity);
void head(Session *session, entt::entity entity);
void transform(Session *session, entt::entity entity);
void velocity(Session *session, entt::entity entity);
} // namespace server_send
