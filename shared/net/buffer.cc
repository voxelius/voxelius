// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/math/constexpr.hh>
#include <shared/math/floathacks.hh>
#include <shared/net/buffer.hh>

void net::Buffer::setup(net::Buffer &buffer)
{
    buffer.read_position = 0;
    buffer.vector.clear();
}

void net::Buffer::setup(net::Buffer &buffer, const ENetPacket *source)
{
    buffer.read_position = 0;
    buffer.vector.assign(source->data, source->data + source->dataLength);
}

ENetPacket *net::Buffer::submit(const net::Buffer &buffer, ENetPacketFlag flags)
{
    return enet_packet_create(buffer.vector.data(), buffer.vector.size(), flags);
}

void net::Buffer::write_F32(net::Buffer &buffer, float value)
{
    net::Buffer::write_UI32(buffer, floathacks::float_to_uint32(value));
}

void net::Buffer::write_I8(net::Buffer &buffer, std::int8_t value)
{
    net::Buffer::write_UI8(buffer, static_cast<std::uint8_t>(value));
}

void net::Buffer::write_I16(net::Buffer &buffer, std::int16_t value)
{
    net::Buffer::write_UI16(buffer, static_cast<std::uint16_t>(value));
}

void net::Buffer::write_I32(net::Buffer &buffer, std::int32_t value)
{
    net::Buffer::write_UI32(buffer, static_cast<std::uint32_t>(value));
}

void net::Buffer::write_I64(net::Buffer &buffer, std::int64_t value)
{
    net::Buffer::write_UI64(buffer, static_cast<std::uint64_t>(value));
}

void net::Buffer::write_UI8(net::Buffer &buffer, std::uint8_t value)
{
    buffer.vector.push_back(value);
}

void net::Buffer::write_UI16(net::Buffer &buffer, std::uint16_t value)
{
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT16_C(0xFF00)) >> 8U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT16_C(0x00FF)) >> 0U));
}

void net::Buffer::write_UI32(net::Buffer &buffer, std::uint32_t value)
{
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0xFF000000)) >> 24U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x00FF0000)) >> 16U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x0000FF00)) >> 8U));
    buffer.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x000000FF)) >> 0U));
}

void net::Buffer::write_UI64(net::Buffer &buffer, std::uint64_t value)
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

void net::Buffer::write_string(net::Buffer &buffer, const std::string &value)
{
    const std::size_t size = cxpr::min<std::size_t>(UINT16_MAX, value.size());
    net::Buffer::write_UI16(buffer, static_cast<std::uint16_t>(size));
    for(std::size_t i = 0; i < size; net::Buffer::write_UI8(buffer, value[i++]));
}

float net::Buffer::read_F32(net::Buffer &buffer)
{
    return floathacks::uint32_to_float(net::Buffer::read_UI32(buffer));
}

std::int8_t net::Buffer::read_I8(net::Buffer &buffer)
{
    return static_cast<std::int8_t>(net::Buffer::read_UI8(buffer));
}

std::int16_t net::Buffer::read_I16(net::Buffer &buffer)
{
    return static_cast<std::int16_t>(net::Buffer::read_UI16(buffer));
}

std::int32_t net::Buffer::read_I32(net::Buffer &buffer)
{
    return static_cast<std::int32_t>(net::Buffer::read_UI32(buffer));
}

std::int64_t net::Buffer::read_I64(net::Buffer &buffer)
{
    return static_cast<std::int64_t>(net::Buffer::read_UI64(buffer));
}

std::uint8_t net::Buffer::read_UI8(net::Buffer &buffer)
{
    if((buffer.read_position + 1U) <= buffer.vector.size()) {
        std::uint8_t result = buffer.vector[buffer.read_position];
        buffer.read_position += 1U;
        return result;
    }

    buffer.read_position += 1U;
    return UINT8_MAX;
}

std::uint16_t net::Buffer::read_UI16(net::Buffer &buffer)
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

std::uint32_t net::Buffer::read_UI32(net::Buffer &buffer)
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

std::uint64_t net::Buffer::read_UI64(net::Buffer &buffer)
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

std::string net::Buffer::read_string(net::Buffer &buffer)
{
    std::size_t size = net::Buffer::read_UI16(buffer);
    std::string result = std::string();

    for(std::size_t i = 0; i < size; ++i) {
        if(buffer.read_position < buffer.vector.size())
            result.push_back(buffer.vector[buffer.read_position]);
        buffer.read_position += 1U;
    }

    return std::move(result);
}
