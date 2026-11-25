#ifndef A4E7C2B9_8D1F_4C3A_9E6B_2F5D7A8C0B1E
#define A4E7C2B9_8D1F_4C3A_9E6B_2F5D7A8C0B1E

#include "core/identifier.hh"

#include "shared/world/biome.hh"

constexpr static std::size_t BIOME_SELECTOR_MAX_GRID_SIZE = 128;

enum biome_selection_strategy : unsigned {
    BIOME_SELECTION_NEAREST = 0,
    BIOME_SELECTION_GRID,
    BIOME_SELECTION_CONSTANT,
};

struct BiomeSelector final {
    struct Entry final {
        Identifier biome_name;
        std::vector<std::uint8_t> target;
        unsigned priority { 0 };
        float offset { 0.0f };

        // Resolved from biome_name in world_loader::load()
        biome_id_type biome { BIOME_ID_NULL };
    };

    biome_id_type select(std::span<const float> sample) const;
    void finalize(void);

    biome_selection_strategy strategy { BIOME_SELECTION_CONSTANT };

    std::vector<std::string> axes;
    std::vector<float> weights;

    std::vector<Entry> entries;

    std::size_t grid_size { 0 };
    std::vector<Identifier> lookup_names;
    std::vector<biome_id_type> lookup;

    Identifier constant_biome_name;
    biome_id_type constant_biome { BIOME_ID_NULL };
};

#endif /* A4E7C2B9_8D1F_4C3A_9E6B_2F5D7A8C0B1E */
