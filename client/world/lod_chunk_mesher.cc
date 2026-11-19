#include "client/pch.hh"

#include "client/world/lod_chunk_mesher.hh"

#include "core/threading.hh"

#include "shared/constant.hh"
#include "shared/utils/coord.hh"
#include "shared/world/chunk_lod.hh"
#include "shared/world/lod.hh"

#include "client/camera.hh"
#include "client/world/lod_manager.hh"
#include "client/world/lod_quad.hh"

constexpr static std::size_t MAX_SUBMIT_PER_FRAME = 64;
constexpr static std::uint8_t LEVEL_MIN = 0;
constexpr static std::uint8_t LEVEL_MAX = 4;

static std::int32_t floor_log2(std::int32_t value)
{
    auto unsigned_value = static_cast<unsigned>(std::max(value, 1));
    return static_cast<std::int32_t>(std::bit_width(unsigned_value)) - 1;
}

static std::int32_t iabs(std::int32_t value)
{
    return value < 0 ? -value : value;
}

static std::uint8_t choose_level(const ChunkPos& cpos, const ChunkPos& camera, unsigned view_distance)
{
    auto distance = std::max({ iabs(cpos.x() - camera.x()), iabs(cpos.y() - camera.y()), iabs(cpos.z() - camera.z()) });
    auto start = static_cast<std::int32_t>(std::max(view_distance, 1U));
    auto level = floor_log2(std::max(distance / start, 1));
    return static_cast<std::uint8_t>(std::clamp<std::int32_t>(level, LEVEL_MIN, LEVEL_MAX));
}

static std::uint32_t pack_color(const ChunkLod& lod, const ChunkLodCell& cell)
{
    auto rgb = lod::to_rgb8(lod::unpack(lod.color_mode, lod.palette, cell.color));

    std::uint32_t color = 0;
    color |= static_cast<std::uint32_t>(rgb[0]);
    color |= static_cast<std::uint32_t>(rgb[1]) << 8U;
    color |= static_cast<std::uint32_t>(rgb[2]) << 16U;

    if(cell.is_water()) {
        color |= LOD_QUAD_WATER_BIT;
    }

    return color;
}

static bool cell_is_solid(const ChunkLod& lod, std::uint8_t level, std::int32_t dim, std::int32_t x, std::int32_t y, std::int32_t z)
{
    if(x < 0 || y < 0 || z < 0 || x >= dim || y >= dim || z >= dim) {
        return false;
    }

    auto cells = lod.cells(level);
    return cells[static_cast<std::size_t>((y * dim + z) * dim + x)].is_solid();
}

class LodChunkMeshingTask final : public Task {
public:
    explicit LodChunkMeshingTask(const ChunkPos& cpos, std::uint8_t level, std::shared_ptr<const ChunkLod> lod,
        std::array<std::shared_ptr<const ChunkLod>, 6> neighbors, std::array<bool, 6> neighbor_covers, std::uint32_t generation);
    virtual ~LodChunkMeshingTask(void) override = default;
    virtual void process(void) override;
    virtual void finalize(void) override;

private:
    bool neighbor_is_solid(std::size_t face, std::int32_t dim, std::int32_t x, std::int32_t y, std::int32_t z) const;

    ChunkPos m_cpos;
    std::array<std::shared_ptr<const ChunkLod>, 6> m_neighbors;
    std::array<bool, 6> m_neighbor_covers;
    std::shared_ptr<const ChunkLod> m_lod;
    std::vector<LodChunkQuad> m_instances;
    std::uint32_t m_generation;
    std::uint8_t m_level;
};

static vx::hash_set<ChunkPos> s_pending;
static std::size_t s_backlog;

LodChunkMeshingTask::LodChunkMeshingTask(const ChunkPos& cpos, std::uint8_t level, std::shared_ptr<const ChunkLod> lod,
    std::array<std::shared_ptr<const ChunkLod>, 6> neighbors, std::array<bool, 6> neighbor_covers, std::uint32_t generation)
    : m_cpos(cpos), m_neighbors(std::move(neighbors)), m_neighbor_covers(neighbor_covers), m_lod(std::move(lod)), m_generation(generation),
      m_level(level)
{
    // empty
}

bool LodChunkMeshingTask::neighbor_is_solid(std::size_t face, std::int32_t dim, std::int32_t x, std::int32_t y, std::int32_t z) const
{
    // A face neighbor is the only thing a border cell can look at, so exactly
    // one coordinate is out of range here and the rest map unchanged.
    const auto& neighbor = m_neighbors[face];

    if(!m_neighbor_covers[face] || neighbor == nullptr) {
        // The neighbor is missing, hidden or meshed at another level, so its
        // geometry may not meet this border exactly. Draw the face: a doubled
        // wall is cheaper than a crack the sky shows through.
        return false;
    }

    switch(face) {
        case 0:
            x = dim - 1;
            break;

        case 1:
            x = 0;
            break;

        case 2:
            y = dim - 1;
            break;

        case 3:
            y = 0;
            break;

        case 4:
            z = dim - 1;
            break;

        case 5:
            z = 0;
            break;

        default:
            break;
    }

    return cell_is_solid(*neighbor, m_level, dim, x, y, z);
}

static void neighbor_coords(std::size_t face, std::int32_t x, std::int32_t y, std::int32_t z, std::int32_t& nx, std::int32_t& ny,
    std::int32_t& nz)
{
    nx = x;
    ny = y;
    nz = z;

    switch(face) {
        case 0:
            nx -= 1;
            break;

        case 1:
            nx += 1;
            break;

        case 2:
            ny -= 1;
            break;

        case 3:
            ny += 1;
            break;

        case 4:
            nz -= 1;
            break;

        case 5:
            nz += 1;
            break;

        default:
            break;
    }
}

void LodChunkMeshingTask::process(void)
{
    ZoneScopedN("lod_chunk_mesher::process");

    auto dim = static_cast<std::int32_t>(constant::CHUNK_SIZE) >> m_level;
    auto cells = m_lod->cells(m_level);
    auto origin = utils::to_fvec(m_cpos);
    auto origin_x = static_cast<std::int32_t>(origin.x());
    auto origin_y = static_cast<std::int32_t>(origin.y());
    auto origin_z = static_cast<std::int32_t>(origin.z());

    m_instances.clear();
    m_instances.reserve(static_cast<std::size_t>(dim) * dim * dim / 2);

    for(std::int32_t y = 0; y < dim; ++y) {
        for(std::int32_t z = 0; z < dim; ++z) {
            for(std::int32_t x = 0; x < dim; ++x) {
                const auto& cell = cells[static_cast<std::size_t>((y * dim + z) * dim + x)];

                if(!cell.is_solid()) {
                    continue;
                }

                auto color = pack_color(*m_lod, cell);

                for(std::size_t face = 0; face < 6; face += 1) {
                    std::int32_t nx;
                    std::int32_t ny;
                    std::int32_t nz;

                    neighbor_coords(face, x, y, z, nx, ny, nz);

                    auto outside_x = nx < 0 || nx >= dim;
                    auto outside_y = ny < 0 || ny >= dim;
                    auto outside_z = nz < 0 || nz >= dim;
                    auto outside = outside_x || outside_y || outside_z;
                    auto solid = false;

                    if(outside) {
                        solid = neighbor_is_solid(face, dim, nx, ny, nz);
                    }
                    else {
                        solid = cell_is_solid(*m_lod, m_level, dim, nx, ny, nz);
                    }

                    if(solid) {
                        continue;
                    }

                    LodChunkQuad quad {};
                    quad.data_cell = 0;
                    quad.data_cell |= static_cast<std::uint32_t>(x);
                    quad.data_cell |= static_cast<std::uint32_t>(y) << 4U;
                    quad.data_cell |= static_cast<std::uint32_t>(z) << 8U;
                    quad.data_cell |= static_cast<std::uint32_t>(face) << 12U;
                    quad.data_color = color;
                    quad.origin_x = origin_x;
                    quad.origin_y = origin_y;
                    quad.origin_z = origin_z;

                    m_instances.push_back(quad);
                }
            }
        }
    }
}

void LodChunkMeshingTask::finalize(void)
{
    ZoneScopedN("lod_chunk_mesher::finalize");

    auto& chunks = lod_manager::chunks();
    auto it = chunks.find(m_cpos);

    if(it != chunks.end() && it->second.generation == m_generation) {
        it->second.instances = std::move(m_instances);
        it->second.level = m_level;
        it->second.dirty = false;

        lod_manager::notify_mesh_changed();
    }

    s_pending.erase(m_cpos);
}

void lod_chunk_mesher::init(void)
{
    // empty
}

void lod_chunk_mesher::shutdown(void)
{
    s_pending.clear();
    s_backlog = 0;
}

void lod_chunk_mesher::update(void)
{
    static std::vector<std::tuple<ChunkPos, float, std::uint8_t>> batch;

    ZoneScoped;

    TracyPlot("LOD mesh queue", static_cast<int64_t>(s_pending.size()));

    auto& chunks = lod_manager::chunks();

    batch.clear();

    const std::array<ChunkPos, 6> deltas = {
        ChunkPos(-1, 0, 0),
        ChunkPos(1, 0, 0),
        ChunkPos(0, -1, 0),
        ChunkPos(0, 1, 0),
        ChunkPos(0, 0, -1),
        ChunkPos(0, 0, 1),
    };

    for(auto& [cpos, entry] : chunks) {
        if(entry.data == nullptr) {
            continue;
        }

        auto desired = choose_level(cpos, camera::chunk, camera::view_distance.value());

        if(entry.level != LOD_CHUNK_LEVEL_NONE && entry.level != desired) {
            entry.dirty = true;

            // The border faces of the neighbours were culled against this
            // chunk at the old level; mesh them again so the surfaces meet.
            for(const auto& delta : deltas) {
                auto neighbor = chunks.find(cpos + delta);

                if(neighbor != chunks.end()) {
                    neighbor->second.dirty = true;
                }
            }
        }

        if(entry.dirty && 0 == s_pending.count(cpos)) {
            batch.emplace_back(cpos, static_cast<float>((cpos - camera::chunk).squaredNorm()), desired);
        }
    }

    s_backlog = batch.size() + s_pending.size();

    auto submit_count = std::min(batch.size(), MAX_SUBMIT_PER_FRAME);

    std::partial_sort(batch.begin(), batch.begin() + static_cast<std::ptrdiff_t>(submit_count), batch.end(),
        [](const auto& a, const auto& b) {
            return std::get<1>(a) < std::get<1>(b);
        });

    for(std::size_t i = 0; i < submit_count; ++i) {
        const auto& item = batch[i];
        const auto& cpos = std::get<0>(item);
        auto level = std::get<2>(item);

        auto it = chunks.find(cpos);

        if(it == chunks.end() || it->second.data == nullptr) {
            continue;
        }

        std::array<std::shared_ptr<const ChunkLod>, 6> neighbors;
        std::array<bool, 6> covers {};

        for(std::size_t n = 0; n < neighbors.size(); ++n) {
            auto neighbor = chunks.find(cpos + deltas[n]);

            if(neighbor == chunks.end()) {
                continue;
            }

            // Only a neighbour that will actually draw at this level can hide
            // a border face. Anything else gets the face.
            if(!neighbor->second.visible || neighbor->second.level != level) {
                continue;
            }

            neighbors[n] = neighbor->second.data;
            covers[n] = true;
        }

        s_pending.insert(cpos);
        threading::submit<LodChunkMeshingTask>(cpos, level, it->second.data, std::move(neighbors), covers, it->second.generation);
    }
}

std::size_t lod_chunk_mesher::backlog(void)
{
    return s_backlog;
}
