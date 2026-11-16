#include "shared/pch.hh"

#include "shared/world/chunk.hh"

#include "core/buffer.hh"

// Marks the schedule trailer so a slot saved before the trailer existed
// decodes as blocks with no pending ticks instead of garbage.
constexpr static std::uint32_t SCHEDULE_MAGIC = 0x4B434954; // TICK

static void serialize_schedule(const Chunk::schedule_type& schedule, WriteBuffer& buffer)
{
    buffer.write<std::uint32_t>(static_cast<std::uint32_t>(schedule.size()));

    for(const auto& [deadline, entry] : schedule) {
        buffer.write<std::uint64_t>(deadline);
        buffer.write<std::uint32_t>(static_cast<std::uint32_t>(entry.first));
        buffer.write<std::uint32_t>(static_cast<std::uint32_t>(entry.second));
    }
}

static void deserialize_schedule(Chunk::schedule_type& schedule, ReadBuffer& buffer)
{
    constexpr static std::size_t RECORD_SIZE = sizeof(std::uint64_t) + 2 * sizeof(std::uint32_t);

    if(!buffer.has_records(1, sizeof(std::uint32_t))) {
        return;
    }

    const auto size = buffer.read<std::uint32_t>();

    if(!buffer.has_records(size, RECORD_SIZE)) {
        return;
    }

    for(std::uint32_t i = 0; i < size; ++i) {
        auto deadline = buffer.read<std::uint64_t>();
        auto index = buffer.read<std::uint32_t>();
        auto source = buffer.read<std::uint32_t>();
        schedule.emplace(deadline, std::make_pair(index, static_cast<block_tick_source>(source)));
    }
}

Chunk::Chunk(entt::entity entity) : m_entity(entity)
{
    // empty
}

void Chunk::encode(const Chunk& chunk, WriteBuffer& buffer)
{
    BlockStorage::encode_net(chunk.m_blocks, buffer);
    serialize_schedule(chunk.m_scheduled, buffer);
}

void Chunk::decode(Chunk& chunk, ReadBuffer& buffer)
{
    BlockStorage::decode_net(chunk.m_blocks, buffer);
    deserialize_schedule(chunk.m_scheduled, buffer);
}

void Chunk::encode_dat(const Snapshot& snapshot, WriteBuffer& buffer)
{
    BlockStorage::encode_dat(snapshot.blocks, buffer);

    buffer.write<std::uint32_t>(SCHEDULE_MAGIC);
    serialize_schedule(snapshot.scheduled, buffer);
}

void Chunk::decode_dat(Snapshot& snapshot, ReadBuffer& buffer)
{
    BlockStorage::decode_dat(snapshot.blocks, buffer);

    snapshot.scheduled.clear();

    if(!buffer.has_records(1, sizeof(std::uint32_t))) {
        return;
    }

    if(buffer.read<std::uint32_t>() != SCHEDULE_MAGIC) {
        return;
    }

    deserialize_schedule(snapshot.scheduled, buffer);
}

block_id_type Chunk::get_block(std::size_t index) const
{
    return m_blocks.get(index);
}

block_id_type Chunk::get_block(const LocalPos& pos) const
{
    return m_blocks.get(pos);
}

void Chunk::set_block(std::size_t index, block_id_type id)
{
    m_blocks.set(index, id);
}

void Chunk::set_block(const LocalPos& pos, block_id_type id)
{
    m_blocks.set(pos, id);
}

void Chunk::set_blocks(BlockStorage blocks)
{
    m_blocks = std::move(blocks);
}

Chunk::Snapshot Chunk::snapshot(std::uint64_t now) const
{
    Snapshot result;
    result.blocks = m_blocks;

    for(const auto& [deadline, entry] : m_scheduled) {
        result.scheduled.emplace(deadline > now ? deadline - now : std::uint64_t(0), entry);
    }

    return result;
}

void Chunk::restore(Snapshot snapshot, std::uint64_t now)
{
    m_blocks = std::move(snapshot.blocks);
    m_scheduled.clear();

    for(const auto& [remaining, entry] : snapshot.scheduled) {
        auto deadline = now + remaining;

        if(deadline < now) {
            continue; // corrupted file
        }

        m_scheduled.emplace(deadline, entry);
    }
}

void Chunk::schedule(std::size_t index, std::uint64_t deadline, block_tick_source source)
{
    auto it = m_scheduled.begin();

    while(it != m_scheduled.end()) {
        if(it->second.first == index && it->second.second == source) {
            if(it->first <= deadline) {
                return;
            }

            it = m_scheduled.erase(it);
            continue;
        }

        it = std::next(it);
    }

    m_scheduled.emplace(deadline, std::make_pair(index, source));
}

void Chunk::pop_due(std::uint64_t now, std::vector<std::pair<std::size_t, block_tick_source>>& out)
{
    auto it = m_scheduled.begin();

    for(;;) {
        if(it == m_scheduled.end() || it->first > now) {
            break;
        }

        out.emplace_back(it->second);

        it = m_scheduled.erase(it);
    }
}
