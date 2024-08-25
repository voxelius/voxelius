// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct Message final {
    std::size_t read_position {};
    std::vector<std::uint8_t> vector {};
    
public:
    static float read_FP32(Message &message);
    static std::int8_t read_I8(Message &message);
    static std::int16_t read_I16(Message &message);
    static std::int32_t read_I32(Message &message);
    static std::int64_t read_I64(Message &message);
    static std::uint8_t read_UI8(Message &message);
    static std::uint16_t read_UI16(Message &message);
    static std::uint32_t read_UI32(Message &message);
    static std::uint64_t read_UI64(Message &message);
    static std::string read_string(Message &message);
    
public:
    static void write_FP32(Message &message, float value);
    static void write_I8(Message &message, std::int8_t value);
    static void write_I16(Message &message, std::int16_t value);
    static void write_I32(Message &message, std::int32_t value);
    static void write_I64(Message &message, std::int64_t value);
    static void write_UI8(Message &message, std::uint8_t value);
    static void write_UI16(Message &message, std::uint16_t value);
    static void write_UI32(Message &message, std::uint32_t value);
    static void write_UI64(Message &message, std::uint64_t value);
    static void write_string(Message &message, const std::string &value);

public:
    static void setup(Message &message);
    static void setup(Message &message, const void *data, std::size_t size);
};
