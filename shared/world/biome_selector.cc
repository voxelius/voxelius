#include "shared/pch.hh"

#include "shared/world/biome_selector.hh"

#include "core/exception.hh"

constexpr static int CLIMATE_SIZE = 100;
constexpr static float TARGET_SCALE = 1.0f / static_cast<float>(CLIMATE_SIZE - 1);
constexpr static std::size_t MAX_NUDGE_STEPS = 1000000;
constexpr static std::uint32_t NUDGE_SEED = UINT32_C(0xB10BEE5E);

static std::string pack_pos(const std::vector<std::uint8_t>& pos)
{
    std::string key;
    key.reserve(pos.size());

    for(auto coordinate : pos) {
        key.push_back(static_cast<char>(coordinate));
    }

    return key;
}

static std::vector<std::vector<std::int8_t>> make_neighbour_dirs(std::size_t dims)
{
    auto total = std::size_t { 1 };

    for(std::size_t i = 0; i < dims; ++i) {
        total *= 3;
    }

    std::vector<std::vector<std::int8_t>> dirs;
    dirs.reserve(total - 1);

    for(std::size_t n = 0; n < total; ++n) {
        auto value = n;
        auto dir = std::vector<std::int8_t>(dims);
        auto nonzero = false;

        for(std::size_t d = 0; d < dims; ++d) {
            auto digit = static_cast<int>(value % 3);
            value /= 3;

            auto coordinate = digit - 1;
            dir[dims - 1 - d] = static_cast<std::int8_t>(coordinate);
            nonzero = nonzero || coordinate;
        }

        if(nonzero) {
            dirs.push_back(std::move(dir));
        }
    }

    return dirs;
}

static bool nudge(vx::hash_map<std::string, biome_id_type>& occupied, std::vector<std::uint8_t>& pos, biome_id_type id,
    std::mt19937& random, const std::vector<std::vector<std::int8_t>>& dirs)
{
    auto key = pack_pos(pos);

    if(occupied.count(key) == 0) {
        occupied.emplace(std::move(key), id);
        return true;
    }

    std::uniform_int_distribution<std::size_t> dir_dist(0, dirs.size() - 1);
    auto direction = dirs.at(dir_dist(random));
    auto current = pos;

    for(std::size_t step = 0; step < MAX_NUDGE_STEPS; step += 1) {
        auto next = std::vector<std::uint8_t>(current.size());

        while(true) {
            auto in_bounds = true;

            for(std::size_t i = 0; i < current.size(); ++i) {
                auto value = static_cast<int>(current[i]) + direction[i];

                if(value < 0 || value >= CLIMATE_SIZE) {
                    in_bounds = false;
                    break;
                }

                next[i] = static_cast<std::uint8_t>(value);
            }

            if(in_bounds) {
                break;
            }

            direction = dirs.at(dir_dist(random));
        }

        current = next;
        key = pack_pos(current);

        if(occupied.count(key) == 0) {
            occupied.emplace(std::move(key), id);
            pos = current;
            return true;
        }
    }

    return false;
}

biome_id_type BiomeSelector::select(std::span<const float> sample) const
{
    if(strategy == BIOME_SELECTION_CONSTANT) {
        return constant_biome;
    }

    if(strategy == BIOME_SELECTION_GRID) {
        return BIOME_ID_NULL; // unreachable: finalize() rejects grid
    }

    auto best_id = BIOME_ID_NULL;
    auto best_score = std::numeric_limits<float>::max();
    auto best_priority = 0U;

    for(const auto& entry : entries) {
        auto dist_squared = 0.0f;

        for(std::size_t i = 0; i < weights.size(); ++i) {
            auto target = static_cast<float>(entry.target[i]) * TARGET_SCALE;
            auto delta = sample[i] - target;
            delta *= weights[i];

            dist_squared += delta * delta;
        }

        auto score = dist_squared + entry.offset;

        if(score < best_score || (score == best_score && entry.priority > best_priority)) {
            best_score = score;
            best_priority = entry.priority;
            best_id = entry.biome;
        }
    }

    return best_id;
}

void BiomeSelector::finalize(void)
{
    vx::throw_if(strategy == BIOME_SELECTION_GRID, "grid biome selection is not implemented");

    if(strategy != BIOME_SELECTION_NEAREST) {
        return;
    }

    vx::throw_if_not(axes.size() == weights.size(), "biome selector: axis count must match weight count");

    for(const auto& entry : entries) {
        vx::throw_if_not(entry.target.size() == weights.size(), "biome selector: entry target size must match axis count");
    }

    std::stable_sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        return a.priority > b.priority;
    });

    auto dirs = make_neighbour_dirs(weights.size());

    std::mt19937 random;
    random.seed(NUDGE_SEED);

    vx::hash_map<std::string, biome_id_type> occupied;

    for(auto& entry : entries) {
        if(!nudge(occupied, entry.target, entry.biome, random, dirs)) {
            msg::log_warning("failed to place biome {} uniquely", entry.biome);
        }
    }
}
