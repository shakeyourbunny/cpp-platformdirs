// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#ifndef PLATFORMDIRS_PLATFORM_H
#define PLATFORMDIRS_PLATFORM_H

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace platformdirs {

enum class Platform : std::uint8_t { Linux, FreeBsd, OpenBsd, MacOS, Windows };

// The platform this library was compiled for. NetBSD and DragonFly
// builds report FreeBsd; the rules are the same.
constexpr Platform host_platform()
{
#if defined(_WIN32)
    return Platform::Windows;
#elif defined(__APPLE__)
    return Platform::MacOS;
#elif defined(__OpenBSD__)
    return Platform::OpenBsd;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
    return Platform::FreeBsd;
#else
    return Platform::Linux;
#endif
}

constexpr bool is_posix(Platform p)
{
    return p != Platform::Windows;
}

constexpr bool is_unix_like(Platform p)
{
    return p == Platform::Linux || p == Platform::FreeBsd || p == Platform::OpenBsd;
}

// POSIX NAME_MAX; it also bounds the Windows limit of 255 UTF-16 units,
// since no UTF-8 string needs more units than it has bytes.
constexpr std::size_t max_component_bytes = 255;
constexpr std::size_t max_path_bytes_posix = 4096;
constexpr std::size_t max_path_units_windows = 32767;

constexpr char separator(Platform p)
{
    return p == Platform::Windows ? '\\' : '/';
}

constexpr char list_separator(Platform p)
{
    return p == Platform::Windows ? ';' : ':';
}

std::string_view platform_name(Platform p);

} // namespace platformdirs

#endif // PLATFORMDIRS_PLATFORM_H
