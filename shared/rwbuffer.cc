// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/math/floathacks.hh>
#include <shared/rwbuffer.hh>

float RWBuffer::read_FP32(RWBuffer &buffer)
{
    return floathacks::uint32_to_float(RWBuffer::read_UI32(buffer));
}

std::int8_t RWBuffer::read_I8(RWBuffer &buffer)
{
    return static_cast<std::int8_t>(RWBuffer::read_UI8(buffer));
}

std::int16_t RWBuffer::read_I16(RWBuffer &buffer)
{
    return static_cast<std::int16_t>(RWBuffer::read_UI16(buffer));
}

std::int32_t RWBuffer::read_I32(RWBuffer &buffer)
{
    return static_cast<std::int32_t>(RWBuffer::read_UI32(buffer));
}

std::int64_t RWBuffer::read_I64(RWBuffer &buffer)
{
    return static_cast<std::int64_t>(RWBuffer::read_UI64(buffer));
}

std::uint8_t RWBuffer::read_UI8(RWBuffer &buffer)
{
    if((buffer.read_position + 1U) <= buffer.vector.size()) {
        std::uint8_t result = buffer.vector[buffer.read_position];
        buffer.read_position += 1U;
        return result;
    }

    buffer.read_position += 1U;
    return UINT8_MAX;
}

std::uint16_t RWBuffer::read_UI16(RWBuffer &buffer)
{
    if((buffer.read_position + 2U) <= buffer.vector.size()) {
        std::uint16_t result = UINT16_C(0x0000);
        result |= static_cast<std::uint16_t>(buffer.vector[buffer.read_position + 0U]) << 8U;
        result |= static_cast<std::uint16_t>(buffer.vector[buffer.read_position + 1U]) << 0U;
        buffer.read_position += 2U;
        return result;
    }

    buffer.read_position += 2U;
    return UINT16_MAX;
}

std::uint32_t RWBuffer::read_UI32(RWBuffer &buffer)
{
    if((buffer.read_position + 4U) <= buffer.vector.size()) {
        std::uint32_t result = UINT32_C(0x00000000);
        result |= static_cast<std::uint32_t>(buffer.vector[buffer.read_position + 0U]) << 24U;
        result |= static_cast<std::uint32_t>(buffer.vector[buffer.read_position + 1U]) << 16U;
        result |= static_cast<std::uint32_t>(buffer.vector[buffer.read_position + 2U]) << 8U;
        result |= static_cast<std::uint32_t>(buffer.vector[buffer.read_position + 3U]) << 0U;
        buffer.read_position += 4U;
        return result;
    }

    buffer.read_position += 4U;
    return UINT32_MAX;
}

std::uint64_t RWBuffer::read_UI64(RWBuffer &buffer)
{
    if((buffer.read_position + 8U) <= buffer.vector.size()) {
        std::uint64_t result = UINT64_C(0x0000000000000000);
        result |= static_cast<std::uint64_t>(buffer.vector[buffer.read_position + 0U]) << 56U;
        result |= static_cast<std::uint64_t>(buffer.vector[buffer.read_position + 1U]) << 48U;
        result |= static_cast<std::uint64_t>(buffer.vector[buffer.read_position + 2U]) << 40U;
        result |= static_cast<std::uint64_t>(buffer.vector[buffer.read_position + 3U]) << 32U;
        result |= static_cast<std::uint64_t>(buffer.vector[buffer.read_position + 4U]) << 24U;
        result |= static_cast<std::uint64_t>(buffer.vector[buffer.read_position + 5U]) << 16U;
        result |= static_cast<std::uint64_t>(buffer.vector[buffer.read_position + 6U]) << 8U;
        result |= static_cast<std::uint64_t>(buffer.vector[buffer.read_position + 7U]) << 0U;
        buffer.read_position += 8U;
        return result;
    }

    buffer.read_position += 8U;
    return UINT64_MAX;
}

std::string RWBuffer::read_string(RWBuffer &buffer)
{
    std::size_t size = RWBuffer::read_UI16(buffer);
    std::string result = std::string();

    for(std::size_t i = 0; i < size; ++i) {
        if(buffer.read_position < buffer.vector.size())
            result.push_back(buffer.vector[buffer.read_position]);
        buffer.read_position += 1U;
    }

    return std::move(result);
}

void RWBuffer::write_FP32(RWBuffer &buffer, float value)
{
    RWBuffer::write_UI32(buffer, floathacks::float_to_uint32(value));
}

void RWBuffer::write_I8(RWBuffer &buffer, std::int8_t value)
{
    RWBuffer::write_UI8(buffer, static_cast<std::uint8_t>(value));
}

void RWBuffer::write_I16(RWBuffer &buffer, std::int16_t value)
{
    RWBuffer::write_UI16(buffer, static_cast<std::uint16_t>(value));
}

void RWBuffer::write_I32(RWBuffer &buffer, std::int32_t value)
{
    RWBuffer::write_UI32(buffer, static_cast<std::uint32_t>(value));
}

void RWBuffer::write_I64(RWBuffer &buffer, std::int64_t value)
{
    RWBuffer::write_UI64(buffer, static_cast<std::uint64_t>(value));
}

void RWBuffer::write_UI8(RWBuffer &buffer, std::uint8_t value)
{
    buffer.vector.push_back(value);
}

void RWBuffer::write_UI16(RWBuffer &buffer, std::uint16_t value)
{
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT16_C(0xFF00)) >> 8U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT16_C(0x00FF)) >> 0U));
}

void RWBuffer::write_UI32(RWBuffer &buffer, std::uint32_t value)
{
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0xFF000000)) >> 24U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x00FF0000)) >> 16U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x0000FF00)) >> 8U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x000000FF)) >> 0U));
}

void RWBuffer::write_UI64(RWBuffer &buffer, std::uint64_t value)
{
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0xFF00000000000000)) >> 56U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x00FF000000000000)) >> 48U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x0000FF0000000000)) >> 40U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x000000FF00000000)) >> 32U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x00000000FF000000)) >> 24U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x0000000000FF0000)) >> 16U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x000000000000FF00)) >> 8U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x00000000000000FF)) >> 0U));
}

void RWBuffer::write_string(RWBuffer &buffer, const std::string &value)
{
    const std::size_t size = cxpr::min<std::size_t>(UINT16_MAX, value.size());
    RWBuffer::write_UI16(buffer, static_cast<std::uint16_t>(size));
    for(std::size_t i = 0; i < size; RWBuffer::write_UI8(buffer, value[i++]));
}

void RWBuffer::setup(RWBuffer &buffer)
{
    buffer.read_position = 0;
    buffer.vector.clear();
}

void RWBuffer::setup(RWBuffer &buffer, const void *data, std::size_t size)
{
    buffer.read_position = 0;
    const std::uint8_t *data_p = reinterpret_cast<const std::uint8_t *>(data);
    buffer.vector.assign(data_p, data_p + size);
}

ENetPacket *RWBuffer::submit(const RWBuffer &buffer, ENetPacketFlag flags)
{
    return enet_packet_create(buffer.vector.data(), buffer.vector.size(), flags);
}
