// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/error.h>
#include <platformdirs/platform.h>
#include <platformdirs/validation.h>

#include "utf8.h"

#include <string>

namespace platformdirs {

namespace {

constexpr std::size_t max_excerpt_bytes = 120;

// Printable for a log line: no C0, DEL or C1 control and nothing invisible.
bool printable(char32_t cp)
{
    if (cp < 0x20 || cp == 0x7F || (cp >= 0x80 && cp <= 0x9F)) {
        return false;
    }
    return !validation::is_invisible_or_directional(cp);
}

} // namespace

std::string_view platform_name(Platform p)
{
    switch (p) {
    case Platform::Linux: return "linux";
    case Platform::FreeBsd: return "freebsd";
    case Platform::OpenBsd: return "openbsd";
    case Platform::MacOS: return "macos";
    case Platform::Windows: return "windows";
    }
    return "unknown";
}

std::string Error::excerpt(std::string_view value)
{
    // Messages end up in logs and terminals; escapes, invisible text and a
    // megabyte of input must not get there unchanged. Cuts on a character boundary.
    std::string out;
    std::string_view rest = value;
    while (!rest.empty() && out.size() < max_excerpt_bytes) {
        const auto d = utf8::decode_one(rest);
        if (!d) {
            out += '?';
            rest.remove_prefix(1);
            continue;
        }
        if (out.size() + d->length > max_excerpt_bytes) {
            break;
        }
        if (printable(d->code_point)) {
            out.append(rest.substr(0, d->length));
        } else {
            out += '?';
        }
        rest.remove_prefix(d->length);
    }
    if (!rest.empty()) {
        out += "...";
    }
    return out;
}

Error Error::invalid_name(std::string_view role, std::string_view value)
{
    std::string message = "the ";
    message += role;
    message += " is not usable as a path component: [";
    message += excerpt(value);
    message += "]";
    return Error{ErrorCode::InvalidName, std::move(message)};
}

Error Error::path_too_long(std::size_t length, std::size_t limit)
{
    std::string message = "resolved path is ";
    message += std::to_string(length);
    message += " units long, the limit for this platform is ";
    message += std::to_string(limit);
    return Error{ErrorCode::PathTooLong, std::move(message)};
}

Error Error::environment(std::string_view message)
{
    return Error{ErrorCode::Environment, std::string(message)};
}

Error Error::io(std::string_view message)
{
    return Error{ErrorCode::Io, std::string(message)};
}

} // namespace platformdirs
