// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GAMEUI_LANGUAGE_HH
#define CLIENT_GAMEUI_LANGUAGE_HH
#include <string>
#include <vector>

struct LangInfo final {
    std::string endonym {};
    std::string display {};
    std::string ietf {};
};

using LangManifest = std::vector<LangInfo>;
using LangIterator = LangManifest::const_iterator;

namespace language
{
void init(void);
void init_late(void);
} // namespace language

namespace language
{
void set(LangIterator iter);
LangIterator find(const std::string &ietf);
LangIterator current(void);
LangIterator cbegin(void);
LangIterator cend(void);
} // namespace language

namespace language
{
const std::string &resolve(const std::string &tag);
const std::string resolve_ui(const std::string &tag);
} // namespace language

#endif /* CLIENT_GAMEUI_LANGUAGE_HH */
