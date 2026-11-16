#ifndef C74D9A9B_567F_492D_871F_CDC652184709
#define C74D9A9B_567F_492D_871F_CDC652184709

#include "shared/constant.hh"
#include "shared/coord.hh"
#include "shared/world/block.hh"
#include "shared/world/light_storage.hh"

class BlockCache final {
public:
    constexpr static std::int16_t PADDING = 2;
    constexpr static std::int16_t CHUNK_SIZE_I16 = static_cast<std::int16_t>(constant::CHUNK_SIZE);

    constexpr static std::size_t SIZE = constant::CHUNK_SIZE + 2 * PADDING;
    constexpr static std::size_t VOLUME = SIZE * SIZE * SIZE;

    void init(const ChunkPos& cpos);

    block_id_type get(const LocalPos& lpos) const;
    block_light_type get_light(const LocalPos& lpos, light_channel channel) const;

private:
    std::array<block_id_type, VOLUME> m_blocks;
    std::array<std::uint8_t, VOLUME> m_light;
};

#endif /* C74D9A9B_567F_492D_871F_CDC652184709 */
