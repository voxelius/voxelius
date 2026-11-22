#include "shared/pch.hh"

#include "shared/world/world_scope.hh"

// Per thread: a world may tick on its own future later, and the API then
// reads the world of the thread that runs the callback
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
