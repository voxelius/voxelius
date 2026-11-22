#ifndef C8783E57_21EC_40A0_90FF_1576C67F77BE
#define C8783E57_21EC_40A0_90FF_1576C67F77BE

namespace config
{
using map_slot_type = std::size_t;
constexpr static map_slot_type NULL_SLOT = std::numeric_limits<map_slot_type>::max();
} // namespace config

namespace config
{
class Map final {
public:
    Map(void) = default;

    Map(const Map& other) = delete;
    Map(Map&& other) = delete;
    Map& operator=(const Map& other) = delete;
    Map& operator=(Map&& other) = delete;

    std::optional<std::string_view> raw_string(map_slot_type slot) const;
    void set_raw_string(map_slot_type slot, std::string_view value);

    template<typename T>
    std::optional<T> value(std::string_view key) const;
    template<typename T>
    void set_value(std::string_view key, const T& value);

    template<typename T>
    std::optional<T> value_raw(map_slot_type slot) const;
    template<typename T>
    void set_value_raw(map_slot_type slot, const T& value);

    std::uint64_t generation(map_slot_type slot) const;

    map_slot_type find_slot(std::string_view key) const;
    map_slot_type find_or_create_slot(std::string_view key);

    bool contains(std::string_view key) const;

    void purge(void);

    void load(std::istream& stream);
    void save(std::ostream& stream) const;

    bool load(std::string_view path);
    bool save(std::string_view path) const;

private:
    std::vector<std::uint64_t> m_generations {};
    std::vector<std::optional<std::string>> m_slots {};
    vx::hash_map<std::string, map_slot_type> m_index;
};
} // namespace config

template<typename T>
std::optional<T> config::Map::value(std::string_view key) const
{
    auto slot = find_slot(key);

    if(slot == NULL_SLOT) {
        return std::nullopt;
    }

    return value_raw<T>(slot);
}

template<typename T>
void config::Map::set_value(std::string_view key, const T& value)
{
    auto slot = find_or_create_slot(key);

    set_value_raw<T>(slot, value);
}

#endif /* C8783E57_21EC_40A0_90FF_1576C67F77BE */
