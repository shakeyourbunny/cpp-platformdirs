// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#ifndef PLATFORMDIRS_COMPOSE_H
#define PLATFORMDIRS_COMPOSE_H

#include <platformdirs/error.h>
#include <platformdirs/options.h>
#include <platformdirs/platform.h>

#include <string>
#include <string_view>
#include <vector>

// Path text building, pure functions of their arguments. Text stays in generic form
// (forward slashes); render() writes the target platform's separator.
namespace platformdirs::compose {

// Folds Windows separators to '/' when the target is Windows, drops
// trailing separators without destroying a bare root ("/" or "C:/").
std::string normalize_base(Platform p, std::string_view base);

// base/name/version, where the version only appears if the name does.
// Length-checked for p.
Result<std::string> append_app_parts(Platform p, std::string_view base, const Options& o);

// base/author/name/segment/version, length-checked for Windows. Nothing is appended without
// an app name (Python's rule); the segment lands only with o.opinion, before the version.
Result<std::string> append_windows_parts(std::string_view base, const Options& o,
                                         std::string_view opinion_segment = {});

// One more component on the end, length-checked for p.
Result<std::string> append(Platform p, std::string_view base, std::string_view component);

// Splits a search path such as XDG_DATA_DIRS on the platform's list separator; entries
// that are empty or not usable as a base are dropped, the rest are trimmed and normalized.
std::vector<std::string> split_path_list(Platform p, std::string_view list);

// Every '/' written the way p writes it. POSIX text is returned
// unchanged, since a backslash is an ordinary character there.
std::string render(Platform p, std::string_view generic);

} // namespace platformdirs::compose

#endif // PLATFORMDIRS_COMPOSE_H
