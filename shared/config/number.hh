// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024, Voxelius Contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
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
