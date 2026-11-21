#include "shared/pch.hh"

#include "shared/world/lighting.hh"

#include "core/threading.hh"

#include "shared/component/player.hh"
#include "shared/component/transform.hh"
#include "shared/constant.hh"
#include "shared/globals.hh"
#include "shared/utils/coord.hh"
#include "shared/world/chunk.hh"
#include "shared/world/light_material.hh"
#include "shared/world/light_storage.hh"
#include "shared/world/world.hh"

// A chunk is lit in a 3x3x3 neighbourhood. The neighbours are a source of
// seeds only: the task writes back its own chunk and never recomputes the
// chunks around it, so re-running it on an already lit world changes
// nothing.
constexpr static int LIGHT_PADDING = 1;
constexpr static int LIGHT_GRID_SIZE = static_cast<int>(constant::CHUNK_SIZE) + 2 * LIGHT_PADDING;
constexpr static std::size_t LIGHT_GRID_VOLUME = static_cast<std::size_t>(LIGHT_GRID_SIZE) * static_cast<std::size_t>(LIGHT_GRID_SIZE)
    * static_cast<std::size_t>(LIGHT_GRID_SIZE);

// Matches the old per-column throughput: many small tasks instead of a few
// whole-column ones, so the budget stays the same amount of cells per tick.
constexpr static std::size_t LIGHT_TASKS_PER_TICK = 64;

// Runtime block edits are propagated by a classic per-cell BFS so a
// single block change only touches the cells it can actually affect.
constexpr static std::size_t LOCAL_BUDGET = 32768;
constexpr static std::size_t LOCAL_MAX_QUEUED = 262144;

// A delta costs 3 bytes per cell, so a chunk with more changes than this is
// sent whole instead.
constexpr static std::size_t MAX_CHANGE_INDICES = 1024;

struct LocalLightNode final {
    std::int16_t x;
    std::int16_t y;
    std::int16_t z;
    std::uint8_t level;
    light_channel channel;
};

struct LightNode final {
    std::shared_ptr<Chunk> chunk;
    ChunkPos cpos;
    std::uint16_t index;
    block_light_type level;
    light_channel channel;
};

struct NeighbourCell final {
    std::shared_ptr<Chunk> chunk;
    ChunkPos cpos;
    std::uint16_t index;
    bool downward;
};

class ChunkLightTask final : public Task {
public:
    ChunkLightTask(World* world, ChunkPos pos);
    virtual ~ChunkLightTask(void) override;
    virtual void process(void) override;
    virtual void finalize(void) override;

private:
    std::size_t index(int x, int y, int z) const;

    World* m_world;
    ChunkPos m_cpos;
    std::array<block_id_type, LIGHT_GRID_VOLUME> m_blocks;
    std::array<std::uint8_t, LIGHT_GRID_VOLUME> m_light;
    LightStorage m_result;
    std::uint64_t m_generation { 0 };
    bool m_above { false };
    bool m_ready { false };
    bool m_empty { true };
    bool m_open_sky { false };
    bool m_realm_top { false };
};

constexpr static std::array<std::array<int, 4>, 6> NEIGHBOURS = { {
    { { -1, 0, 0, 0 } },
    { { +1, 0, 0, 0 } },
    { { 0, -1, 0, 1 } },
    { { 0, +1, 0, 1 } },
    { { 0, 0, -1, 2 } },
    { { 0, 0, +1, 2 } },
} };

static vx::hash_set<ChunkPos> s_dirty;
static vx::hash_set<ChunkPos> s_in_flight;

static std::array<std::deque<LightNode>, 16> s_add;
static std::deque<LightNode> s_remove;
static std::vector<BlockPos> s_pending_relight;
static vx::hash_set<ChunkPos> s_changed;
static vx::hash_map<ChunkPos, lighting::LightChange> s_change;
static std::size_t s_queued;

static std::uint64_t s_generation;
static bool s_replica;

static std::size_t grid_index(int x, int y, int z)
{
    auto ux = static_cast<std::size_t>(x);
    auto uy = static_cast<std::size_t>(y);
    auto uz = static_cast<std::size_t>(z);
    auto size = static_cast<std::size_t>(LIGHT_GRID_SIZE);
    return ux + uy * size + uz * size * size;
}

static block_light_type channel_level(std::uint8_t packed, light_channel channel)
{
    if(channel == LIGHT_SKY) {
        return static_cast<block_light_type>(packed >> 4U);
    }

    return static_cast<block_light_type>(packed & 0x0FU);
}

// The flood fill never lowers a level, so a cell that is not brighter than any
// of its neighbours has nothing to give. Seeding only the cells that are
// brighter turns a pass over every lit cell into a pass over the light
// boundaries, which matters under an open sky where everything is lit.
static bool needs_seed(const std::array<std::uint8_t, LIGHT_GRID_VOLUME>& light, int x, int y, int z, light_channel channel)
{
    auto level = channel_level(light[grid_index(x, y, z)], channel);

    for(const auto& offset : NEIGHBOURS) {
        auto nx = x + offset[0];
        auto ny = y + offset[1];
        auto nz = z + offset[2];

        if(nx < 0 || nx >= LIGHT_GRID_SIZE || ny < 0 || ny >= LIGHT_GRID_SIZE || nz < 0 || nz >= LIGHT_GRID_SIZE) {
            continue;
        }

        if(channel_level(light[grid_index(nx, ny, nz)], channel) < level) {
            return true;
        }
    }

    return false;
}

static bool registry_valid(World& world, entt::entity entity)
{
    auto& chunk_registry = world.chunk_registry();
    return chunk_registry.valid(entity);
}

static bool realm_top(const World& world, ChunkPos::value_type y)
{
    auto& definition = world.definition();
    auto top = static_cast<ChunkPos::value_type>(definition.max_y >> constant::CHUNK_SIZE_LOG2);
    return y == top;
}

static bool sky_ready(const World& world, const ChunkPos& pos)
{
    // Sunlight enters a chunk through the bottom row of the chunk above, so
    // a chunk below an unlit one has to wait for it.
    if(realm_top(world, pos.y())) {
        return true;
    }

    auto above = world.find_chunk(ChunkPos(pos.x(), pos.y() + 1, pos.z()));

    // A chunk at the top of the loaded world has nothing above it and the
    // column pass already treats that as open sky. Waiting for a neighbour
    // that is not loaded keeps such a chunk unlit for as long as it stays at
    // the edge, and every chunk below it with it.
    return above == nullptr || above->has_light();
}

static std::int64_t chunk_distance(World& world, const ChunkPos& pos)
{
    auto& registry = world.registry();

    auto best = std::numeric_limits<std::int64_t>::max();
    auto players = registry.view<Player, Transform>();

    for(const auto [entity, transform] : players.each()) {
        auto dx = static_cast<std::int64_t>(pos.x()) - transform.chunk.x();
        auto dz = static_cast<std::int64_t>(pos.z()) - transform.chunk.z();
        auto distance = dx * dx + dz * dz;

        best = std::min(best, distance);
    }

    return best;
}

block_light_type lighting::step_level(block_light_type level, block_light_type cost, light_channel channel, bool downward)
{
    if(channel == LIGHT_SKY && downward && cost == BLOCK_LIGHT_MIN) {
        return level;
    }

    auto step = std::max<unsigned>(1U, cost);

    if(step >= level) {
        return BLOCK_LIGHT_MIN;
    }

    return static_cast<block_light_type>(level - step);
}

// The face of the destination cell that a step along `offset` crosses.
static block_face enter_face(const std::array<int, 4>& offset)
{
    if(offset[0] < 0) {
        return BLOCK_FACE_EAST;
    }

    if(offset[0] > 0) {
        return BLOCK_FACE_WEST;
    }

    if(offset[1] < 0) {
        return BLOCK_FACE_TOP;
    }

    if(offset[1] > 0) {
        return BLOCK_FACE_BOTTOM;
    }

    if(offset[2] < 0) {
        return BLOCK_FACE_SOUTH;
    }

    return BLOCK_FACE_NORTH;
}

static void mark_dirty(const ChunkPos& pos)
{
    if(!s_replica) {
        s_dirty.insert(pos);
    }
}

static void record_light_change(const ChunkPos& pos, std::size_t index)
{
    auto& change = s_change[pos];

    if(change.full) {
        return;
    }

    change.indices.push_back(static_cast<std::uint16_t>(index));
}

static void mark_full_light_change(const ChunkPos& pos)
{
    auto& change = s_change[pos];

    change.full = true;
    change.indices.clear();
}

static void mark_face_neighbors_dirty(const ChunkPos& pos)
{
    for(const auto& offset : NEIGHBOURS) {
        mark_dirty(pos + ChunkPos(offset[0], offset[1], offset[2]));
    }
}

static bool same_light(const LightStorage& first, const LightStorage& second)
{
    auto left = first.data();
    auto right = second.data();
    return std::equal(left.begin(), left.end(), right.begin());
}

ChunkLightTask::ChunkLightTask(World* world, ChunkPos pos) : m_world(world), m_cpos(std::move(pos)), m_generation(s_generation)
{
    m_blocks.fill(BLOCK_ID_NULL);
    m_light.fill(0x00);

    constexpr auto chunk_size = static_cast<int>(constant::CHUNK_SIZE);

    for(int dy = -1; dy <= 1; dy += 1) {
        for(int dz = -1; dz <= 1; dz += 1) {
            for(int dx = -1; dx <= 1; dx += 1) {
                auto cpos = ChunkPos(m_cpos.x() + dx, m_cpos.y() + dy, m_cpos.z() + dz);
                auto chunk = world->find_chunk(cpos);

                if(chunk == nullptr || !registry_valid(*world, chunk->entity())) {
                    continue;
                }

                if(0 == dx && 1 == dy && 0 == dz) {
                    m_above = true;
                }

                auto& light_storage = chunk->light();
                auto light = light_storage.data();

                auto gx_begin = dx * chunk_size + LIGHT_PADDING;
                auto gy_begin = dy * chunk_size + LIGHT_PADDING;
                auto gz_begin = dz * chunk_size + LIGHT_PADDING;

                auto lx_begin = std::max(0, -gx_begin);
                auto ly_begin = std::max(0, -gy_begin);
                auto lz_begin = std::max(0, -gz_begin);

                auto lx_end = std::min(chunk_size, LIGHT_GRID_SIZE - gx_begin);
                auto ly_end = std::min(chunk_size, LIGHT_GRID_SIZE - gy_begin);
                auto lz_end = std::min(chunk_size, LIGHT_GRID_SIZE - gz_begin);

                if(lx_begin >= lx_end || ly_begin >= ly_end || lz_begin >= lz_end) {
                    continue;
                }

                for(int lz = lz_begin; lz < lz_end; lz += 1) {
                    for(int ly = ly_begin; ly < ly_end; ly += 1) {
                        for(int lx = lx_begin; lx < lx_end; lx += 1) {
                            auto grid = index(gx_begin + lx, gy_begin + ly, gz_begin + lz);
                            auto local = utils::to_index(LocalPos(lx, ly, lz));

                            auto id = chunk->get_block(local);

                            m_blocks[grid] = id;
                            m_light[grid] = light[local];

                            if(id && 0 == dx && 0 == dy && 0 == dz) {
                                m_empty = false;
                            }
                        }
                    }
                }
            }
        }
    }

    // Sunlight enters through the row above the chunk, so an empty chunk is
    // uniformly lit only when that row is already at full level. At the top
    // of a realm the sky is open by definition.
    m_realm_top = realm_top(*world, m_cpos.y());
    m_open_sky = m_realm_top;

    if(!m_open_sky) {
        m_open_sky = true;

        for(int z = 0; z < chunk_size; ++z) {
            for(int x = 0; x < chunk_size; ++x) {
                auto cell = m_light[grid_index(LIGHT_PADDING + x, LIGHT_PADDING + chunk_size, LIGHT_PADDING + z)];

                if(channel_level(cell, LIGHT_SKY) != BLOCK_LIGHT_MAX) {
                    m_open_sky = false;
                }
            }
        }
    }
}

ChunkLightTask::~ChunkLightTask(void)
{
    if(m_generation == s_generation) {
        s_in_flight.erase(m_cpos);
    }
}

std::size_t ChunkLightTask::index(int x, int y, int z) const
{
    return grid_index(x, y, z);
}

void ChunkLightTask::process(void)
{
    ZoneScopedN("lighting::process");

    constexpr auto chunk_size = static_cast<int>(constant::CHUNK_SIZE);

    auto& blocks = m_blocks;
    auto& light = m_light;

    // Sunlight enters the chunk through the row above it. That row is the
    // bottom of the chunk above, or open sky at the top of a realm. A
    // missing above chunk can only happen if it unloaded after the task was
    // submitted; treating it as open sky keeps the chunk from going black.
    auto seed_sky = !m_realm_top && m_above;

    for(int z = 0; z < chunk_size; ++z) {
        for(int x = 0; x < chunk_size; ++x) {
            auto gx = LIGHT_PADDING + x;
            auto gz = LIGHT_PADDING + z;
            block_light_type level = BLOCK_LIGHT_MAX;

            if(seed_sky) {
                level = static_cast<block_light_type>(light[index(gx, LIGHT_PADDING + chunk_size, gz)] >> 4U);
            }

            // The column pass enters every cell through its top face and leaves
            // it through the bottom one, so a half block closes exactly one of
            // the two and the sun stops on the side its solid part is on.
            auto above = blocks[index(gx, LIGHT_PADDING + chunk_size, gz)];

            for(int y = chunk_size - 1; y >= 0; y -= 1) {
                auto cell = index(gx, LIGHT_PADDING + y, gz);
                auto block = blocks[cell];
                auto cost = light_material::crossing_cost(above, block, BLOCK_FACE_TOP);

                level = lighting::step_level(level, cost, LIGHT_SKY, true);
                light[cell] = static_cast<std::uint8_t>((light[cell] & 0x0FU) | (level << 4U));

                above = block;
            }
        }
    }

    // Block light comes from the emitters inside the chunk and from the
    // neighbours' border cells that were copied into the padding.
    for(int y = 0; y < chunk_size; ++y) {
        for(int z = 0; z < chunk_size; ++z) {
            for(int x = 0; x < chunk_size; ++x) {
                auto cell = index(LIGHT_PADDING + x, LIGHT_PADDING + y, LIGHT_PADDING + z);
                auto emission = light_material::get(blocks[cell]).emission;

                light[cell] = static_cast<std::uint8_t>((light[cell] & 0xF0U) | emission);
            }
        }
    }

    std::array<std::deque<LocalLightNode>, 16> add;

    // An empty chunk under an open sky is lit to the last cell, and block
    // light can only enter it from the padding. When neither applies the
    // whole fill is redundant.
    if(m_empty && m_open_sky) {
        bool any_block_light = false;

        for(auto packed : light) {
            if(packed & 0x0FU) {
                any_block_light = true;
                break;
            }
        }

        if(!any_block_light) {
            m_result.fill(0xF0);
            m_ready = true;
            return;
        }
    }

    for(int y = 0; y < LIGHT_GRID_SIZE; ++y) {
        for(int z = 0; z < LIGHT_GRID_SIZE; ++z) {
            for(int x = 0; x < LIGHT_GRID_SIZE; ++x) {
                auto sky = channel_level(light[index(x, y, z)], LIGHT_SKY);
                auto block = channel_level(light[index(x, y, z)], LIGHT_BLOCK);

                if(sky && needs_seed(light, x, y, z, LIGHT_SKY)) {
                    add[sky].push_back(LocalLightNode {
                        static_cast<std::int16_t>(x), static_cast<std::int16_t>(y), static_cast<std::int16_t>(z), sky, LIGHT_SKY });
                }

                if(block && needs_seed(light, x, y, z, LIGHT_BLOCK)) {
                    add[block].push_back(LocalLightNode {
                        static_cast<std::int16_t>(x), static_cast<std::int16_t>(y), static_cast<std::int16_t>(z), block, LIGHT_BLOCK });
                }
            }
        }
    }

    for(int level = static_cast<int>(BLOCK_LIGHT_MAX); level > 0; level -= 1) {
        auto& bucket = add[static_cast<std::size_t>(level)];

        while(!bucket.empty()) {
            auto node = bucket.front();

            bucket.pop_front();

            auto cell = index(node.x, node.y, node.z);
            auto current = (node.channel == LIGHT_SKY)
                ? static_cast<block_light_type>(light[cell] >> 4U)
                : static_cast<block_light_type>(light[cell] & 0x0FU);

            if(current != node.level) {
                continue;
            }

            for(const auto& offset : NEIGHBOURS) {
                auto nx = static_cast<int>(node.x) + offset[0];
                auto ny = static_cast<int>(node.y) + offset[1];
                auto nz = static_cast<int>(node.z) + offset[2];

                if(nx < 0 || nx >= LIGHT_GRID_SIZE || ny < 0 || ny >= LIGHT_GRID_SIZE || nz < 0 || nz >= LIGHT_GRID_SIZE) {
                    continue;
                }

                auto next_cell = index(nx, ny, nz);
                auto cost = light_material::crossing_cost(blocks[cell], blocks[next_cell], enter_face(offset));
                auto next = lighting::step_level(node.level, cost, node.channel, offset[1] < 0);

                if(next == BLOCK_LIGHT_MIN) {
                    continue;
                }

                auto target = (node.channel == LIGHT_SKY)
                    ? static_cast<block_light_type>(light[next_cell] >> 4U)
                    : static_cast<block_light_type>(light[next_cell] & 0x0FU);

                if(next <= target) {
                    continue;
                }

                if(node.channel == LIGHT_SKY) {
                    light[next_cell] = static_cast<std::uint8_t>((light[next_cell] & 0x0FU) | (next << 4U));
                }
                else {
                    light[next_cell] = static_cast<std::uint8_t>((light[next_cell] & 0xF0U) | next);
                }

                add[next].push_back(LocalLightNode {
                    static_cast<std::int16_t>(nx), static_cast<std::int16_t>(ny), static_cast<std::int16_t>(nz), next, node.channel });
            }
        }
    }

    m_result.fill(0x00);

    for(int y = 0; y < chunk_size; ++y) {
        for(int z = 0; z < chunk_size; ++z) {
            for(int x = 0; x < chunk_size; ++x) {
                auto grid = index(LIGHT_PADDING + x, LIGHT_PADDING + y, LIGHT_PADDING + z);
                m_result.set_packed(utils::to_index(LocalPos(x, y, z)), light[grid]);
            }
        }
    }

    m_ready = true;
}

void ChunkLightTask::finalize(void)
{
    if(!m_ready || m_generation != s_generation || s_replica) {
        return;
    }

    auto chunk = m_world->find_chunk(m_cpos);

    if(chunk == nullptr || !registry_valid(*m_world, chunk->entity())) {
        return;
    }

    if(!chunk->has_light() || !same_light(chunk->light(), m_result)) {
        chunk->set_light(std::move(m_result));

        mark_full_light_change(m_cpos);

        globals::dispatcher.trigger(ChunkLightUpdateEvent(m_world, m_cpos));

        // A changed border is a seed for the six chunks that share a face
        // with this one; the diagonal chunks are reached through them.
        mark_face_neighbors_dirty(m_cpos);
    }
}

static bool node_alive(const World& world, const LightNode& node)
{
    auto& chunks = world.chunks();
    auto it = chunks.find(node.cpos);
    return it != chunks.cend() && it->second.get() == node.chunk.get();
}

static void push_add(const std::shared_ptr<Chunk>& chunk, const ChunkPos& cpos, std::size_t index, block_light_type level,
    light_channel channel)
{
    if(level == BLOCK_LIGHT_MIN || s_queued >= LOCAL_MAX_QUEUED) {
        return;
    }

    auto clamped = std::min<block_light_type>(level, BLOCK_LIGHT_MAX);

    s_add[clamped].push_back(LightNode { chunk, cpos, static_cast<std::uint16_t>(index), clamped, channel });
    s_queued += 1;
}

static void push_remove(const std::shared_ptr<Chunk>& chunk, const ChunkPos& cpos, std::size_t index, block_light_type level,
    light_channel channel)
{
    if(level == BLOCK_LIGHT_MIN || s_queued >= LOCAL_MAX_QUEUED) {
        return;
    }

    s_remove.push_back(LightNode { chunk, cpos, static_cast<std::uint16_t>(index), level, channel });
    s_queued += 1;
}

static std::optional<NeighbourCell> find_neighbour(World& world, const std::shared_ptr<Chunk>& chunk, const ChunkPos& cpos,
    std::size_t index, const std::array<int, 4>& offset)
{
    auto lpos = utils::to_local(index);
    auto shifted = LocalPos(lpos.x() + offset[0], lpos.y() + offset[1], lpos.z() + offset[2]);

    auto bpos = utils::to_block(cpos, shifted);
    auto ncpos = utils::to_chunk(bpos);
    auto nlpos = utils::to_local(bpos);

    std::shared_ptr<Chunk> neighbour;

    if(ncpos == cpos) {
        neighbour = chunk;
    }
    else {
        neighbour = world.find_chunk(ncpos);
    }

    if(neighbour == nullptr || !registry_valid(world, neighbour->entity())) {
        return std::nullopt;
    }

    NeighbourCell result;
    result.chunk = std::move(neighbour);
    result.cpos = ncpos;
    result.index = static_cast<std::uint16_t>(utils::to_index(nlpos));
    result.downward = offset[1] < 0;
    return result;
}

static void write_light(const std::shared_ptr<Chunk>& chunk, const ChunkPos& cpos, std::size_t index, light_channel channel,
    block_light_type value)
{
    if(chunk->get_light(index, channel) == value) {
        return;
    }

    chunk->set_light(index, channel, value);
    s_changed.insert(cpos);
    record_light_change(cpos, index);
}

static void seed_cell(const std::shared_ptr<Chunk>& chunk, const ChunkPos& cpos, std::size_t index)
{
    push_add(chunk, cpos, index, chunk->get_light(index, LIGHT_SKY), LIGHT_SKY);
    push_add(chunk, cpos, index, chunk->get_light(index, LIGHT_BLOCK), LIGHT_BLOCK);
}

static void relight_position(World& world, const BlockPos& bpos)
{
    auto cpos = utils::to_chunk(bpos);
    auto chunk = world.find_chunk(cpos);

    if(chunk == nullptr || !registry_valid(world, chunk->entity())) {
        return;
    }

    if(!chunk->has_light()) {
        if(s_replica) {
            // Without a base light the client cannot predict this edit;
            // the server packet will bring the whole chunk.
            return;
        }

        // The chunk has not been lit yet. Do not invent an all-zero array and
        // call it valid: it would be sent to clients as real light, and every
        // chunk below would take its zero row as the sky above it. Keeping
        // the light invalid makes the client fall back to its own estimate
        // until the fill below delivers the real thing.
        chunk->clear_light();
        mark_full_light_change(cpos);
        s_changed.insert(cpos);
        mark_dirty(cpos);
    }

    auto index = utils::to_index(utils::to_local(bpos));
    auto old_sky = chunk->get_light(index, LIGHT_SKY);
    auto old_block = chunk->get_light(index, LIGHT_BLOCK);
    auto emission = light_material::get(chunk->get_block(index)).emission;

    write_light(chunk, cpos, index, LIGHT_SKY, BLOCK_LIGHT_MIN);
    write_light(chunk, cpos, index, LIGHT_BLOCK, emission);

    if(old_sky != BLOCK_LIGHT_MIN) {
        push_remove(chunk, cpos, index, old_sky, LIGHT_SKY);
    }

    if(old_block > emission) {
        push_remove(chunk, cpos, index, old_block, LIGHT_BLOCK);
    }

    if(emission != BLOCK_LIGHT_MIN) {
        push_add(chunk, cpos, index, emission, LIGHT_BLOCK);
    }

    if(!s_replica) {
        // A task that is already running read its snapshot before this edit.
        for(ChunkPos::value_type dz = -1; dz <= 1; dz += 1) {
            for(ChunkPos::value_type dy = -1; dy <= 1; dy += 1) {
                for(ChunkPos::value_type dx = -1; dx <= 1; dx += 1) {
                    auto pos = cpos + ChunkPos(dx, dy, dz);

                    if(s_in_flight.contains(pos)) {
                        s_dirty.insert(pos);
                    }
                }
            }
        }
    }

    for(const auto& offset : NEIGHBOURS) {
        auto cell = find_neighbour(world, chunk, cpos, index, offset);

        if(!cell.has_value()) {
            continue;
        }

        seed_cell(cell->chunk, cell->cpos, cell->index);
    }
}

static void process_remove(World& world, std::size_t& budget)
{
    while(!s_remove.empty()) {
        if(budget == 0) {
            return;
        }

        auto node = std::move(s_remove.front());

        s_remove.pop_front();
        s_queued -= 1;
        budget -= 1;

        if(!node_alive(world, node)) {
            continue;
        }

        for(const auto& offset : NEIGHBOURS) {
            auto cell = find_neighbour(world, node.chunk, node.cpos, node.index, offset);

            if(!cell.has_value()) {
                continue;
            }

            auto current = cell->chunk->get_light(cell->index, node.channel);

            if(current == BLOCK_LIGHT_MIN) {
                continue;
            }

            auto from = node.chunk->get_block(node.index);
            auto to = cell->chunk->get_block(cell->index);
            auto cost = light_material::crossing_cost(from, to, enter_face(offset));
            auto expected = lighting::step_level(node.level, cost, node.channel, cell->downward);

            if(current > expected) {
                push_add(cell->chunk, cell->cpos, cell->index, current, node.channel);
            }
            else {
                write_light(cell->chunk, cell->cpos, cell->index, node.channel, BLOCK_LIGHT_MIN);
                push_remove(cell->chunk, cell->cpos, cell->index, current, node.channel);
            }
        }
    }
}

static void process_add(World& world, std::size_t& budget)
{
    for(auto level = static_cast<int>(BLOCK_LIGHT_MAX); level > 0; level -= 1) {
        auto& bucket = s_add[static_cast<std::size_t>(level)];

        while(!bucket.empty()) {
            if(budget == 0) {
                return;
            }

            auto node = std::move(bucket.front());

            bucket.pop_front();
            s_queued -= 1;
            budget -= 1;

            if(!node_alive(world, node)) {
                continue;
            }

            if(node.chunk->get_light(node.index, node.channel) != node.level) {
                continue;
            }

            for(const auto& offset : NEIGHBOURS) {
                auto cell = find_neighbour(world, node.chunk, node.cpos, node.index, offset);

                if(!cell.has_value()) {
                    continue;
                }

                auto from = node.chunk->get_block(node.index);
                auto to = cell->chunk->get_block(cell->index);
                auto cost = light_material::crossing_cost(from, to, enter_face(offset));
                auto next = lighting::step_level(node.level, cost, node.channel, cell->downward);

                if(next == BLOCK_LIGHT_MIN) {
                    continue;
                }

                if(next <= cell->chunk->get_light(cell->index, node.channel)) {
                    continue;
                }

                write_light(cell->chunk, cell->cpos, cell->index, node.channel, next);
                push_add(cell->chunk, cell->cpos, cell->index, next, node.channel);
            }
        }
    }
}

static void on_chunk_ready(const ChunkReadyEvent& event)
{
    mark_dirty(event.pos());
    mark_face_neighbors_dirty(event.pos());
}

static void on_chunk_update(const ChunkUpdateEvent& event)
{
    mark_dirty(event.pos());
    mark_face_neighbors_dirty(event.pos());
}

static void on_chunk_remove(const ChunkRemoveEvent& event)
{
    // The chunk is gone, so a queued task would find nothing. The neighbours
    // are not recomputed here: they are dirtied again when the chunk loads.
    s_dirty.erase(event.pos());
}

static void on_block_update(const BlockUpdateEvent& event)
{
    s_pending_relight.push_back(event.bpos());
}

void lighting::init(void)
{
    globals::dispatcher.sink<ChunkUpdateEvent>().connect<&on_chunk_update>();
    globals::dispatcher.sink<ChunkReadyEvent>().connect<&on_chunk_ready>();
    globals::dispatcher.sink<ChunkRemoveEvent>().connect<&on_chunk_remove>();
    globals::dispatcher.sink<BlockUpdateEvent>().connect<&on_block_update>();
}

void lighting::set_replica(bool value)
{
    s_replica = value;
}

void lighting::reset(void)
{
    s_generation += 1;

    s_dirty.clear();
    s_in_flight.clear();

    s_add = {};
    s_remove.clear();
    s_pending_relight.clear();
    s_changed.clear();
    s_change.clear();
    s_queued = 0;
}

void lighting::shutdown(void)
{
    reset();
}

lighting::LightChange lighting::take_change(const ChunkPos& pos)
{
    auto it = s_change.find(pos);

    if(it == s_change.cend()) {
        return LightChange {};
    }

    auto result = std::move(it->second);

    s_change.erase(it);

    if(result.full) {
        return result;
    }

    std::sort(result.indices.begin(), result.indices.end());
    result.indices.erase(std::unique(result.indices.begin(), result.indices.end()), result.indices.end());

    if(result.indices.size() > MAX_CHANGE_INDICES) {
        result.full = true;
        result.indices.clear();
    }

    return result;
}

void lighting::fixed_update(World& world)
{
    ZoneScoped;

    // Block edits are processed first: they are the visible, interactive
    // change and must not wait behind chunk snapshot copies.
    if(!s_pending_relight.empty()) {
        auto positions = std::move(s_pending_relight);

        s_pending_relight.clear();

        for(const auto& bpos : positions) {
            relight_position(world, bpos);
        }
    }

    auto budget = LOCAL_BUDGET;

    process_remove(world, budget);
    process_add(world, budget);

    for(const auto& cpos : s_changed) {
        globals::dispatcher.trigger(ChunkLightUpdateEvent(&world, cpos));
    }

    s_changed.clear();
    s_change.clear();

    if(s_replica || s_dirty.empty()) {
        return;
    }

    std::vector<std::pair<std::int64_t, ChunkPos>> ordered;
    ordered.reserve(s_dirty.size());

    for(const auto& pos : s_dirty) {
        if(world.find_chunk(pos) == nullptr) {
            continue;
        }

        ordered.emplace_back(chunk_distance(world, pos), pos);
    }

    s_dirty.clear();

    std::sort(ordered.begin(), ordered.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });

    std::size_t submitted = 0;

    for(const auto& entry : ordered) {
        auto pos = entry.second;

        if(submitted >= LIGHT_TASKS_PER_TICK || s_in_flight.contains(pos) || !sky_ready(world, pos)) {
            // Not this tick: the budget is spent, a task is already running,
            // or the chunk above has not been lit yet.
            s_dirty.insert(pos);
            continue;
        }

        s_in_flight.insert(pos);
        threading::submit<ChunkLightTask>(&world, pos);
        submitted += 1;
    }
}
