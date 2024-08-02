// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#ifndef CLIENT_GUI_TEXT_ATTRIBUTE_HH
#define CLIENT_GUI_TEXT_ATTRIBUTE_HH

using TextAttributeBits = unsigned short;
constexpr static TextAttributeBits TEXT_NULL            = 0x0000U;
constexpr static TextAttributeBits TEXT_RANDOM          = 0x0001U;
constexpr static TextAttributeBits TEXT_BOLD            = 0x0002U;
constexpr static TextAttributeBits TEXT_STRIKETHROUGH   = 0x0004U;
constexpr static TextAttributeBits TEXT_UNDERLINE       = 0x0008U;
constexpr static TextAttributeBits TEXT_ITALIC          = 0x0010U;

#endif /* CLIENT_GUI_TEXT_ATTRIBUTE_HH */
