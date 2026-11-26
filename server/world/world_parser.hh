#ifndef A3F2A7C4_9B6E_4D58_A1C3_7E5B2D9F8A60
#define A3F2A7C4_9B6E_4D58_A1C3_7E5B2D9F8A60

#include "server/world/world_definition.hh"

namespace world_parser
{
std::shared_ptr<ServerWorldDefinition> parse_world(std::string_view name, ryml::ConstNodeRef object, std::string_view where);
} // namespace world_parser

#endif /* A3F2A7C4_9B6E_4D58_A1C3_7E5B2D9F8A60 */
