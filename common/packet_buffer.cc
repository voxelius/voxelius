// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/packet_buffer.hh>
#include <mathlib/constexpr.hh>
#include <mathlib/floathacks.hh>

float PacketBuffer::read_FP32(PacketBuffer &buffer)
{
    return floathacks::uint32_to_float(PacketBuffer::read_UI32(buffer));
}

std::int8_t PacketBuffer::read_I8(PacketBuffer &buffer)
{
    return static_cast<std::int8_t>(PacketBuffer::read_UI8(buffer));
}

std::int16_t PacketBuffer::read_I16(PacketBuffer &buffer)
{
    return static_cast<std::int16_t>(PacketBuffer::read_UI16(buffer));
}

std::int32_t PacketBuffer::read_I32(PacketBuffer &buffer)
{
    return static_cast<std::int32_t>(PacketBuffer::read_UI32(buffer));
}

std::int64_t PacketBuffer::read_I64(PacketBuffer &buffer)
{
    return static_cast<std::int64_t>(PacketBuffer::read_UI64(buffer));
}

std::uint8_t PacketBuffer::read_UI8(PacketBuffer &buffer)
{
    if((buffer.read_position + 1U) <= buffer.vector.size()) {
        std::uint8_t result = buffer.vector[buffer.read_position];
        buffer.read_position += 1U;
        return result;
    }

    buffer.read_position += 1U;
    return UINT8_C(0);
}

std::uint16_t PacketBuffer::read_UI16(PacketBuffer &buffer)
{
    if((buffer.read_position + 2U) <= buffer.vector.size()) {
        std::uint16_t result = UINT16_C(0x0000);
        result |= static_cast<std::uint16_t>(buffer.vector[buffer.read_position + 0U]) << 8U;
        result |= static_cast<std::uint16_t>(buffer.vector[buffer.read_position + 1U]) << 0U;
        buffer.read_position += 2U;
        return result;
    }

    buffer.read_position += 2U;
    return UINT16_C(0);
}

std::uint32_t PacketBuffer::read_UI32(PacketBuffer &buffer)
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
    return UINT32_C(0);
}

std::uint64_t PacketBuffer::read_UI64(PacketBuffer &buffer)
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
    return UINT64_C(0);
}

std::string PacketBuffer::read_string(PacketBuffer &buffer)
{
    std::size_t size = PacketBuffer::read_UI16(buffer);
    std::string result = std::string();

    for(std::size_t i = 0; i < size; ++i) {
        if(buffer.read_position < buffer.vector.size())
            result.push_back(buffer.vector[buffer.read_position]);
        buffer.read_position += 1U;
    }

    return std::move(result);
}

void PacketBuffer::write_FP32(PacketBuffer &buffer, float value)
{
    PacketBuffer::write_UI32(buffer, floathacks::float_to_uint32(value));
}

void PacketBuffer::write_I8(PacketBuffer &buffer, std::int8_t value)
{
    PacketBuffer::write_UI8(buffer, static_cast<std::uint8_t>(value));
}

void PacketBuffer::write_I16(PacketBuffer &buffer, std::int16_t value)
{
    PacketBuffer::write_UI16(buffer, static_cast<std::uint16_t>(value));
}

void PacketBuffer::write_I32(PacketBuffer &buffer, std::int32_t value)
{
    PacketBuffer::write_UI32(buffer, static_cast<std::uint32_t>(value));
}

void PacketBuffer::write_I64(PacketBuffer &buffer, std::int64_t value)
{
    PacketBuffer::write_UI64(buffer, static_cast<std::uint64_t>(value));
}

void PacketBuffer::write_UI8(PacketBuffer &buffer, std::uint8_t value)
{
    buffer.vector.push_back(value);
}

void PacketBuffer::write_UI16(PacketBuffer &buffer, std::uint16_t value)
{
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT16_C(0xFF00)) >> 8U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT16_C(0x00FF)) >> 0U));
}

void PacketBuffer::write_UI32(PacketBuffer &buffer, std::uint32_t value)
{
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0xFF000000)) >> 24U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x00FF0000)) >> 16U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x0000FF00)) >> 8U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x000000FF)) >> 0U));
}

void PacketBuffer::write_UI64(PacketBuffer &buffer, std::uint64_t value)
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

void PacketBuffer::write_string(PacketBuffer &buffer, const std::string &value)
{
    const std::size_t size = cxpr::min<std::size_t>(UINT16_MAX, value.size());
    PacketBuffer::write_UI16(buffer, static_cast<std::uint16_t>(size));
    for(std::size_t i = 0; i < size; PacketBuffer::write_UI8(buffer, value[i++]));
}

void PacketBuffer::setup(PacketBuffer &buffer)
{
    buffer.read_position = 0;
    buffer.vector.clear();
}

void PacketBuffer::setup(PacketBuffer &buffer, const void *data, std::size_t size)
{
    buffer.read_position = 0;
    const std::uint8_t *data_p = reinterpret_cast<const std::uint8_t *>(data);
    buffer.vector.assign(data_p, data_p + size);
}
