#ifndef A3E273F9_3BF0_49AA_8E3B_C6E5385F7259
#define A3E273F9_3BF0_49AA_8E3B_C6E5385F7259

#include "shared/net/packet.hh"

namespace protocol
{
constexpr static enet_uint16 PORT = 43103;

// ENet keeps one sequence per channel, so channels are how unrelated traffic
// stops waiting for each other. The world stream lives on its own channel: a
// burst of chunk packets no longer holds back auth, entity or timing packets,
// and the reverse holds too.
constexpr static enet_uint32 CHANNEL_DEFAULT = 0;
constexpr static enet_uint32 CHANNEL_WORLD = 1;

// Far terrain LOD rides a channel of its own. A pyramid packet is large, and
// the world channel has to keep its order for blocks, biomes and light.
constexpr static enet_uint32 CHANNEL_LOD = 2;
constexpr static enet_uint32 CHANNEL_COUNT = 3;
} // namespace protocol

namespace protocol
{
void decode(entt::dispatcher& dispatcher, const ENetPacket* packet, ENetPeer* peer);
} // namespace protocol

namespace protocol
{
template<typename T>
void broadcast(const T& packet, ENetHost* host, ENetPeer* except = nullptr);
template<typename T>
void send(const T& packet, ENetPeer* peer);
} // namespace protocol

#endif /* A3E273F9_3BF0_49AA_8E3B_C6E5385F7259 */
