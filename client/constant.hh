#ifndef B67C41CD_9717_4135_A86E_B36E54504CF1
#define B67C41CD_9717_4135_A86E_B36E54504CF1

#include "shared/constant.hh"

namespace constant
{
// The unit the interface is authored in; the GUI scale is how many of these fit on the screen at once
constexpr static int BASE_WIDTH = 320;
constexpr static int BASE_HEIGHT = 240;

// The window is never smaller than this, whatever the GUI scale is
constexpr static int MIN_WIDTH = 800;
constexpr static int MIN_HEIGHT = 600;
} // namespace constant

#endif /* B67C41CD_9717_4135_A86E_B36E54504CF1 */
