#ifndef BB08B180_E85C_4711_9925_4A70EC106E2F
#define BB08B180_E85C_4711_9925_4A70EC106E2F

namespace vx
{
template<typename... Args>
struct FormatWithLocation final {
    template<typename StringT>
    consteval FormatWithLocation(const StringT& value, std::source_location location = std::source_location::current())
        : value(value), location(std::move(location))
    {
        // empty
    }

    std::format_string<Args...> value;
    std::source_location location;
};
} // namespace vx

#endif /* BB08B180_E85C_4711_9925_4A70EC106E2F */
