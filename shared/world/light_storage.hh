#ifndef EC1D2E3A_4B5C_4D6E_8F90_A1B2C3D4E5F6
#define EC1D2E3A_4B5C_4D6E_8F90_A1B2C3D4E5F6

#include "shared/constant.hh"
#include "shared/world/block.hh"

class ReadBuffer;
class WriteBuffer;

enum light_channel : unsigned {
    LIGHT_SKY = 0,
    LIGHT_BLOCK = 1,
};

class LightStorage final {
public:
    static void encode_net(const LightStorage& storage, WriteBuffer& buffer);
    static void decode_net(LightStorage& storage, ReadBuffer& buffer);

    static void encode_dat(const LightStorage& storage, WriteBuffer& buffer);
    static void decode_dat(LightStorage& storage, ReadBuffer& buffer);

    block_light_type get(std::size_t index, light_channel channel) const;
    std::uint8_t get_packed(std::size_t index) const;

    void set(std::size_t index, light_channel channel, block_light_type value);
    void set_packed(std::size_t index, std::uint8_t value);
    void fill(std::uint8_t packed);

    constexpr std::span<const std::uint8_t> data(void) const;

private:
    std::array<std::uint8_t, constant::CHUNK_VOLUME> m_cells {};
};

constexpr std::span<const std::uint8_t> LightStorage::data(void) const
{
    return m_cells;
}

#endif /* EC1D2E3A_4B5C_4D6E_8F90_A1B2C3D4E5F6 */
