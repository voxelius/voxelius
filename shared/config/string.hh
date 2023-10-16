// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_CONFIG_STRING_HH
#define SHARED_CONFIG_STRING_HH
#include <shared/config/variable.hh>

namespace config
{
class String final : public IVariable {
public:
    String() = default;
    String(const std::string &default_value);
    virtual void from_string(const std::string &sval) override;
    virtual const std::string to_string() const override;
    std::string value {};
};
} // namespace config

inline config::String::String(const std::string &default_value)
    : value{default_value}
{

}

inline void config::String::from_string(const std::string &sval)
{
    value = sval;
}

inline const std::string config::String::to_string() const
{
    return value;
}

#endif /* SHARED_CONFIG_STRING_HH */
