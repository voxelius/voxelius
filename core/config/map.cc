#include "core/pch.hh"

#include "core/config/map.hh"

#include "core/concepts.hh"
#include "core/utils/physfs.hh"
#include "core/utils/string.hh"
#include "core/version.hh"

template<vx::arithmetic T>
static std::optional<T> get_arithmetic(const config::Map* map, config::map_slot_type slot)
{
    const auto string = map->raw_string(slot);

    if(string.has_value()) {
        T value;
        auto check = std::from_chars(string->data(), string->data() + string->size(), value);

        if(check.ec == std::errc {}) {
            return value;
        }
    }

    return std::nullopt;
}

template<vx::arithmetic T, std::size_t N>
static std::optional<Eigen::Vector<T, N>> get_vector(const config::Map* map, config::map_slot_type slot)
{
    const auto string = map->raw_string(slot);

    if(string.has_value()) {
        std::string_view sv = string.value();
        auto bracket_op = sv.find('[');
        auto bracket_cl = sv.find(']');

        if(bracket_op == std::string_view::npos || bracket_cl == std::string_view::npos || bracket_op > bracket_cl) {
            return std::nullopt;
        }

        std::string_view inner = sv.substr(bracket_op + 1, bracket_cl - bracket_op - 1);
        inner = utils::trim_whitespace(inner);

        auto tokens = utils::tokenize(inner);

        if(tokens.size() >= N) {
            Eigen::Vector<T, N> vector;

            for(std::size_t i = 0; i < N; ++i) {
                auto check = std::from_chars(tokens[i].data(), tokens[i].data() + tokens[i].size(), vector[i]);

                if(check.ec != std::errc {}) {
                    return std::nullopt;
                }
            }

            return vector;
        }
    }

    return std::nullopt;
}

std::optional<std::string_view> config::Map::raw_string(map_slot_type slot) const
{
    if(slot >= m_slots.size() || !m_slots[slot].has_value()) {
        return std::nullopt;
    }

    return std::string_view(m_slots[slot].value());
}

void config::Map::set_raw_string(map_slot_type slot, std::string_view value)
{
    if(slot < m_slots.size()) {
        m_generations[slot] += 1;
        m_slots[slot] = std::string(value);
    }
}

std::uint64_t config::Map::generation(map_slot_type slot) const
{
    if(slot >= m_generations.size()) {
        return 0;
    }

    return m_generations[slot];
}

config::map_slot_type config::Map::find_slot(std::string_view key) const
{
    auto it = m_index.find(key);

    if(it == m_index.cend()) {
        return NULL_SLOT;
    }

    return it->second;
}

config::map_slot_type config::Map::find_or_create_slot(std::string_view key)
{
    auto it = m_index.find(key);

    if(it == m_index.cend()) {
        auto slot = m_slots.size();
        m_slots.emplace_back(std::nullopt);
        m_generations.emplace_back(0);
        m_index.try_emplace(std::string(key), slot);

        return slot;
    }

    return it->second;
}

bool config::Map::contains(std::string_view key) const
{
    return m_index.contains(key);
}

void config::Map::purge(void)
{
    m_slots.clear();
    m_generations.clear();
    m_index.clear();
}

void config::Map::load(std::istream& stream)
{
    std::string line;
    std::string kv_string;

    while(std::getline(stream, line)) {
        kv_string = utils::remove_comments(line);
        kv_string = utils::trim_whitespace(kv_string);

        if(utils::is_whitespace(kv_string)) {
            continue;
        }

        auto separator = kv_string.find('=');

        if(separator == std::string::npos) {
            msg::log_warning("[stream]: invalid line: '{}'", line);
            continue;
        }

        auto kv_name = kv_string.substr(0, separator);
        auto kv_value = kv_string.substr(separator + 1);

        kv_name = utils::trim_whitespace(kv_name);
        kv_value = utils::trim_whitespace(kv_value);

        auto slot = find_or_create_slot(kv_name);

        set_raw_string(slot, kv_value);
    }
}

void config::Map::save(std::ostream& stream) const
{
    auto curtime = std::time(nullptr);

    stream << "# Voxelius " << version::semantic << " configuration file" << std::endl;
    stream << "# Generated on " << std::asctime(std::localtime(&curtime)) << std::endl;

    for(const auto& it : m_index) {
        const auto& slot = m_slots[it.second];

        if(slot.has_value()) {
            stream << it.first << "=";
            stream << slot.value();
            stream << std::endl;
        }
    }
}

bool config::Map::load(std::string_view path)
{
    std::istringstream stream;

    if(utils::read_file(path, stream)) {
        load(stream);

        return true;
    }

    return false;
}

bool config::Map::save(std::string_view path) const
{
    std::ostringstream stream;

    save(stream);

    return utils::write_file(path, stream.str());
}

template<>
std::optional<std::string> config::Map::value_raw<std::string>(map_slot_type slot) const
{
    auto string = raw_string(slot);

    if(string.has_value()) {
        return std::string(string.value());
    }

    return std::nullopt;
}

template<>
void config::Map::set_value_raw<std::string>(map_slot_type slot, const std::string& value)
{
    set_raw_string(slot, value);
}

template<>
std::optional<bool> config::Map::value_raw<bool>(map_slot_type slot) const
{
    auto string = raw_string(slot);

    if(string.has_value()) {
        auto text = string.value();
        auto is_true = false;
        is_true = is_true || 0 == text.compare("true");
        is_true = is_true || 0 == text.compare("yes");
        is_true = is_true || 0 == text.compare("on");
        is_true = is_true || 0 == text.compare("1");

        auto is_false = false;
        is_false = is_false || 0 == text.compare("false");
        is_false = is_false || 0 == text.compare("off");
        is_false = is_false || 0 == text.compare("no");
        is_false = is_false || 0 == text.compare("0");

        return is_true && !is_false;
    }

    return std::nullopt;
}

template<>
std::optional<short> config::Map::value_raw<short>(map_slot_type slot) const
{
    return get_arithmetic<short>(this, slot);
}

template<>
std::optional<int> config::Map::value_raw<int>(map_slot_type slot) const
{
    return get_arithmetic<int>(this, slot);
}

template<>
std::optional<long> config::Map::value_raw<long>(map_slot_type slot) const
{
    return get_arithmetic<long>(this, slot);
}

template<>
std::optional<long long> config::Map::value_raw<long long>(map_slot_type slot) const
{
    return get_arithmetic<long long>(this, slot);
}

template<>
std::optional<unsigned short> config::Map::value_raw<unsigned short>(map_slot_type slot) const
{
    return get_arithmetic<unsigned short>(this, slot);
}

template<>
std::optional<unsigned> config::Map::value_raw<unsigned>(map_slot_type slot) const
{
    return get_arithmetic<unsigned>(this, slot);
}

template<>
std::optional<unsigned long> config::Map::value_raw<unsigned long>(map_slot_type slot) const
{
    return get_arithmetic<unsigned long>(this, slot);
}

template<>
std::optional<unsigned long long> config::Map::value_raw<unsigned long long>(map_slot_type slot) const
{
    return get_arithmetic<unsigned long long>(this, slot);
}

template<>
std::optional<float> config::Map::value_raw<float>(map_slot_type slot) const
{
    return get_arithmetic<float>(this, slot);
}

template<>
std::optional<double> config::Map::value_raw<double>(map_slot_type slot) const
{
    return get_arithmetic<double>(this, slot);
}

template<>
std::optional<Eigen::Vector2i> config::Map::value_raw<Eigen::Vector2i>(map_slot_type slot) const
{
    return get_vector<int, 2>(this, slot);
}

template<>
std::optional<Eigen::Vector3i> config::Map::value_raw<Eigen::Vector3i>(map_slot_type slot) const
{
    return get_vector<int, 3>(this, slot);
}

template<>
std::optional<Eigen::Vector4i> config::Map::value_raw<Eigen::Vector4i>(map_slot_type slot) const
{
    return get_vector<int, 4>(this, slot);
}

template<>
std::optional<Eigen::Vector2f> config::Map::value_raw<Eigen::Vector2f>(map_slot_type slot) const
{
    return get_vector<float, 2>(this, slot);
}

template<>
std::optional<Eigen::Vector3f> config::Map::value_raw<Eigen::Vector3f>(map_slot_type slot) const
{
    return get_vector<float, 3>(this, slot);
}

template<>
std::optional<Eigen::Vector4f> config::Map::value_raw<Eigen::Vector4f>(map_slot_type slot) const
{
    return get_vector<float, 4>(this, slot);
}

template<>
std::optional<Eigen::Vector2d> config::Map::value_raw<Eigen::Vector2d>(map_slot_type slot) const
{
    return get_vector<double, 2>(this, slot);
}

template<>
std::optional<Eigen::Vector3d> config::Map::value_raw<Eigen::Vector3d>(map_slot_type slot) const
{
    return get_vector<double, 3>(this, slot);
}

template<>
std::optional<Eigen::Vector4d> config::Map::value_raw<Eigen::Vector4d>(map_slot_type slot) const
{
    return get_vector<double, 4>(this, slot);
}

template<>
void config::Map::set_value_raw<bool>(map_slot_type slot, const bool& value)
{
    if(value) {
        set_raw_string(slot, "true");
    }
    else {
        set_raw_string(slot, "false");
    }
}

template<>
void config::Map::set_value_raw<short>(map_slot_type slot, const short& value)
{
    set_raw_string(slot, std::to_string(value));
}

template<>
void config::Map::set_value_raw<int>(map_slot_type slot, const int& value)
{
    set_raw_string(slot, std::to_string(value));
}

template<>
void config::Map::set_value_raw<long>(map_slot_type slot, const long& value)
{
    set_raw_string(slot, std::to_string(value));
}

template<>
void config::Map::set_value_raw<long long>(map_slot_type slot, const long long& value)
{
    set_raw_string(slot, std::to_string(value));
}

template<>
void config::Map::set_value_raw<unsigned short>(map_slot_type slot, const unsigned short& value)
{
    set_raw_string(slot, std::to_string(value));
}

template<>
void config::Map::set_value_raw<unsigned>(map_slot_type slot, const unsigned& value)
{
    set_raw_string(slot, std::to_string(value));
}

template<>
void config::Map::set_value_raw<unsigned long>(map_slot_type slot, const unsigned long& value)
{
    set_raw_string(slot, std::to_string(value));
}

template<>
void config::Map::set_value_raw<unsigned long long>(map_slot_type slot, const unsigned long long& value)
{
    set_raw_string(slot, std::to_string(value));
}

template<>
void config::Map::set_value_raw<float>(map_slot_type slot, const float& value)
{
    set_raw_string(slot, std::to_string(value));
}

template<>
void config::Map::set_value_raw<double>(map_slot_type slot, const double& value)
{
    set_raw_string(slot, std::to_string(value));
}

template<>
void config::Map::set_value_raw<Eigen::Vector2i>(map_slot_type slot, const Eigen::Vector2i& value)
{
    set_raw_string(slot, std::format("[{} {}]", value.x(), value.y()));
}

template<>
void config::Map::set_value_raw<Eigen::Vector3i>(map_slot_type slot, const Eigen::Vector3i& value)
{
    set_raw_string(slot, std::format("[{} {} {}]", value.x(), value.y(), value.z()));
}

template<>
void config::Map::set_value_raw<Eigen::Vector4i>(map_slot_type slot, const Eigen::Vector4i& value)
{
    set_raw_string(slot, std::format("[{} {} {} {}]", value.x(), value.y(), value.z(), value.w()));
}

template<>
void config::Map::set_value_raw<Eigen::Vector2f>(map_slot_type slot, const Eigen::Vector2f& value)
{
    set_raw_string(slot, std::format("[{} {}]", value.x(), value.y()));
}

template<>
void config::Map::set_value_raw<Eigen::Vector3f>(map_slot_type slot, const Eigen::Vector3f& value)
{
    set_raw_string(slot, std::format("[{} {} {}]", value.x(), value.y(), value.z()));
}

template<>
void config::Map::set_value_raw<Eigen::Vector4f>(map_slot_type slot, const Eigen::Vector4f& value)
{
    set_raw_string(slot, std::format("[{} {} {} {}]", value.x(), value.y(), value.z(), value.w()));
}

template<>
void config::Map::set_value_raw<Eigen::Vector2d>(map_slot_type slot, const Eigen::Vector2d& value)
{
    set_raw_string(slot, std::format("[{} {}]", value.x(), value.y()));
}

template<>
void config::Map::set_value_raw<Eigen::Vector3d>(map_slot_type slot, const Eigen::Vector3d& value)
{
    set_raw_string(slot, std::format("[{} {} {}]", value.x(), value.y(), value.z()));
}

template<>
void config::Map::set_value_raw<Eigen::Vector4d>(map_slot_type slot, const Eigen::Vector4d& value)
{
    set_raw_string(slot, std::format("[{} {} {} {}]", value.x(), value.y(), value.z(), value.w()));
}
