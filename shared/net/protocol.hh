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
constexpr static enet_uint32 CHANNEL_COUNT = 2;
} // namespace protocol

namespace protocol
{
// How many packets of each family were handed to ENet. Taking the counts
// resets them, so one call covers one interval.
struct NetStats final {
    std::uint64_t chunks { 0 };
    std::uint64_t biomes { 0 };
    std::uint64_t light { 0 };
    std::uint64_t set_block { 0 };
    std::uint64_t entities { 0 };
    std::uint64_t timings { 0 };
    std::uint64_t other { 0 };
};
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

namespace protocol
{
NetStats take_sent_stats(void);

// The same, counted where packets are decoded.
NetStats take_received_stats(void);
} // namespace protocol

#endif /* A3E273F9_3BF0_49AA_8E3B_C6E5385F7259 */
