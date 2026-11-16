#include "shared/pch.hh"

#include "shared/world/light_storage.hh"

#include "core/buffer.hh"

// Sky light is mostly full and block light mostly empty, so the array shrinks
// by an order of magnitude. Whole arrays go through the buffer compressor, so
// it costs less than the blocks do once those are packed into a palette.
void LightStorage::encode_net(const LightStorage& storage, WriteBuffer& buffer)
{
    buffer.write_compressed(std::as_bytes(std::span(storage.m_cells)));
}

void LightStorage::decode_net(LightStorage& storage, ReadBuffer& buffer)
{
    storage.fill(0x00);

    auto raw = buffer.read_compressed();

    if(raw.size() != constant::CHUNK_VOLUME) {
        return;
    }

    std::memcpy(storage.m_cells.data(), raw.data(), constant::CHUNK_VOLUME);
}

void LightStorage::encode_dat(const LightStorage& storage, WriteBuffer& buffer)
{
    buffer.write_bytes(std::as_bytes(std::span(storage.m_cells)));
}

void LightStorage::decode_dat(LightStorage& storage, ReadBuffer& buffer)
{
    storage.fill(0x00);
    buffer.read_bytes(std::as_writable_bytes(std::span(storage.m_cells)));
}

block_light_type LightStorage::get(std::size_t index, light_channel channel) const
{
    if(index >= constant::CHUNK_VOLUME) {
        return BLOCK_LIGHT_MIN;
    }

    auto packed = m_cells[index];

    if(channel == LIGHT_SKY) {
        return static_cast<block_light_type>(packed >> 4U);
    }

    return static_cast<block_light_type>(packed & 0x0FU);
}

std::uint8_t LightStorage::get_packed(std::size_t index) const
{
    if(index >= constant::CHUNK_VOLUME) {
        return 0x00;
    }

    return m_cells[index];
}

void LightStorage::set(std::size_t index, light_channel channel, block_light_type value)
{
    if(index >= constant::CHUNK_VOLUME) {
        return;
    }

    auto clamped = std::min<block_light_type>(value, BLOCK_LIGHT_MAX);

    if(channel == LIGHT_SKY) {
        m_cells[index] = static_cast<std::uint8_t>((m_cells[index] & 0x0FU) | (clamped << 4U));
    }
    else {
        m_cells[index] = static_cast<std::uint8_t>((m_cells[index] & 0xF0U) | clamped);
    }
}

void LightStorage::set_packed(std::size_t index, std::uint8_t value)
{
    if(index >= constant::CHUNK_VOLUME) {
        return;
    }

    m_cells[index] = value;
}

void LightStorage::fill(std::uint8_t packed)
{
    m_cells.fill(packed);
}
