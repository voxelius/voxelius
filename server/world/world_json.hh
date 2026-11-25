#ifndef A3F2A7C4_9B6E_4D58_A1C3_7E5B2D9F8A60
#define A3F2A7C4_9B6E_4D58_A1C3_7E5B2D9F8A60

#include "server/world/world_definition.hh"

namespace world_json
{
std::shared_ptr<ServerWorldDefinition> parse_world(std::string_view name, const JSON_Object* object, std::string_view where);
} // namespace world_json

#endif /* A3F2A7C4_9B6E_4D58_A1C3_7E5B2D9F8A60 */
