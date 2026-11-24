#ifndef B3E8D4A1_6C2F_4F79_8A35_1D7C9E2B4F60
#define B3E8D4A1_6C2F_4F79_8A35_1D7C9E2B4F60

#include "shared/world/precipitation.hh"

class ModContext;

namespace precipitation_registry
{
std::span<const PrecipitationDefinition> all_definitions(void);
std::uint64_t checksum(void);
} // namespace precipitation_registry

namespace precipitation_registry
{
void commit(ModContext& ctx);
void purge(void);
} // namespace precipitation_registry

namespace precipitation_registry
{
precipitation_type_id find(const Identifier& id);
} // namespace precipitation_registry

namespace precipitation_registry
{
std::optional<Identifier> name_of(precipitation_type_id id);
} // namespace precipitation_registry

namespace precipitation_registry
{
const PrecipitationDefinition* find_definition(precipitation_type_id id);
const PrecipitationDefinition* find_definition(const Identifier& id);
} // namespace precipitation_registry

namespace precipitation_registry
{
precipitation_type_id resolve(std::string_view name);
} // namespace precipitation_registry

#endif /* B3E8D4A1_6C2F_4F79_8A35_1D7C9E2B4F60 */
