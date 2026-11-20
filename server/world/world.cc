#include "server/pch.hh"

#include "server/world/world.hh"

ServerWorld::ServerWorld(std::shared_ptr<const WorldDefinition> definition) : World(std::move(definition))
{
    // empty
}

void ServerWorld::set_generator(const GeneratorDefinition* generator)
{
    assert(generator);

    m_generator = generator;
}

void ServerWorld::set_generator_state(std::shared_ptr<void> state)
{
    m_generator_state = std::move(state);
}
