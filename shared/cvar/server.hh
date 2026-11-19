#ifndef EF7CB0AF_B842_4A55_8A1B_0B842C96512E
#define EF7CB0AF_B842_4A55_8A1B_0B842C96512E

#include "core/cvar/boolean.hh"
#include "core/cvar/number.hh"

namespace server_vars
{
extern cvar::Number<unsigned> day_length;
extern cvar::Number<unsigned> lod_distance;
} // namespace server_vars

namespace server_vars
{
std::uint32_t day_length_ticks(void);
} // namespace server_vars

namespace server_vars
{
void init(void);
} // namespace server_vars

#endif /* EF7CB0AF_B842_4A55_8A1B_0B842C96512E */
