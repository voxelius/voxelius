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
template<typename T>
class Number final : public IVariable {
public:
    Number() = default;
    Number(T default_value);
    virtual void from_string(const std::string &sval) override;
    virtual const std::string to_string() const override;
    T value {};
};
} // namespace config

template<typename T>
inline config::Number<T>::Number(T default_value)
    : value{default_value}
{

}

template<typename T>
inline void config::Number<T>::from_string(const std::string &sval)
{
    std::istringstream iss = {};
    iss.str(sval);
    iss >> value;
}

template<typename T>
inline const std::string config::Number<T>::to_string() const
{
    return std::to_string(value);
}

#endif /* SHARED_CONFIG_NUMBER_HH */
