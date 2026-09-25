// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#ifndef PLATFORMDIRS_UTF8_H
#define PLATFORMDIRS_UTF8_H

#include <cstddef>
#include <optional>
#include <string_view>

namespace platformdirs::utf8 {

struct Decoded {
    char32_t code_point = 0;
    std::size_t length = 0; // bytes consumed
};

// The code point at the start of s; nullopt on empty input or any malformed sequence
// (stray continuation, overlong, surrogate, above U+10FFFF) per RFC 3629, section 4.
std::optional<Decoded> decode_one(std::string_view s);

bool is_valid(std::string_view s);

// UTF-16 code units the string would need; nullopt when the input is
// not valid UTF-8.
std::optional<std::size_t> utf16_length(std::string_view s);

} // namespace platformdirs::utf8

#endif // PLATFORMDIRS_UTF8_H
