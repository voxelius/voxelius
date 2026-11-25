#ifndef AFCA58AE_5EE6_4076_9D52_A981F7CCF330
#define AFCA58AE_5EE6_4076_9D52_A981F7CCF330

#include "client/input/source.hh"

struct AxisValue final {
    bool is_delta;
    float value;
};

namespace axes
{
void init(void);
} // namespace axes

namespace axes
{
void bind(AxisSource source, std::string slot, bool overwrite = true);
void unbind(AxisSource source);
void clear(void);
} // namespace axes

namespace axes
{
AxisValue consume(std::string_view slot);
} // namespace axes

namespace axes
{
void reset_all(void);
} // namespace axes

#endif /* AFCA58AE_5EE6_4076_9D52_A981F7CCF330 */
