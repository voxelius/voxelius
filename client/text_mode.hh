// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_TEXT_MODE_HH
#define CLIENT_TEXT_MODE_HH

using TextModeBits = unsigned short;
constexpr static TextModeBits TEXT_MODE_ASCII = 0x0001U; // Enable newlines
constexpr static TextModeBits TEXT_MODE_VT241 = 0x0002U; // Enable ANSI colors
constexpr static TextModeBits TEXT_MODE_NOTCH = 0x0004U; // https://wiki.vg/Text_formatting

#endif /* CLIENT_TEXT_MODE_HH */
