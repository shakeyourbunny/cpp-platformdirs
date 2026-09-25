// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/validation.h>

#include "utf8.h"

#include <algorithm>
#include <array>
#include <string>

namespace platformdirs::validation {

namespace {

// Characters Windows reserves in a file name. Separators and NUL are
// checked separately so the reason is the right one.
constexpr std::string_view reserved_characters = "<>\"?*|:";

// Fullwidth solidus and reverse solidus: not separators to any file
// system, but they read as separators to a person.
bool is_lookalike_separator(char32_t cp)
{
    return cp == 0xFF0F || cp == 0xFF3C;
}

bool is_c1_control(char32_t cp)
{
    return cp >= 0x80 && cp <= 0x9F;
}

// ASCII only, independent of the C locale; bytes above 0x7F pass through.
std::string upper_ascii(std::string_view s)
{
    std::string out(s);
    for (char& c : out) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }
    return out;
}

// The stem before the first dot, trailing spaces stripped as Windows strips them, against
// the reserved names of Microsoft's "Naming Files, Paths, and Namespaces" page.
bool is_device_stem(std::string_view stem)
{
    static constexpr std::array<std::string_view, 6> fixed = {"CON", "PRN",    "AUX",
                                                              "NUL", "CONIN$", "CONOUT$"};
    while (!stem.empty() && stem.back() == ' ') {
        stem.remove_suffix(1);
    }
    const std::string upper = upper_ascii(stem);
    if (std::ranges::find(fixed, upper) != fixed.end()) {
        return true;
    }
    if (!upper.starts_with("COM") && !upper.starts_with("LPT")) {
        return false;
    }
    const std::string_view rest = std::string_view(upper).substr(3);
    if (rest.size() == 1 && rest[0] >= '0' && rest[0] <= '9') {
        return true;
    }
    // U+00B9, U+00B2, U+00B3 in UTF-8.
    return rest == "\xC2\xB9" || rest == "\xC2\xB2" || rest == "\xC2\xB3";
}

// Python's str.strip() set: ASCII whitespace, C1 NEL, and the Unicode space separators.
bool is_python_whitespace(char32_t cp)
{
    return cp == ' ' || (cp >= 0x09 && cp <= 0x0D) || (cp >= 0x1C && cp <= 0x1F) || cp == 0x85 ||
           cp == 0xA0 || cp == 0x1680 || (cp >= 0x2000 && cp <= 0x200A) || cp == 0x2028 ||
           cp == 0x2029 || cp == 0x202F || cp == 0x205F || cp == 0x3000;
}

} // namespace

// A name carrying one of these looks like another name on screen.
bool is_invisible_or_directional(char32_t cp)
{
    return (cp >= 0x200B && cp <= 0x200F) || (cp >= 0x202A && cp <= 0x202E) ||
           (cp >= 0x2060 && cp <= 0x2064) || (cp >= 0x2066 && cp <= 0x2069) || cp == 0xFEFF ||
           cp == 0x061C || cp == 0x180E || (cp >= 0xE0000 && cp <= 0xE007F);
}

std::string_view trim_whitespace(std::string_view s)
{
    while (!s.empty()) {
        const auto d = utf8::decode_one(s);
        if (!d || !is_python_whitespace(d->code_point)) {
            break;
        }
        s.remove_prefix(d->length);
    }
    while (!s.empty()) {
        // Walk back to the start of the last code point, at most four bytes.
        std::size_t start = s.size() - 1;
        while (start > 0 && s.size() - start < 4 &&
               (static_cast<unsigned char>(s[start]) & 0xC0) == 0x80) {
            --start;
        }
        const auto d = utf8::decode_one(s.substr(start));
        if (!d || d->length != s.size() - start || !is_python_whitespace(d->code_point)) {
            break;
        }
        s.remove_suffix(d->length);
    }
    return s;
}

bool contains_control_character(std::string_view s)
{
    return std::ranges::any_of(s, [](char c) {
        const auto u = static_cast<unsigned char>(c);
        return u < 0x20 || u == 0x7F;
    });
}

bool is_valid_component(std::string_view s)
{
    if (s.empty()) {
        return true;
    }
    if (s.size() > max_component_bytes) {
        return false;
    }
    if (s == "." || s == "..") {
        return false;
    }
    if (contains_control_character(s)) {
        return false;
    }
    if (s.front() == ' ' || s.back() == ' ' || s.back() == '.') {
        return false;
    }

    std::string_view rest = s;
    while (!rest.empty()) {
        const auto d = utf8::decode_one(rest);
        if (!d) {
            return false;
        }
        const char32_t cp = d->code_point;
        if (cp == U'/' || cp == U'\\') {
            return false;
        }
        if (cp < 0x80 &&
            reserved_characters.find(static_cast<char>(cp)) != std::string_view::npos) {
            return false;
        }
        if (is_c1_control(cp) || is_invisible_or_directional(cp) || is_lookalike_separator(cp)) {
            return false;
        }
        rest.remove_prefix(d->length);
    }

    const std::size_t dot = s.find('.');
    const std::string_view stem = dot == std::string_view::npos ? s : s.substr(0, dot);
    return !is_device_stem(stem);
}

Status check_component(std::string_view s, std::string_view role)
{
    if (!is_valid_component(s)) {
        return std::unexpected(Error::invalid_name(role, s));
    }
    return {};
}

bool is_absolute(Platform p, std::string_view s)
{
    if (s.empty()) {
        return false;
    }
    if (p != Platform::Windows) {
        return s.front() == '/';
    }
    const auto is_sep = [](char c) { return c == '\\' || c == '/'; };
    if (s.size() > 2 && is_sep(s[0]) && is_sep(s[1]) && !is_sep(s[2])) {
        // UNC needs a server and a share; the "." and "?" device namespaces are not shares.
        const std::size_t end = s.find_first_of("\\/", 2);
        if (end == std::string_view::npos || end + 1 >= s.size() || is_sep(s[end + 1])) {
            return false;
        }
        const std::string_view server = s.substr(2, end - 2);
        return server != "." && server != "?";
    }
    // Drive rooted: a letter, a colon, a separator. "C:foo" is drive
    // relative and does not count.
    const auto is_letter = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); };
    return s.size() > 2 && is_letter(s[0]) && s[1] == ':' && is_sep(s[2]);
}

bool is_usable_base(Platform p, std::string_view s)
{
    if (s.empty()) {
        return false;
    }
    if (std::ranges::all_of(s, [](char c) { return c == ' ' || c == '\t'; })) {
        return false;
    }
    if (contains_control_character(s) || !utf8::is_valid(s)) {
        return false;
    }
    if (!check_path_length(p, s).has_value()) {
        return false;
    }
    return is_absolute(p, s);
}

Status check_path_length(Platform p, std::string_view s)
{
    if (p == Platform::Windows) {
        // Malformed input cannot be measured in UTF-16 units; the byte
        // count is the conservative substitute.
        const auto units = utf8::utf16_length(s);
        const std::size_t length = units ? *units : s.size();
        if (length > max_path_units_windows) {
            return std::unexpected(Error::path_too_long(length, max_path_units_windows));
        }
        return {};
    }
    if (s.size() > max_path_bytes_posix) {
        return std::unexpected(Error::path_too_long(s.size(), max_path_bytes_posix));
    }
    return {};
}

} // namespace platformdirs::validation
