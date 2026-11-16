#ifndef D77922EA_96E4_44AB_B819_2754E8E6FB3E
#define D77922EA_96E4_44AB_B819_2754E8E6FB3E

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

#endif /* D77922EA_96E4_44AB_B819_2754E8E6FB3E */
