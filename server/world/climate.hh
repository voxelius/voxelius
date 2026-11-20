#ifndef F73C497B_F125_46D5_AAB3_7F8EC86D13FC
#define F73C497B_F125_46D5_AAB3_7F8EC86D13FC

#include "shared/world/biome.hh"

class ServerWorld;

constexpr static std::size_t CLIMATE_DIMS = 5;

using ClimatePos = std::array<std::uint8_t, CLIMATE_DIMS>;

struct ClimateSeed final {
    ClimatePos target;
    biome_id_type id;
    unsigned priority;
    float offset;
};

struct ClimateSample final {
    float temperature;
    float humidity;
    float continentalness;
    float erosion;
    float weirdness;
};

namespace climate
{
void rebuild(void);
void rebuild(ServerWorld* world);
} // namespace climate

namespace climate
{
biome_id_type find(ServerWorld* world, const ClimateSample& sample);
float peaks_valleys(float weirdness);
float normalize_01(float sample);
} // namespace climate

#endif /* F73C497B_F125_46D5_AAB3_7F8EC86D13FC */
