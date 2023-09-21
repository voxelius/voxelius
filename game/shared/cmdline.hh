// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CFA3E605_983E_426D_9328_7839B0D8FE71
#define CFA3E605_983E_426D_9328_7839B0D8FE71
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

#endif /* CFA3E605_983E_426D_9328_7839B0D8FE71 */
