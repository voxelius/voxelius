#include "server/pch.hh"

#include "server/world/entropy_cache.hh"

struct Entry final {
    std::once_flag init_flag;
    cached_entropy_type data;
};

static std::shared_mutex s_mutex;
static vx::hash_map<ChunkPosXZ, std::shared_ptr<Entry>> s_cache;
static std::uint64_t s_seed;

static void generate(const ChunkPosXZ& pos, cached_entropy_type& data)
{
    std::mt19937_64 random;
    random.seed(s_seed ^ std::hash<ChunkPosXZ>()(pos));

    for(std::size_t i = 0; i < constant::CHUNK_AREA; ++i) {
        data[i] = random();
    }
}

static std::shared_ptr<Entry> find_entry(const ChunkPosXZ& pos)
{
    std::shared_lock lock(s_mutex);

    auto it = s_cache.find(pos);

    if(it == s_cache.end()) {
        return nullptr;
    }

    return it->second;
}

static std::shared_ptr<Entry> get_entry(const ChunkPosXZ& pos)
{
    if(auto entry = find_entry(pos)) {
        return entry;
    }

    std::unique_lock lock(s_mutex);

    // Another thread may have inserted this column while this one waited for
    // the exclusive lock. Overwriting its entry frees an array that an
    // in-flight placement pass still reads through a reference.
    auto it = s_cache.find(pos);

    if(it != s_cache.end()) {
        return it->second;
    }

    auto entry = std::make_shared<Entry>();
    s_cache.emplace(pos, entry);
    return entry;
}

void entropy_cache::init(std::mt19937_64& seeder)
{
    s_seed = seeder();
}

void entropy_cache::shutdown(void)
{
    std::unique_lock lock(s_mutex);

    s_cache.clear();
}

const cached_entropy_type& entropy_cache::get(const ChunkPosXZ& pos)
{
    auto entry = get_entry(pos);

    std::call_once(entry->init_flag, [pos, entry] {
        generate(pos, entry->data);
    });

    return entry->data;
}
