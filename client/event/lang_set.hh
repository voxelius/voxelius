// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_EVENT_LANG_SET_HH
#define CLIENT_EVENT_LANG_SET_HH
#include <client/lang.hh>

struct LangSetEvent final {
    lang_iterator_t it {};
};

#endif /* CLIENT_EVENT_LANG_SET_HH */
