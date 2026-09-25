// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#ifndef PLATFORMDIRS_VALIDATION_H
#define PLATFORMDIRS_VALIDATION_H

#include <platformdirs/error.h>
#include <platformdirs/platform.h>

#include <string_view>

// Hardening for the two untrusted inputs: application metadata supplied
// by the caller, and directory values supplied by the environment.
namespace platformdirs::validation {

// True when s is safe as one path component on every supported platform, per design
// decision 3 (the refused set is listed there). The empty string means "not supplied".
bool is_valid_component(std::string_view s);

// is_valid_component as a Status; the InvalidName message names the role ("app name").
Status check_component(std::string_view s, std::string_view role);

// Absolute for the given platform. Windows accepts a drive-rooted or UNC path and
// rejects "C:foo", "\foo" and the device namespaces.
bool is_absolute(Platform p, std::string_view s);

// True when a value can serve as a base directory: not blank, valid UTF-8, no control
// characters, within the path limit, absolute. Otherwise it reads as unset (XDG spec).
bool is_usable_base(Platform p, std::string_view s);

// NUL, C0 controls and DEL. Bytes above 0x7F are not controls here;
// the C1 range is a code point question handled by is_valid_component.
bool contains_control_character(std::string_view s);

// Zero-width characters, BiDi controls, joiners, BOM and the Tags block.
bool is_invisible_or_directional(char32_t cp);

// Leading and trailing whitespace removed, the set Python's str.strip() uses
// (ASCII whitespace plus the Unicode spaces and separators).
std::string_view trim_whitespace(std::string_view s);

// PathTooLong when a composed path exceeds the platform limit.
// Windows counts UTF-16 units, POSIX counts bytes.
Status check_path_length(Platform p, std::string_view s);

} // namespace platformdirs::validation

#endif // PLATFORMDIRS_VALIDATION_H
