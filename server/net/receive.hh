#ifndef D77922EA_96E4_44AB_B819_2754E8E6FB3E
#define D77922EA_96E4_44AB_B819_2754E8E6FB3E

#include "shared/coord.hh"

// What a session wants from a chunk. The two are disjoint: a session that
// holds the full chunk does not need its pyramid, and the far ring does not
// hold the blocks.
enum class chunk_want : std::uint8_t {
    FULL,
    LOD,
};

// A chunk or a chunk LOD pyramid was handed to ENet for a peer. The delivery
// state lives with the stream planner, but only after this point: queueing a
// send is not the same as the peer having the data.
class ChunkSentEvent final {
public:
    explicit ChunkSentEvent(ENetPeer* peer, const ChunkPos& pos, chunk_want kind);
    constexpr ENetPeer* peer(void) const;
    constexpr const ChunkPos& pos(void) const;
    constexpr chunk_want kind(void) const;

private:
    ENetPeer* m_peer;
    ChunkPos m_pos;
    chunk_want m_kind;
};

namespace receive
{
void init(void);
} // namespace receive

namespace receive
{
void fixed_update_late(void);
} // namespace receive

namespace receive
{
// Drops every queued chunk send and pending chunk request that targets the
// peer. ENet reuses host->peers slots, so a stale entry would leak into the
// next connection that lands on the same address.
void forget_peer(ENetPeer* peer);
} // namespace receive

namespace receive
{
// Queues a chunk for the peer, starting the load or generation when it is
// not in the world yet. A LOD request waits for the chunk and sends only the
// pyramid.
void enqueue_chunk(ENetPeer* peer, const ChunkPos& cpos, chunk_want kind);
} // namespace receive

constexpr ENetPeer* ChunkSentEvent::peer(void) const
{
    return m_peer;
}

constexpr const ChunkPos& ChunkSentEvent::pos(void) const
{
    return m_pos;
}

constexpr chunk_want ChunkSentEvent::kind(void) const
{
    return m_kind;
}

#endif /* D77922EA_96E4_44AB_B819_2754E8E6FB3E */
