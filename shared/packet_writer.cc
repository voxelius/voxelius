// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/math/constexpr.hh>
#include <shared/math/floathacks.hh>
#include <shared/packet_writer.hh>

ENetPacket *PacketWriter::submit(const PacketWriter &writer, ENetPacketFlag flags)
{
    return enet_packet_create(writer.buffer.data(), writer.buffer.size(), flags);
}

void PacketWriter::I8(PacketWriter &writer, std::int8_t value)
{
    PacketWriter::UI8(writer, static_cast<std::uint8_t>(value));
}

void PacketWriter::I16(PacketWriter &writer, std::int16_t value)
{
    PacketWriter::UI16(writer, static_cast<std::uint16_t>(value));
}

void PacketWriter::I32(PacketWriter &writer, std::int32_t value)
{
    PacketWriter::UI32(writer, static_cast<std::uint32_t>(value));
}

void PacketWriter::I64(PacketWriter &writer, std::int64_t value)
{
    PacketWriter::UI64(writer, static_cast<std::uint64_t>(value));
}

void PacketWriter::UI8(PacketWriter &writer, std::uint8_t value)
{
    writer.buffer.push_back(value);
}

void PacketWriter::UI16(PacketWriter &writer, std::uint16_t value)
{
    writer.buffer.push_back(static_cast<std::uint8_t>((value & UINT16_C(0xFF00)) >> 8U));
    writer.buffer.push_back(static_cast<std::uint8_t>((value & UINT16_C(0x00FF)) >> 0U));
}

void PacketWriter::UI32(PacketWriter &writer, std::uint32_t value)
{
    writer.buffer.push_back(static_cast<std::uint8_t>((value & UINT32_C(0xFF000000)) >> 24U));
    writer.buffer.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x00FF0000)) >> 16U));
    writer.buffer.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x0000FF00)) >> 8U));
    writer.buffer.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x000000FF)) >> 0U));
}

void PacketWriter::UI64(PacketWriter &writer, std::uint64_t value)
{
    writer.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0xFF00000000000000)) >> 56U));
    writer.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x00FF000000000000)) >> 48U));
    writer.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x0000FF0000000000)) >> 40U));
    writer.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x000000FF00000000)) >> 32U));
    writer.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x00000000FF000000)) >> 24U));
    writer.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x0000000000FF0000)) >> 16U));
    writer.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x000000000000FF00)) >> 8U));
    writer.buffer.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x00000000000000FF)) >> 0U));
}

void PacketWriter::string(PacketWriter &writer, const std::string &value)
{
    const std::size_t size = cxpr::min<std::size_t>(UINT16_MAX, value.size());
    PacketWriter::UI16(writer, static_cast<std::uint16_t>(size));
    for(std::size_t i = 0; i < size; PacketWriter::UI8(writer, value[i++]));
}

void PacketWriter::FP32(PacketWriter &writer, float value)
{
    PacketWriter::UI32(writer, floathacks::float_to_uint32(value));
}
