// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef DB638122_CA26_4AE4_B4AB_EC0F475DDDEA
#define DB638122_CA26_4AE4_B4AB_EC0F475DDDEA
#include <stdint.h>
#include <string>
#include <vector>

namespace crc64
{
uint64_t get(const void *data, size_t size);
uint64_t get(const std::vector<uint8_t> &data);
uint64_t get(const std::string &data);
} // namespace crc64

#endif/* DB638122_CA26_4AE4_B4AB_EC0F475DDDEA */
