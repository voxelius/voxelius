// SPDX-License-Identifier: Zlib
#ifndef SHARED_CONFIG_HH
#define SHARED_CONFIG_HH
#include <string>

namespace config
{
void add(const std::string &name, int &vref);
void add(const std::string &name, bool &vref);
void add(const std::string &name, double &vref);
void add(const std::string &name, unsigned int &vref);
void add(const std::string &name, std::string &vref);
} // namespace config

namespace config
{
void load(const std::string &path);
void save(const std::string &path);
} // namespace config

#endif /* SHARED_CONFIG_HH */
