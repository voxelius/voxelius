// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <string>

namespace cmdline
{
void append(int argc, char **argv);
void append(const std::string &option);
void append(const std::string &option, const std::string &argument);
bool get_value(const std::string &option, std::string &argument);
bool contains(const std::string &option);
} // namespace cmdline
