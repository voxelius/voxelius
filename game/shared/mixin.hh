// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef D20B1A85_CFA7_4530_BCDC_BFD97A28F09E
#define D20B1A85_CFA7_4530_BCDC_BFD97A28F09E

namespace mixin
{
class NonCopyable {
public:
    NonCopyable() = default;
    NonCopyable(const NonCopyable &rhs) = delete;
    NonCopyable &operator=(const NonCopyable &rhs) = delete;
};

class NonMovable {
    NonMovable() = default;
    NonMovable(NonMovable &&rhs) = delete;
    NonMovable &operator=(NonMovable &&rhs) = delete;
};
} // namespace mixin

#endif /* D20B1A85_CFA7_4530_BCDC_BFD97A28F09E */
