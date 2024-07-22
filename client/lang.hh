// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_LANG_HH
#define CLIENT_LANG_HH
#include <string>
#include <vector>

struct LangInfo final {
    std::string endonym {};
    std::string display {};
    std::string ietf {};
};

using LangManifest = std::vector<LangInfo>;
using LangIterator = LangManifest::const_iterator;

namespace lang
{
void init(void);
void init_late(void);
} // namespace lang

namespace lang
{
void set(LangIterator language);
LangIterator find(const std::string &ietf);
LangIterator current(void);
LangIterator cbegin(void);
LangIterator cend(void);
} // namespace lang

namespace lang
{
const std::string &resolve(const std::string &tag);
} // namespace lang

#endif /* CLIENT_LANG_HH */
