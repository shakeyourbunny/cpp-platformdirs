// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#ifndef PLATFORMDIRS_ERROR_H
#define PLATFORMDIRS_ERROR_H

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace platformdirs {

enum class ErrorCode : std::uint8_t {
    InvalidName, // app name, author or version cannot be a path component
    PathTooLong, // the composed path exceeds the platform limit
    Environment, // no home directory, no usable base, no known folder
    Io,          // a file the library reads could not be read
};

struct Error {
    ErrorCode code = ErrorCode::Environment;
    std::string message;

    static Error invalid_name(std::string_view role, std::string_view value);
    static Error path_too_long(std::size_t length, std::size_t limit);
    static Error environment(std::string_view message);
    static Error io(std::string_view message);

    // A value made safe for a message: control bytes replaced, length capped.
    static std::string excerpt(std::string_view value);
};

template <typename T>
using Result = std::expected<T, Error>;
using Status = Result<void>;

} // namespace platformdirs

#endif // PLATFORMDIRS_ERROR_H
