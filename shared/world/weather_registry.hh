#ifndef B7D3E2A8_5C41_4D9B_8E27_3A9F1C6D0B52
#define B7D3E2A8_5C41_4D9B_8E27_3A9F1C6D0B52

#include "shared/world/weather.hh"

class ModContext;

namespace weather_registry
{
std::span<const WeatherTypeDefinition> all_definitions(void);
std::uint64_t checksum(void);
} // namespace weather_registry

namespace weather_registry
{
void commit(ModContext& ctx);
void purge(void);
} // namespace weather_registry

namespace weather_registry
{
weather_type_id find(const Identifier& id);
} // namespace weather_registry

namespace weather_registry
{
std::optional<Identifier> name_of(weather_type_id id);
} // namespace weather_registry

namespace weather_registry
{
const WeatherTypeDefinition* find_definition(weather_type_id id);
const WeatherTypeDefinition* find_definition(const Identifier& id);
} // namespace weather_registry

namespace weather_registry
{
weather_type_id resolve(std::string_view name);
} // namespace weather_registry

namespace weather_registry
{
// The slice of the [0, 1) roll range this type owns; the slices
// of all types tile the range, which is what makes the spatial weights continuous
bool type_interval(weather_type_id type, float& start, float& end);
} // namespace weather_registry

#endif /* B7D3E2A8_5C41_4D9B_8E27_3A9F1C6D0B52 */
