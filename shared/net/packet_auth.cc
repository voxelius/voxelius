#include "shared/pch.hh"

#include "shared/net/packet_auth.hh"

#include "core/buffer.hh"

void packet::Auth_Request::encode(const Auth_Request& packet, WriteBuffer& buffer)
{
    buffer.write<std::uint32_t>(packet.version_major);
    buffer.write<std::uint32_t>(packet.version_minor);
    buffer.write<std::uint32_t>(packet.version_patch);
    buffer.write_bytes(packet.pkey);
    buffer.write<std::uint64_t>(packet.invite_code);
    buffer.write<std::uint64_t>(packet.biomes_hash);
    buffer.write<std::uint64_t>(packet.blocks_hash);
    buffer.write<std::uint64_t>(packet.fluids_hash);
    buffer.write<std::uint64_t>(packet.tints_hash);
    buffer.write<std::uint64_t>(packet.ents_hash);
    buffer.write<std::uint64_t>(packet.weather_hash);
    buffer.write<std::uint64_t>(packet.precip_hash);
    buffer.write<std::string_view>(packet.username.substr(0, 64));
}

void packet::Auth_Request::decode(Auth_Request& packet, ReadBuffer& buffer)
{
    packet.version_major = buffer.read<std::uint32_t>();
    packet.version_minor = buffer.read<std::uint32_t>();
    packet.version_patch = buffer.read<std::uint32_t>();
    buffer.read_bytes(packet.pkey);
    packet.invite_code = buffer.read<std::uint64_t>();
    packet.biomes_hash = buffer.read<std::uint64_t>();
    packet.blocks_hash = buffer.read<std::uint64_t>();
    packet.fluids_hash = buffer.read<std::uint64_t>();
    packet.tints_hash = buffer.read<std::uint64_t>();
    packet.ents_hash = buffer.read<std::uint64_t>();
    packet.weather_hash = buffer.read<std::uint64_t>();
    packet.precip_hash = buffer.read<std::uint64_t>();
    packet.username = buffer.read<std::string>();
}

void packet::Auth_Challenge::encode(const Auth_Challenge& packet, WriteBuffer& buffer)
{
    buffer.write_bytes(packet.nonce);
}

void packet::Auth_Challenge::decode(Auth_Challenge& packet, ReadBuffer& buffer)
{
    buffer.read_bytes(packet.nonce);
}

void packet::Auth_Response::encode(const Auth_Response& packet, WriteBuffer& buffer)
{
    buffer.write_bytes(packet.signature);
}

void packet::Auth_Response::decode(Auth_Response& packet, ReadBuffer& buffer)
{
    buffer.read_bytes(packet.signature);
}

void packet::Auth_Admission::encode(const Auth_Admission& packet, WriteBuffer& buffer)
{
    buffer.write<std::uint16_t>(packet.client_id);
    buffer.write<std::uint64_t>(packet.identity);
    buffer.write<std::string_view>(packet.username.substr(0, 64));
}

void packet::Auth_Admission::decode(Auth_Admission& packet, ReadBuffer& buffer)
{
    packet.client_id = buffer.read<std::uint16_t>();
    packet.identity = buffer.read<std::uint64_t>();
    packet.username = buffer.read<std::string>();
}
