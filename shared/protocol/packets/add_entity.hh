#ifndef SHARED_PROTOCOL_PACKETS_ADD_ENTITY_HH
#define SHARED_PROTOCOL_PACKETS_ADD_ENTITY_HH
#include <entt/entity/entity.hpp>
#include <shared/protocol/protocol.hh>

struct protocol::AddEntity final : public protocol::BasePacket<0x0005> {
    entt::entity entity {};
};

template<>
inline void protocol::decode_packet<protocol::AddEntity>(PacketReader &reader, protocol::AddEntity &packet)
{
    packet.entity = static_cast<entt::entity>(PacketReader::UI64(reader));
}

template<>
inline void protocol::encode_packet<protocol::AddEntity>(PacketWriter &writer, const protocol::AddEntity &packet)
{
    PacketWriter::UI64(writer, static_cast<std::uint64_t>(packet.entity));
}

#endif /* SHARED_PROTOCOL_PACKETS_ADD_ENTITY_HH */
