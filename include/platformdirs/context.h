// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#ifndef PLATFORMDIRS_CONTEXT_H
#define PLATFORMDIRS_CONTEXT_H

#include <platformdirs/error.h>
#include <platformdirs/options.h>
#include <platformdirs/sources.h>

#include <optional>
#include <string>
#include <string_view>

namespace platformdirs::rules {

// Holds references, not copies; the caller keeps the three objects alive.
struct Context {
    const EnvironmentSource& env;
    const SystemSource& sys;
    const Options& options;
};

// The home directory in generic form, or an Environment error when it
// is missing or is not an absolute path for the target platform.
Result<std::string> home(const Context& c);

// The trimmed value of name when it can serve as a base directory, normalized; nullopt
// for unset, blank, relative, overlong or control-character values (design decision 4).
std::optional<std::string> base_from_env(const Context& c, std::string_view name);

} // namespace platformdirs::rules

#endif // PLATFORMDIRS_CONTEXT_H
