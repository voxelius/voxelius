#ifndef C94E2B10_6A7D_4F83_8E15_3D9B0C6A7F42
#define C94E2B10_6A7D_4F83_8E15_3D9B0C6A7F42

#include "shared/world/world_definition.hh"

namespace world_registry
{
std::shared_ptr<WorldDefinition> add(const Identifier& id);
std::shared_ptr<WorldDefinition> find(const Identifier& id);
std::span<const std::shared_ptr<WorldDefinition>> all(void);
void remove(const Identifier& id);
} // namespace world_registry

namespace world_registry
{
void resolve(void);
void purge(void);
} // namespace world_registry

#endif /* C94E2B10_6A7D_4F83_8E15_3D9B0C6A7F42 */
