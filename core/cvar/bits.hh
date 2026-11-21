#ifndef ECA8FF40_9CD6_4040_A456_6ABB22F0036D
#define ECA8FF40_9CD6_4040_A456_6ABB22F0036D

namespace cvar::detail
{
enum flag_index : unsigned short {
    IDX_ADMIN,
    IDX_READONLY,
    IDX_ARCHIVE,
    IDX_REPLICATED,
    IDX_NOTIFY,
    IDX_HIDDEN,
    IDX_RECALL,
};
} // namespace cvar::detail

namespace cvar
{
using flag_bits = std::uint16_t;
} // namespace cvar

namespace cvar
{
template<detail::flag_index Index>
constexpr static flag_bits FLAG_BIT = 1 << Index;
} // namespace cvar

constexpr static auto FCVAR_NONE = static_cast<cvar::flag_bits>(0x0000);
constexpr static auto FCVAR_ADMIN = cvar::FLAG_BIT<cvar::detail::IDX_ADMIN>;
constexpr static auto FCVAR_READONLY = cvar::FLAG_BIT<cvar::detail::IDX_READONLY>;
constexpr static auto FCVAR_ARCHIVE = cvar::FLAG_BIT<cvar::detail::IDX_ARCHIVE>;
constexpr static auto FCVAR_REPLICATED = cvar::FLAG_BIT<cvar::detail::IDX_REPLICATED>;
constexpr static auto FCVAR_NOTIFY = cvar::FLAG_BIT<cvar::detail::IDX_NOTIFY>;
constexpr static auto FCVAR_HIDDEN = cvar::FLAG_BIT<cvar::detail::IDX_HIDDEN>;
constexpr static auto FCVAR_RECALL = cvar::FLAG_BIT<cvar::detail::IDX_RECALL>;
constexpr static auto FCVAR_MASK = static_cast<cvar::flag_bits>(0xFFFF);

namespace cvar
{
void make_bitstring_short(flag_bits bits, std::string& out);
void make_bitstring_long(flag_bits bits, std::string& out);
} // namespace cvar

#endif /* ECA8FF40_9CD6_4040_A456_6ABB22F0036D */
