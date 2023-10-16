// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_CONFIG_VARIABLE_HH
#define SHARED_CONFIG_VARIABLE_HH
#include <string>

namespace config
{
class IVariable {
public:
    virtual ~IVariable() = default;
    virtual void from_string(const std::string &sval) = 0;
    virtual const std::string to_string() const = 0;
};
} // namespace config

namespace config
{
class Boolean;
template<typename T>
class Number;
class String;
} // namespace config

#endif /* SHARED_CONFIG_VARIABLE_HH */
