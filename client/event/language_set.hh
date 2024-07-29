// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_EVENT_LANGUAGE_SET_HH
#define CLIENT_EVENT_LANGUAGE_SET_HH
#include <client/gui/language.hh>

struct LanguageSetEvent final {
    LangIterator language {};
};

#endif /* CLIENT_EVENT_LANGUAGE_SET_HH */
