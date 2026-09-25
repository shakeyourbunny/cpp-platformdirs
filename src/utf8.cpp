// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include "utf8.h"

namespace platformdirs::utf8 {

namespace {

constexpr bool is_continuation(unsigned char b)
{
    return (b & 0xC0) == 0x80;
}

} // namespace

std::optional<Decoded> decode_one(std::string_view s)
{
    if (s.empty()) {
        return std::nullopt;
    }
    const auto b0 = static_cast<unsigned char>(s[0]);
    if (b0 < 0x80) {
        return Decoded{b0, 1};
    }

    // RFC 3629 table: the second byte's range depends on the lead byte
    // so that overlong forms and surrogates cannot be expressed.
    std::size_t length = 0;
    unsigned char second_min = 0x80;
    unsigned char second_max = 0xBF;
    char32_t cp = 0;
    if (b0 >= 0xC2 && b0 <= 0xDF) {
        length = 2;
        cp = b0 & 0x1F;
    } else if (b0 >= 0xE0 && b0 <= 0xEF) {
        length = 3;
        cp = b0 & 0x0F;
        if (b0 == 0xE0) {
            second_min = 0xA0;
        } else if (b0 == 0xED) {
            second_max = 0x9F;
        }
    } else if (b0 >= 0xF0 && b0 <= 0xF4) {
        length = 4;
        cp = b0 & 0x07;
        if (b0 == 0xF0) {
            second_min = 0x90;
        } else if (b0 == 0xF4) {
            second_max = 0x8F;
        }
    } else {
        return std::nullopt; // 0x80 to 0xC1, 0xF5 to 0xFF
    }

    if (s.size() < length) {
        return std::nullopt;
    }
    const auto b1 = static_cast<unsigned char>(s[1]);
    if (b1 < second_min || b1 > second_max) {
        return std::nullopt;
    }
    cp = (cp << 6) | (b1 & 0x3F);
    for (std::size_t i = 2; i < length; ++i) {
        const auto b = static_cast<unsigned char>(s[i]);
        if (!is_continuation(b)) {
            return std::nullopt;
        }
        cp = (cp << 6) | (b & 0x3F);
    }
    return Decoded{cp, length};
}

bool is_valid(std::string_view s)
{
    while (!s.empty()) {
        const auto d = decode_one(s);
        if (!d) {
            return false;
        }
        s.remove_prefix(d->length);
    }
    return true;
}

std::optional<std::size_t> utf16_length(std::string_view s)
{
    std::size_t units = 0;
    while (!s.empty()) {
        const auto d = decode_one(s);
        if (!d) {
            return std::nullopt;
        }
        units += d->code_point >= 0x10000 ? 2 : 1;
        s.remove_prefix(d->length);
    }
    return units;
}

} // namespace platformdirs::utf8
