// SPDX-License-Identifier: Zlib
// Copyright (c) 2024, Voxelius Contributors
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
