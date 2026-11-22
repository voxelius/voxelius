#include "shared/pch.hh"

#include "shared/net/packet_player.hh"

#include "core/buffer.hh"

void packet::Player_AttackE::encode(const Player_AttackE& packet, WriteBuffer& buffer)
{
    buffer.write<std::uint64_t>(static_cast<std::uint64_t>(packet.target));
}

void packet::Player_AttackE::decode(Player_AttackE& packet, ReadBuffer& buffer)
{
    packet.target = static_cast<entt::entity>(buffer.read<std::uint64_t>());
}

void packet::Player_AttackB::encode(const Player_AttackB& packet, WriteBuffer& buffer)
{
    buffer.write_vector<std::int64_t, 3>(packet.bpos.cast<std::int64_t>());
    buffer.write<std::uint32_t>(packet.expected);
}

void packet::Player_AttackB::decode(Player_AttackB& packet, ReadBuffer& buffer)
{
    auto bpos = buffer.read_vector<std::int64_t, 3>();
    packet.bpos = bpos.cast<BlockPos::value_type>();
    packet.expected = buffer.read<std::uint32_t>();
}

void packet::Player_InteractE::encode(const Player_InteractE& packet, WriteBuffer& buffer)
{
    buffer.write<std::uint64_t>(static_cast<std::uint64_t>(packet.target));
}

void packet::Player_InteractE::decode(Player_InteractE& packet, ReadBuffer& buffer)
{
    packet.target = static_cast<entt::entity>(buffer.read<std::uint64_t>());
}

void packet::Player_InteractB::encode(const Player_InteractB& packet, WriteBuffer& buffer)
{
    buffer.write_vector<std::int64_t, 3>(packet.bpos.cast<std::int64_t>());
    buffer.write<std::uint32_t>(packet.expected);
    buffer.write<std::uint8_t>(static_cast<std::uint8_t>(packet.face));
    buffer.write_vector<float, 3>(packet.normal);
    buffer.write_vector<float, 3>(packet.point);
}

void packet::Player_InteractB::decode(Player_InteractB& packet, ReadBuffer& buffer)
{
    auto bpos = buffer.read_vector<std::int64_t, 3>();
    packet.bpos = bpos.cast<BlockPos::value_type>();
    packet.expected = buffer.read<std::uint32_t>();
    packet.face = static_cast<block_face>(buffer.read<std::uint8_t>());
    packet.normal = buffer.read_vector<float, 3>();
    packet.point = buffer.read_vector<float, 3>();
}

void packet::Player_MoveData::encode(const Player_MoveData& packet, WriteBuffer& buffer)
{
    buffer.write_vector<std::int32_t, 3>(packet.simulated_cpos.cast<std::int32_t>());
    buffer.write_vector<float, 3>(packet.camera_angles);
    buffer.write_vector<float, 3>(packet.simulated_lpos);
    buffer.write_vector<float, 3>(packet.velocity);
    buffer.write_vector<float, 3>(packet.wishdir);
}

void packet::Player_MoveData::decode(Player_MoveData& packet, ReadBuffer& buffer)
{
    auto cpos = buffer.read_vector<std::int32_t, 3>();
    packet.simulated_cpos = cpos.cast<ChunkPos::value_type>();
    packet.camera_angles = buffer.read_vector<float, 3>();
    packet.simulated_lpos = buffer.read_vector<float, 3>();
    packet.velocity = buffer.read_vector<float, 3>();
    packet.wishdir = buffer.read_vector<float, 3>();
}
