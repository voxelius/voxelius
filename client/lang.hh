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
#ifndef CLIENT_LANG_HH
#define CLIENT_LANG_HH
#include <string>
#include <vector>

struct LangInfo final {
    std::string endonym {};
    std::string ietf {};
    std::string display {};
};

using lang_vector_t = std::vector<LangInfo>;
using lang_iterator_t = lang_vector_t::const_iterator;

namespace lang
{
void init();
void init_late();
void set(lang_iterator_t language);
const std::string &find(const std::string &key);
lang_iterator_t lookup(const std::string &ietf);
lang_iterator_t current();
lang_iterator_t cbegin();
lang_iterator_t cend();
} // namespace lang

#endif /* CLIENT_LANG_HH */
