// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <shared/math/floathacks.hh>
#include <shared/packet_reader.hh>
#include <shared/packet_writer.hh>

void PacketReader::setup(PacketReader &reader, const ENetPacket *packet)
{
    reader.read_position = 0;
    reader.buffer.assign(packet->data, packet->data + packet->dataLength);
}

void PacketReader::setup(PacketReader &reader, const void *data, std::size_t size)
{
    reader.read_position = 0;
    const auto dp = reinterpret_cast<const std::uint8_t *>(data);
    reader.buffer.assign(dp, dp + size);
}

void PacketReader::setup(PacketReader &reader, const PacketWriter &writer)
{
    reader.read_position = 0;
    reader.buffer.assign(writer.buffer.cbegin(), writer.buffer.cend());
}

std::int8_t PacketReader::I8(PacketReader &reader)
{
    return static_cast<std::int8_t>(PacketReader::UI8(reader));
}

std::int16_t PacketReader::I16(PacketReader &reader)
{
    return static_cast<std::int16_t>(PacketReader::UI16(reader));
}

std::int32_t PacketReader::I32(PacketReader &reader)
{
    return static_cast<std::int32_t>(PacketReader::UI32(reader));
}

std::int64_t PacketReader::I64(PacketReader &reader)
{
    return static_cast<std::int64_t>(PacketReader::UI64(reader));
}

std::uint8_t PacketReader::UI8(PacketReader &reader)
{
    if((reader.read_position + 1U) <= reader.buffer.size()) {
        std::uint8_t result = reader.buffer[reader.read_position];
        reader.read_position += 1U;
        return result;
    }

    reader.read_position += 1U;
    return UINT8_MAX;
}

std::uint16_t PacketReader::UI16(PacketReader &reader)
{
    if((reader.read_position + 2U) <= reader.buffer.size()) {
        std::uint16_t result = UINT16_C(0x0000);
        result |= static_cast<std::uint16_t>(reader.buffer[reader.read_position + 0U]) << 8U;
        result |= static_cast<std::uint16_t>(reader.buffer[reader.read_position + 1U]) << 0U;
        reader.read_position += 2U;
        return result;
    }

    reader.read_position += 2U;
    return UINT16_MAX;
}

std::uint32_t PacketReader::UI32(PacketReader &reader)
{
    if((reader.read_position + 4U) <= reader.buffer.size()) {
        std::uint32_t result = UINT32_C(0x00000000);
        result |= static_cast<std::uint32_t>(reader.buffer[reader.read_position + 0U]) << 24U;
        result |= static_cast<std::uint32_t>(reader.buffer[reader.read_position + 1U]) << 16U;
        result |= static_cast<std::uint32_t>(reader.buffer[reader.read_position + 2U]) << 8U;
        result |= static_cast<std::uint32_t>(reader.buffer[reader.read_position + 3U]) << 0U;
        reader.read_position += 4U;
        return result;
    }

    reader.read_position += 4U;
    return UINT32_MAX;
}

std::uint64_t PacketReader::UI64(PacketReader &reader)
{
    if((reader.read_position + 8U) <= reader.buffer.size()) {
        std::uint64_t result = UINT64_C(0x0000000000000000);
        result |= static_cast<std::uint64_t>(reader.buffer[reader.read_position + 0U]) << 56U;
        result |= static_cast<std::uint64_t>(reader.buffer[reader.read_position + 1U]) << 48U;
        result |= static_cast<std::uint64_t>(reader.buffer[reader.read_position + 2U]) << 40U;
        result |= static_cast<std::uint64_t>(reader.buffer[reader.read_position + 3U]) << 32U;
        result |= static_cast<std::uint64_t>(reader.buffer[reader.read_position + 4U]) << 24U;
        result |= static_cast<std::uint64_t>(reader.buffer[reader.read_position + 5U]) << 16U;
        result |= static_cast<std::uint64_t>(reader.buffer[reader.read_position + 6U]) << 8U;
        result |= static_cast<std::uint64_t>(reader.buffer[reader.read_position + 7U]) << 0U;
        reader.read_position += 8U;
        return result;
    }

    reader.read_position += 8U;
    return UINT64_MAX;
}

std::string PacketReader::string(PacketReader &reader)
{
    std::size_t size = PacketReader::UI16(reader);
    std::string result = std::string();

    for(std::size_t i = 0; i < size; ++i) {
        if(reader.read_position < reader.buffer.size())
            result.push_back(reader.buffer[reader.read_position]);
        reader.read_position += 1U;
    }

    return std::move(result);
}

float PacketReader::FP32(PacketReader &reader)
{
    return floathacks::uint32_to_float(PacketReader::UI32(reader));
}
