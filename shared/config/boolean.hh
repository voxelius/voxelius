// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_CONFIG_BOOLEAN_HH
#define SHARED_CONFIG_BOOLEAN_HH
#include <shared/config/variable.hh>
#include <stdlib.h>

namespace config
{
class Boolean final : public IVariable {
public:
    Boolean() = default;
    Boolean(bool default_value);
    virtual void from_string(const std::string &sval) override;
    virtual const std::string to_string() const override;
    bool value {};
};
} // namespace config

inline config::Boolean::Boolean(bool default_value)
    : value{default_value}
{

}

inline void config::Boolean::from_string(const std::string &sval)
{
    value = (atof(sval.c_str()) || !sval.compare("true"));
}

inline const std::string config::Boolean::to_string() const
{
    if(value)
        return std::string{"true"};
    return std::string{"false"};
}

#endif /* SHARED_CONFIG_BOOLEAN_HH */
