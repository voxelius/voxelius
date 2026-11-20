#ifndef B7D9F1C3_3E5A_4B2D_9C6F_8A0E2C4D6F71
#define B7D9F1C3_3E5A_4B2D_9C6F_8A0E2C4D6F71

#include "server/world/climate.hh"
#include "shared/world/world.hh"

struct GeneratorDefinition;

class ServerWorld final : public World {
public:
    explicit ServerWorld(std::shared_ptr<const WorldDefinition> definition);

    constexpr const GeneratorDefinition* generator(void) const;
    void set_generator(const GeneratorDefinition* generator);

    constexpr std::vector<ClimateSeed>& climate_seeds(void);
    constexpr const std::vector<ClimateSeed>& climate_seeds(void) const;

    constexpr void* generator_state(void) const;
    void set_generator_state(std::shared_ptr<void> state);

private:
    const GeneratorDefinition* m_generator { nullptr };
    std::vector<ClimateSeed> m_climate_seeds;
    std::shared_ptr<void> m_generator_state;
};

constexpr const GeneratorDefinition* ServerWorld::generator(void) const
{
    return m_generator;
}

constexpr std::vector<ClimateSeed>& ServerWorld::climate_seeds(void)
{
    return m_climate_seeds;
}

constexpr const std::vector<ClimateSeed>& ServerWorld::climate_seeds(void) const
{
    return m_climate_seeds;
}

constexpr void* ServerWorld::generator_state(void) const
{
    return m_generator_state.get();
}

#endif /* B7D9F1C3_3E5A_4B2D_9C6F_8A0E2C4D6F71 */
