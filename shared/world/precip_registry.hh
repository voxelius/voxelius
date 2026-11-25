#ifndef B3E8D4A1_6C2F_4F79_8A35_1D7C9E2B4F60
#define B3E8D4A1_6C2F_4F79_8A35_1D7C9E2B4F60

#include "shared/world/precip.hh"

class ModContext;

namespace precip_registry
{
std::span<const PrecipDefinition> all_definitions(void);
std::uint64_t checksum(void);
} // namespace precip_registry

namespace precip_registry
{
void commit(ModContext& ctx);
void purge(void);
} // namespace precip_registry

namespace precip_registry
{
precip_type_id find(const Identifier& id);
} // namespace precip_registry

namespace precip_registry
{
std::optional<Identifier> name_of(precip_type_id id);
} // namespace precip_registry

namespace precip_registry
{
const PrecipDefinition* find_definition(precip_type_id id);
const PrecipDefinition* find_definition(const Identifier& id);
} // namespace precip_registry

namespace precip_registry
{
precip_type_id resolve(std::string_view name);
} // namespace precip_registry

#endif /* B3E8D4A1_6C2F_4F79_8A35_1D7C9E2B4F60 */
