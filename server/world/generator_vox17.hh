#ifndef A7C1B3D5_E2F4_4A6B_8C0D_1E3F5A7B9C21
#define A7C1B3D5_E2F4_4A6B_8C0D_1E3F5A7B9C21

#include "shared/coord.hh"
#include "shared/world/biome_slice.hh"
#include "shared/world/block_storage.hh"

class World;
struct ClimateSeed;

struct Vox17 final {
    static void register_generator(void);

    static std::vector<ClimateSeed>& climate_seeds(World* world);
};

#endif /* A7C1B3D5_E2F4_4A6B_8C0D_1E3F5A7B9C21 */
