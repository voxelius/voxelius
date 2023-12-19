// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_LANG_HH
#define CLIENT_LANG_HH
#include <string>
#include <vector>

struct LangInfo final {
    std::string endonym {};
    std::string ietf {};
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
