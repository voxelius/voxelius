// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/math/constexpr.hh>
#include <shared/math/floathacks.hh>
#include <shared/packet.hh>

void Packet::setup(Packet &packet)
{
    packet.read_position = 0;
    packet.buffer.clear();
}

void Packet::setup(Packet &packet, const ENetPacket *source)
{
    packet.read_position = 0;
    packet.buffer.assign(source->data, source->data + source->dataLength);
}

ENetPacket *Packet::submit(const Packet &packet, ENetPacketFlag flags)
{
    return enet_packet_create(packet.buffer.data(), packet.buffer.size(), flags);
}

void Packet::write_F32(Packet &packet, float value)
{
    Packet::write_UI32(packet, floathacks::float_to_uint32(value));
}

void Packet::write_I8(Packet &packet, std::int8_t value)
{
    Packet::write_UI8(packet, static_cast<std::uint8_t>(value));
}

void Packet::write_I16(Packet &packet, std::int16_t value)
{
    Packet::write_UI16(packet, static_cast<std::uint16_t>(value));
}

void Packet::write_I32(Packet &packet, std::int32_t value)
{
    Packet::write_UI32(packet, static_cast<std::uint32_t>(value));
}

void Packet::write_I64(Packet &packet, std::int64_t value)
{
    Packet::write_UI64(packet, static_cast<std::uint64_t>(value));
}

void Packet::write_UI8(Packet &packet, std::uint8_t value)
{
    packet.buffer.push_back(value);
}

void Packet::write_UI16(Packet &packet, std::uint16_t value)
{
    packet.buffer.push_back(static_cast<std::uint8_t>((value & UINT16_C(0xFF00)) >> 8U));
    packet.buffer.push_back(static_cast<std::uint8_t>((value & UINT16_C(0x00FF)) >> 0U));
}

void Packet::write_UI32(Packet &packet, std::uint32_t value)
{
    packet.buffer.push_back(static_cast<std::uint8_t>((value & UINT32_C(0xFF000000)) >> 24U));
    packet.buffer.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x00FF0000)) >> 16U));
    packet.buffer.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x0000FF00)) >> 8U));
    packet.buffer.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x000000FF)) >> 0U));
}

void Packet::write_UI64(Packet &packet, std::uint64_t value)
{
    packet.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0xFF00000000000000)) >> 56U));
    packet.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x00FF000000000000)) >> 48U));
    packet.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x0000FF0000000000)) >> 40U));
    packet.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x000000FF00000000)) >> 32U));
    packet.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x00000000FF000000)) >> 24U));
    packet.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x0000000000FF0000)) >> 16U));
    packet.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x000000000000FF00)) >> 8U));
    packet.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x00000000000000FF)) >> 0U));
}

void Packet::write_string(Packet &packet, const std::string &value)
{
    const std::size_t size = cxpr::min<std::size_t>(UINT16_MAX, value.size());
    Packet::write_UI16(packet, static_cast<std::uint16_t>(size));
    for(std::size_t i = 0; i < size; Packet::write_UI8(packet, value[i++]));
}

float Packet::read_F32(Packet &packet)
{
    return floathacks::uint32_to_float(Packet::read_UI32(packet));
}

std::int8_t Packet::read_I8(Packet &packet)
{
    return static_cast<std::int8_t>(Packet::read_UI8(packet));
}

std::int16_t Packet::read_I16(Packet &packet)
{
    return static_cast<std::int16_t>(Packet::read_UI16(packet));
}

std::int32_t Packet::read_I32(Packet &packet)
{
    return static_cast<std::int32_t>(Packet::read_UI32(packet));
}

std::int64_t Packet::read_I64(Packet &packet)
{
    return static_cast<std::int64_t>(Packet::read_UI64(packet));
}

std::uint8_t Packet::read_UI8(Packet &packet)
{
    if((packet.read_position + 1U) <= packet.buffer.size()) {
        std::uint8_t result = packet.buffer[packet.read_position];
        packet.read_position += 1U;
        return result;
    }

    packet.read_position += 1U;
    return UINT8_MAX;
}

std::uint16_t Packet::read_UI16(Packet &packet)
{
    if((packet.read_position + 2U) <= packet.buffer.size()) {
        std::uint16_t result = UINT16_C(0x0000);
        result |= static_cast<std::uint16_t>(packet.buffer[packet.read_position + 0U]) << 8U;
        result |= static_cast<std::uint16_t>(packet.buffer[packet.read_position + 1U]) << 0U;
        packet.read_position += 2U;
        return result;
    }

    packet.read_position += 2U;
    return UINT16_MAX;
}

std::uint32_t Packet::read_UI32(Packet &packet)
{
    if((packet.read_position + 4U) <= packet.buffer.size()) {
        std::uint32_t result = UINT32_C(0x00000000);
        result |= static_cast<std::uint32_t>(packet.buffer[packet.read_position + 0U]) << 24U;
        result |= static_cast<std::uint32_t>(packet.buffer[packet.read_position + 1U]) << 16U;
        result |= static_cast<std::uint32_t>(packet.buffer[packet.read_position + 2U]) << 8U;
        result |= static_cast<std::uint32_t>(packet.buffer[packet.read_position + 3U]) << 0U;
        packet.read_position += 4U;
        return result;
    }

    packet.read_position += 4U;
    return UINT32_MAX;
}

std::uint64_t Packet::read_UI64(Packet &packet)
{
    if((packet.read_position + 8U) <= packet.buffer.size()) {
        std::uint64_t result = UINT64_C(0x0000000000000000);
        result |= static_cast<std::uint64_t>(packet.buffer[packet.read_position + 0U]) << 56U;
        result |= static_cast<std::uint64_t>(packet.buffer[packet.read_position + 1U]) << 48U;
        result |= static_cast<std::uint64_t>(packet.buffer[packet.read_position + 2U]) << 40U;
        result |= static_cast<std::uint64_t>(packet.buffer[packet.read_position + 3U]) << 32U;
        result |= static_cast<std::uint64_t>(packet.buffer[packet.read_position + 4U]) << 24U;
        result |= static_cast<std::uint64_t>(packet.buffer[packet.read_position + 5U]) << 16U;
        result |= static_cast<std::uint64_t>(packet.buffer[packet.read_position + 6U]) << 8U;
        result |= static_cast<std::uint64_t>(packet.buffer[packet.read_position + 7U]) << 0U;
        packet.read_position += 8U;
        return result;
    }

    packet.read_position += 8U;
    return UINT64_MAX;
}

std::string Packet::read_string(Packet &packet)
{
    std::size_t size = Packet::read_UI16(packet);
    std::string result = std::string();

    for(std::size_t i = 0; i < size; ++i) {
        if(packet.read_position < packet.buffer.size())
            result.push_back(packet.buffer[packet.read_position]);
        packet.read_position += 1U;
    }

    return std::move(result);
}
