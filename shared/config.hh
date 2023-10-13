// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_CONFIG_HH
#define SHARED_CONFIG_HH
#include <shared/mixin.hh>
#include <shared/vfs.hh>
#include <stdint.h>

namespace config
{
class IVariable : public mixin::NonCopyable {
public:
    virtual ~IVariable() = default;
    virtual void from_string(const std::string &sval) = 0;
    virtual std::string to_string() const = 0;
};

class Boolean final : public IVariable {
public:
    Boolean() = default;
    Boolean(bool default_value);
    virtual void from_string(const std::string &sval) override;
    virtual std::string to_string() const override;
    void set_value(bool value);
    bool get_default_value() const;
    bool get_value() const;

private:
    bool default_value {};
    bool value {};
};

class String final : public IVariable {
public:
    String() = default;
    String(const std::string &default_value);
    virtual void from_string(const std::string &sval) override;
    virtual std::string to_string() const override;
    void set_value(const std::string &value);
    const std::string &get_default_value() const;
    const std::string &get_value() const;

private:
    std::string default_value {};
    std::string value {};
};

class Number final : public IVariable {
public:
    Number() = default;
    Number(double default_value);
    virtual void from_string(const std::string &sval) override;
    virtual std::string to_string() const override;
    void set_value(double value);
    double get_default_value() const;
    double get_value() const;

private:
    double default_value {};
    double value {};
};
} // namespace config

namespace config
{
void load_cmdline();
void load_file(const vfs::path_t &path);
void save_file(const vfs::path_t &path);
void add_variable(const std::string &name, IVariable &vref);
} // namespace config

#endif /* SHARED_CONFIG_HH */
