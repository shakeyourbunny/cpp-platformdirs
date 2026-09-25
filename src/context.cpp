// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/compose.h>
#include <platformdirs/context.h>
#include <platformdirs/validation.h>

namespace platformdirs::rules {

Result<std::string> home(const Context& c)
{
    const auto h = c.sys.home_directory();
    if (!h) {
        return std::unexpected(Error::environment(
            "no usable home directory: HOME is unset or unusable and the system lookup failed"));
    }
    if (!validation::is_usable_base(c.options.platform, *h)) {
        return std::unexpected(Error::environment(
            "home directory is not usable as a base directory: [" + Error::excerpt(*h) + "]"));
    }
    return compose::normalize_base(c.options.platform, *h);
}

std::optional<std::string> base_from_env(const Context& c, std::string_view name)
{
    const auto v = c.env.get(name);
    if (!v) {
        return std::nullopt;
    }
    // Python strips a single XDG value; a stray blank must not read as "unset".
    const std::string_view trimmed = validation::trim_whitespace(*v);
    if (!validation::is_usable_base(c.options.platform, trimmed)) {
        return std::nullopt;
    }
    return compose::normalize_base(c.options.platform, trimmed);
}

} // namespace platformdirs::rules
