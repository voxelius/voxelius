#ifndef F5A1B8D7_3C62_4E94_8E71_2F6D9A0B4C18
#define F5A1B8D7_3C62_4E94_8E71_2F6D9A0B4C18

#include "core/cvar/boolean.hh"
#include "core/cvar/number.hh"

namespace gamerules
{
extern cvar::Boolean allow_building;
extern cvar::Boolean daylight_cycle;
extern cvar::Boolean weather;
extern cvar::Number<float> reach_distance;
} // namespace gamerules

namespace gamerules
{
void init(void);
} // namespace gamerules

#endif /* F5A1B8D7_3C62_4E94_8E71_2F6D9A0B4C18 */
