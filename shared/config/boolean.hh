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
    value = atof(sval.c_str()) || !sval.compare("true");
}

inline const std::string config::Boolean::to_string() const
{
    if(value)
        return std::string{"true"};
    return std::string{"false"};
}

#endif /* SHARED_CONFIG_BOOLEAN_HH */
