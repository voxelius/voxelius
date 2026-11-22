#include "shared/pch.hh"

#include "shared/net/packet_status.hh"

#include "core/buffer.hh"

void packet::Status_Request::encode(const Status_Request& packet, WriteBuffer& buffer)
{
    buffer.write<std::uint32_t>(packet.version_major);
    buffer.write<std::uint32_t>(packet.version_minor);
    buffer.write<std::uint32_t>(packet.version_patch);
}

void packet::Status_Request::decode(Status_Request& packet, ReadBuffer& buffer)
{
    packet.version_major = buffer.read<std::uint32_t>();
    packet.version_minor = buffer.read<std::uint32_t>();
    packet.version_patch = buffer.read<std::uint32_t>();
}

void packet::Status_Response::encode(const Status_Response& packet, WriteBuffer& buffer)
{
    buffer.write<std::uint32_t>(packet.version_major);
    buffer.write<std::uint16_t>(packet.max_players);
    buffer.write<std::uint16_t>(packet.num_players);
    buffer.write<std::string_view>(packet.motd.substr(0, 32));
    buffer.write<std::uint32_t>(packet.version_minor);
    buffer.write<std::uint32_t>(packet.version_patch);
    buffer.write<std::uint32_t>(packet.server_tags);
}

void packet::Status_Response::decode(Status_Response& packet, ReadBuffer& buffer)
{
    packet.version_major = buffer.read<std::uint32_t>();
    packet.max_players = buffer.read<std::uint16_t>();
    packet.num_players = buffer.read<std::uint16_t>();
    auto motd = buffer.read<std::string>();
    packet.motd = motd.substr(0, 32);
    packet.version_minor = buffer.read<std::uint32_t>();
    packet.version_patch = buffer.read<std::uint32_t>();
    packet.server_tags = buffer.read<std::uint32_t>();
}
