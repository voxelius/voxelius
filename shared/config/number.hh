// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_CONFIG_NUMBER_HH
#define SHARED_CONFIG_NUMBER_HH
#include <shared/config/variable.hh>
#include <sstream>

namespace config
{
template<typename value_type>
class Number final : public IVariable {
public:
    Number() = default;
    Number(value_type default_value);
    virtual void from_string(const std::string &sval) override;
    virtual const std::string to_string() const override;
    value_type value {};
};
} // namespace config

namespace config
{
using Double = Number<double>;
using Integer = Number<int>;
} // namespace config

template<typename value_type>
inline config::Number<value_type>::Number(value_type default_value)
    : value{default_value}
{

}

template<typename value_type>
inline void config::Number<value_type>::from_string(const std::string &sval)
{
    std::istringstream iss = {};
    iss.str(sval);
    iss >> value;
}

template<typename value_type>
inline const std::string config::Number<value_type>::to_string() const
{
    return std::to_string(value);
}

#endif /* SHARED_CONFIG_NUMBER_HH */
