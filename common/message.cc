// SPDX-License-Identifier: Zlib
// Copyright (C) 2024, Voxelius Contributors
#include <common/message.hh>
#include <mathlib/constexpr.hh>
#include <mathlib/floathacks.hh>

float Message::read_FP32(Message &message)
{
    return floathacks::uint32_to_float(Message::read_UI32(message));
}

std::int8_t Message::read_I8(Message &message)
{
    return static_cast<std::int8_t>(Message::read_UI8(message));
}

std::int16_t Message::read_I16(Message &message)
{
    return static_cast<std::int16_t>(Message::read_UI16(message));
}

std::int32_t Message::read_I32(Message &message)
{
    return static_cast<std::int32_t>(Message::read_UI32(message));
}

std::int64_t Message::read_I64(Message &message)
{
    return static_cast<std::int64_t>(Message::read_UI64(message));
}

std::uint8_t Message::read_UI8(Message &message)
{
    if((message.read_position + 1U) <= message.vector.size()) {
        std::uint8_t result = message.vector[message.read_position];
        message.read_position += 1U;
        return result;
    }

    message.read_position += 1U;
    return UINT8_MAX;
}

std::uint16_t Message::read_UI16(Message &message)
{
    if((message.read_position + 2U) <= message.vector.size()) {
        std::uint16_t result = UINT16_C(0x0000);
        result |= static_cast<std::uint16_t>(message.vector[message.read_position + 0U]) << 8U;
        result |= static_cast<std::uint16_t>(message.vector[message.read_position + 1U]) << 0U;
        message.read_position += 2U;
        return result;
    }

    message.read_position += 2U;
    return UINT16_MAX;
}

std::uint32_t Message::read_UI32(Message &message)
{
    if((message.read_position + 4U) <= message.vector.size()) {
        std::uint32_t result = UINT32_C(0x00000000);
        result |= static_cast<std::uint32_t>(message.vector[message.read_position + 0U]) << 24U;
        result |= static_cast<std::uint32_t>(message.vector[message.read_position + 1U]) << 16U;
        result |= static_cast<std::uint32_t>(message.vector[message.read_position + 2U]) << 8U;
        result |= static_cast<std::uint32_t>(message.vector[message.read_position + 3U]) << 0U;
        message.read_position += 4U;
        return result;
    }

    message.read_position += 4U;
    return UINT32_MAX;
}

std::uint64_t Message::read_UI64(Message &message)
{
    if((message.read_position + 8U) <= message.vector.size()) {
        std::uint64_t result = UINT64_C(0x0000000000000000);
        result |= static_cast<std::uint64_t>(message.vector[message.read_position + 0U]) << 56U;
        result |= static_cast<std::uint64_t>(message.vector[message.read_position + 1U]) << 48U;
        result |= static_cast<std::uint64_t>(message.vector[message.read_position + 2U]) << 40U;
        result |= static_cast<std::uint64_t>(message.vector[message.read_position + 3U]) << 32U;
        result |= static_cast<std::uint64_t>(message.vector[message.read_position + 4U]) << 24U;
        result |= static_cast<std::uint64_t>(message.vector[message.read_position + 5U]) << 16U;
        result |= static_cast<std::uint64_t>(message.vector[message.read_position + 6U]) << 8U;
        result |= static_cast<std::uint64_t>(message.vector[message.read_position + 7U]) << 0U;
        message.read_position += 8U;
        return result;
    }

    message.read_position += 8U;
    return UINT64_MAX;
}

std::string Message::read_string(Message &message)
{
    std::size_t size = Message::read_UI16(message);
    std::string result = std::string();

    for(std::size_t i = 0; i < size; ++i) {
        if(message.read_position < message.vector.size())
            result.push_back(message.vector[message.read_position]);
        message.read_position += 1U;
    }

    return std::move(result);
}

void Message::write_FP32(Message &message, float value)
{
    Message::write_UI32(message, floathacks::float_to_uint32(value));
}

void Message::write_I8(Message &message, std::int8_t value)
{
    Message::write_UI8(message, static_cast<std::uint8_t>(value));
}

void Message::write_I16(Message &message, std::int16_t value)
{
    Message::write_UI16(message, static_cast<std::uint16_t>(value));
}

void Message::write_I32(Message &message, std::int32_t value)
{
    Message::write_UI32(message, static_cast<std::uint32_t>(value));
}

void Message::write_I64(Message &message, std::int64_t value)
{
    Message::write_UI64(message, static_cast<std::uint64_t>(value));
}

void Message::write_UI8(Message &message, std::uint8_t value)
{
    message.vector.push_back(value);
}

void Message::write_UI16(Message &message, std::uint16_t value)
{
    message.vector.push_back(static_cast<std::uint8_t>((value & UINT16_C(0xFF00)) >> 8U));
    message.vector.push_back(static_cast<std::uint8_t>((value & UINT16_C(0x00FF)) >> 0U));
}

void Message::write_UI32(Message &message, std::uint32_t value)
{
    message.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0xFF000000)) >> 24U));
    message.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x00FF0000)) >> 16U));
    message.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x0000FF00)) >> 8U));
    message.vector.push_back(static_cast<std::uint8_t>((value & UINT32_C(0x000000FF)) >> 0U));
}

void Message::write_UI64(Message &message, std::uint64_t value)
{
    message.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0xFF00000000000000)) >> 56U));
    message.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x00FF000000000000)) >> 48U));
    message.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x0000FF0000000000)) >> 40U));
    message.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x000000FF00000000)) >> 32U));
    message.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x00000000FF000000)) >> 24U));
    message.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x0000000000FF0000)) >> 16U));
    message.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x000000000000FF00)) >> 8U));
    message.vector.push_back(static_cast<std::uint8_t>((value & UINT64_C(0x00000000000000FF)) >> 0U));
}

void Message::write_string(Message &message, const std::string &value)
{
    const std::size_t size = cxpr::min<std::size_t>(UINT16_MAX, value.size());
    Message::write_UI16(message, static_cast<std::uint16_t>(size));
    for(std::size_t i = 0; i < size; Message::write_UI8(message, value[i++]));
}

void Message::setup(Message &message)
{
    message.read_position = 0;
    message.vector.clear();
}

void Message::setup(Message &message, const void *data, std::size_t size)
{
    message.read_position = 0;
    const std::uint8_t *data_p = reinterpret_cast<const std::uint8_t *>(data);
    message.vector.assign(data_p, data_p + size);
}
