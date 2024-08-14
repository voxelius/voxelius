#ifndef SERVER_SEND_HH
#define SERVER_SEND_HH
#include <entt/entity/entity.hpp>
#include <shared/chunk.hh>

struct Session;

namespace server_send
{
void chunk(Session *session, entt::entity entity);
void head(Session *session, entt::entity entity);
void transform(Session *session, entt::entity entity);
void velocity(Session *session, entt::entity entity);
} // namespace server_send

#endif /* SERVER_SEND_HH */
