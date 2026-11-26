#include "shared/pch.hh"

#include "shared/world/world_scope.hh"

static thread_local World* s_active { nullptr };

World* WorldScope::active(void)
{
    return s_active;
}

WorldScope::WorldScope(World* world) : m_previous(s_active)
{
    s_active = world;
}

WorldScope::~WorldScope(void)
{
    s_active = m_previous;
}
