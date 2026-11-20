#include "shared/pch.hh"

#include "shared/world/world_scope.hh"

#include "shared/globals.hh"

WorldScope::WorldScope(World* world) : m_previous(std::exchange(globals::world, world))
{
    // empty
}

WorldScope::~WorldScope(void)
{
    globals::world = m_previous;
}
