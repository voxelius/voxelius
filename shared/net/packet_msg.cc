#include "shared/pch.hh"

#include "shared/net/packet_msg.hh"

#include "core/buffer.hh"

void packet::Msg_Log::encode(const Msg_Log& packet, WriteBuffer& buffer)
{
    buffer.write<std::uint32_t>(packet.request_id);
    buffer.write<std::uint8_t>(packet.level);
    buffer.write<std::string_view>(std::string_view(packet.message).substr(0, MAX_MESSAGE));
}

void packet::Msg_Log::decode(Msg_Log& packet, ReadBuffer& buffer)
{
    packet.request_id = buffer.read<std::uint32_t>();
    packet.level = buffer.read<std::uint8_t>();
    auto message = buffer.read<std::string>();
    packet.message = message.substr(0, MAX_MESSAGE);
}
