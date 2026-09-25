// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include "rules/common.h"

#include <platformdirs/compose.h>

namespace platformdirs::rules::detail {

using compose::append;
using compose::append_app_parts;

Platform target(const Context& c)
{
    return c.options.platform;
}

Result<std::string> xdg_base(const Context& c, std::string_view variable,
                             std::string_view default_under_home)
{
    if (auto from_env = base_from_env(c, variable)) {
        return *from_env;
    }
    return under_home(c, default_under_home);
}

Result<std::string> under_base(const Context& c, std::string_view variable,
                               std::string_view default_under_home)
{
    const auto base = xdg_base(c, variable, default_under_home);
    if (!base) {
        return std::unexpected(base.error());
    }
    return append_app_parts(target(c), *base, c.options);
}

Result<std::string> under_fixed(const Context& c, std::string_view base)
{
    return append_app_parts(target(c), base, c.options);
}

Result<std::string> under_home(const Context& c, std::string_view suffix)
{
    const auto h = home(c);
    if (!h) {
        return std::unexpected(h.error());
    }
    return append(target(c), *h, suffix);
}

Result<std::string> under_home_app(const Context& c, std::string_view suffix)
{
    const auto base = under_home(c, suffix);
    if (!base) {
        return std::unexpected(base.error());
    }
    return append_app_parts(target(c), *base, c.options);
}

Result<std::vector<std::string>> search_path(const Context& c, std::string_view variable,
                                             std::string_view fallback, bool app_parts,
                                             std::string_view subdirectory)
{
    const Platform p = target(c);
    const auto resolve = [&](std::string_view list) {
        std::vector<std::string> result;
        for (const std::string& entry : compose::split_path_list(p, list)) {
            const auto path =
                app_parts ? append_app_parts(p, entry, c.options) : append(p, entry, subdirectory);
            // An entry that overflows the limit is dropped like an unusable one.
            if (path) {
                result.push_back(*path);
            }
        }
        return result;
    };
    if (const auto raw = c.env.get(variable)) {
        if (auto from_env = resolve(*raw); !from_env.empty()) {
            return from_env;
        }
    }
    // A variable that yields nothing usable is treated as unset.
    return resolve(fallback);
}

} // namespace platformdirs::rules::detail
