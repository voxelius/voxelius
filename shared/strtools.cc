// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <shared/strtools.hh>

std::vector<std::string> util::split(const std::string &value, const std::string &delim)
{
    size_t cpos = 0;
    size_t lpos = 0;
    std::vector<std::string> vec = {};

    while((cpos = value.find(delim, lpos)) != std::string::npos) {
        vec.push_back(value.substr(lpos, cpos - lpos));
        lpos = cpos + delim.length();
    }

    if(lpos <= value.length()) {
        // We lack the last element
        vec.push_back(value.substr(lpos, value.length() - lpos));
    }

    return vec;
}
