// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef SHARED_CMDLINE_HH
#define SHARED_CMDLINE_HH
#include <string>

namespace cmdline
{
void append(int argc, char **argv);
void append(const std::string &key);
void append(const std::string &key, const std::string &value);
bool contains(const std::string &key);
bool has_value(const std::string &key);
bool get_value(const std::string &key, std::string &value);
} // namespace cmdline

#endif /* SHARED_CMDLINE_HH */
