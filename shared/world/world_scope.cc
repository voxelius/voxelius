#include "shared/pch.hh"

#include "shared/world/world_scope.hh"

#include "shared/globals.hh"

WorldScope::WorldScope(std::shared_ptr<World> world) : m_previous(std::exchange(globals::world, std::move(world)))
{
    // empty
}

WorldScope::~WorldScope(void)
{
    globals::world = std::move(m_previous);
}
