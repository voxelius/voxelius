// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <shared/identifier.hh>
#include <shared/strtools.hh>
#include <spdlog/fmt/fmt.h>

Identifier::Identifier(const std::string &full)
{
    set(full);
}

Identifier::Identifier(const std::string &space, const std::string &value)
    : space{space}, value{value}
{

}

const std::string Identifier::get() const
{
    return space + ":" + value;
}

const std::string &Identifier::get_namespace() const
{
    return space;
}

const std::string &Identifier::get_value() const
{
    return value;
}

void Identifier::set_namespace(const std::string &space)
{
    this->space = space;
}

void Identifier::set_value(const std::string &value)
{
    this->value = value;
}

void Identifier::set(const std::string &full)
{
    const std::vector<std::string> parts = util::split(full, ":");

    if(parts.size() < 2) {
        this->space = DEFAULT_NAMESPACE;
        this->value = parts[0];
    }
    else {
        this->space = parts[0];
        this->value = parts[1];
    }
}

std::string Identifier::get_file_path() const
{
    return fmt::format("/{}/{}", space, value);
}

std::string Identifier::get_file_path(const std::string &subdirectory) const
{
    return fmt::format("/{}/{}/{}", space, subdirectory, value);
}

std::string Identifier::get_file_path_ext(const std::string &ext) const
{
    return fmt::format("/{}/{}.{}", space, value, ext);
}

std::string Identifier::get_file_path_ext(const std::string &subdirectory, const std::string &ext) const
{
    return fmt::format("/{}/{}/{}.{}", space, subdirectory, value, ext);
}
