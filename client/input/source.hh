#ifndef A73825E6_9EFA_4923_8983_5CC3F7D83B45
#define A73825E6_9EFA_4923_8983_5CC3F7D83B45

constexpr static std::uint32_t MAXIS_X = 0;
constexpr static std::uint32_t MAXIS_Y = 1;

constexpr static std::uint32_t MWHEEL_XUP = 0;
constexpr static std::uint32_t MWHEEL_XDOWN = 1;
constexpr static std::uint32_t MWHEEL_YUP = 2;
constexpr static std::uint32_t MWHEEL_YDOWN = 3;

enum class input_device : std::uint32_t {
    KEYBOARD,
    MOUSE,
    MWHEEL,
    GAMEPAD,
};

enum class input_edge : std::uint32_t {
    DOWN,
    UP,
};

class ButtonSource final {
public:
    constexpr ButtonSource(void) = default;
    constexpr ButtonSource(input_device device, std::uint32_t code, input_edge edge);

    constexpr input_device device(void) const;
    constexpr input_edge edge(void) const;
    constexpr std::uint32_t code(void) const;
    constexpr std::uint32_t value(void) const;

    constexpr bool operator==(const ButtonSource& other) const = default;

private:
    std::uint32_t m_value { 0 };
};

template<>
struct std::hash<ButtonSource> final {
    std::size_t operator()(const ButtonSource& source) const
    {
        return static_cast<std::size_t>(source.value());
    }
};

class AxisSource final {
public:
    constexpr AxisSource(void) = default;
    constexpr AxisSource(input_device device, std::uint32_t code);

    constexpr input_device device(void) const;
    constexpr std::uint32_t code(void) const;
    constexpr std::uint32_t value(void) const;

    constexpr bool operator==(const AxisSource& other) const = default;

private:
    std::uint32_t m_value { 0 };
};

template<>
struct std::hash<AxisSource> final {
    std::size_t operator()(const AxisSource& source) const
    {
        return static_cast<std::size_t>(source.value());
    }
};

namespace input
{
std::optional<std::uint32_t> parse_button_code(input_device device, std::string_view name);
std::optional<std::uint32_t> parse_axis_code(input_device device, std::string_view name);
std::string format_button_code(input_device device, std::uint32_t code);
std::string format_axis_code(input_device device, std::uint32_t code);
} // namespace input

namespace input
{
std::optional<input_edge> parse_edge(std::string_view name);
std::string_view format_edge(input_edge edge);
std::string_view format_button_cmd(input_device device);
std::string_view format_axis_cmd(input_device device);
} // namespace input

constexpr ButtonSource::ButtonSource(input_device device, std::uint32_t code, input_edge edge)
{
    m_value = 0;
    m_value |= static_cast<std::uint32_t>(device) << 28;
    m_value |= static_cast<std::uint32_t>(edge) << 27;
    m_value |= code & UINT32_C(0x07FFFFFF);
}

constexpr input_device ButtonSource::device(void) const
{
    return static_cast<input_device>(m_value >> 28);
}

constexpr input_edge ButtonSource::edge(void) const
{
    return static_cast<input_edge>((m_value >> 27) & 0x01);
}

constexpr std::uint32_t ButtonSource::code(void) const
{
    return m_value & UINT32_C(0x07FFFFFF);
}

constexpr std::uint32_t ButtonSource::value(void) const
{
    return m_value;
}

constexpr AxisSource::AxisSource(input_device device, std::uint32_t code)
{
    m_value = 0;
    m_value |= static_cast<std::uint32_t>(device) << 28;
    m_value |= code & UINT32_C(0x0FFFFFFF);
}

constexpr input_device AxisSource::device(void) const
{
    return static_cast<input_device>(m_value >> 28);
}

constexpr std::uint32_t AxisSource::code(void) const
{
    return m_value & UINT32_C(0x0FFFFFFF);
}

constexpr std::uint32_t AxisSource::value(void) const
{
    return m_value;
}

#endif /* A73825E6_9EFA_4923_8983_5CC3F7D83B45 */
