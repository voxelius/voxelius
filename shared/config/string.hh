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
