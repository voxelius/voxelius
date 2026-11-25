#ifndef A76309E5_0293_4EA9_95FF_6538BF068CED
#define A76309E5_0293_4EA9_95FF_6538BF068CED

#include "shared/constant.hh"
#include "shared/coord.hh"
#include "shared/world/block.hh"

class ReadBuffer;
class WriteBuffer;

class BlockStorage final {
public:
    static void encode_net(const BlockStorage& storage, WriteBuffer& buffer);
    static void decode_net(BlockStorage& storage, ReadBuffer& buffer);

    static void encode_dat(const BlockStorage& storage, WriteBuffer& buffer);
    static void decode_dat(BlockStorage& storage, ReadBuffer& buffer);

    block_id_type get(std::size_t index) const;
    block_id_type get(const LocalPos& pos) const;

    void flatten(std::span<block_id_type> out) const;

    void set(std::size_t index, block_id_type id);
    void set(const LocalPos& pos, block_id_type id);
    void fill(block_id_type id);

    void optimize(void);

    std::size_t size(void) const;

private:
    using array8_type = std::array<std::uint8_t, constant::CHUNK_VOLUME>;
    using array16_type = std::array<std::uint16_t, constant::CHUNK_VOLUME>;
    using palette_type = std::vector<std::pair<block_id_type, std::uint16_t>>;

    struct Uniform final {
        block_id_type filler;
    };

    struct Palette8 final {
        array8_type indices;
        palette_type palette;
    };

    struct Palette16 final {
        array16_type indices;
        palette_type palette;
    };

    using variant_type = std::variant<Uniform, Palette8, Palette16>;

    static void encode_net(const Uniform* uniform, WriteBuffer& buffer);
    static void encode_net(const Palette8* p8, WriteBuffer& buffer);
    static void encode_net(const Palette16* p16, WriteBuffer& buffer);

    static void decode_net(Uniform& uniform, ReadBuffer& buffer);
    static void decode_net(Palette8& p8, ReadBuffer& buffer);
    static void decode_net(Palette16& p16, ReadBuffer& buffer);

    static std::optional<std::size_t> find_slot(const palette_type& palette, block_id_type id);
    static std::size_t add_slot(palette_type& palette, block_id_type id);

    static void promote(Palette8& dest, const Uniform* src);
    static void promote(Palette16& dest, const Palette8* src);

    static void optimize(Palette8* p8, variant_type& dest);
    static void optimize(Palette16* p16, variant_type& dest);

private:
    variant_type m_variant { Uniform { BLOCK_ID_NULL } };
};

#endif /* A76309E5_0293_4EA9_95FF_6538BF068CED */
