#ifndef F73C497B_F125_46D5_AAB3_7F8EC86D13FC
#define F73C497B_F125_46D5_AAB3_7F8EC86D13FC

#include "shared/world/biome.hh"

class World;

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
void rebuild(World* world);
} // namespace climate

namespace climate
{
biome_id_type find(World* world, const ClimateSample& sample);
float peaks_valleys(float weirdness);
float normalize_01(float sample);
} // namespace climate

#endif /* F73C497B_F125_46D5_AAB3_7F8EC86D13FC */
