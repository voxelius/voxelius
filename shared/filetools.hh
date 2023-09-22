// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef B6FB9749_445F_475B_A8EF_8F59D88067DB
#define B6FB9749_445F_475B_A8EF_8F59D88067DB
#include <physfs.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace filetools
{
bool read(const std::string &path, std::string &out);
bool read(const std::string &path, std::vector<uint8_t> &out);
} // namespace filetools

#endif/* B6FB9749_445F_475B_A8EF_8F59D88067DB */
