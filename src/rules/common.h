// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#ifndef PLATFORMDIRS_RULES_COMMON_H
#define PLATFORMDIRS_RULES_COMMON_H

#include <platformdirs/context.h>

#include <string>
#include <string_view>
#include <vector>

// Helpers shared by the POSIX rule sets. Not installed.
namespace platformdirs::rules::detail {

Platform target(const Context& c);

// The variable when usable as a base, else home plus the default.
Result<std::string> xdg_base(const Context& c, std::string_view variable,
                             std::string_view default_under_home);

// xdg_base with the app name and version appended.
Result<std::string> under_base(const Context& c, std::string_view variable,
                               std::string_view default_under_home);

// A fixed base with the app name and version appended.
Result<std::string> under_fixed(const Context& c, std::string_view base);

// home/suffix, no app parts.
Result<std::string> under_home(const Context& c, std::string_view suffix);

// home/suffix with the app name and version appended.
Result<std::string> under_home_app(const Context& c, std::string_view suffix);

// A search path variable split on the list separator, or the fallback list, with either
// the app parts or a fixed subdirectory appended to each entry.
Result<std::vector<std::string>> search_path(const Context& c, std::string_view variable,
                                             std::string_view fallback, bool app_parts,
                                             std::string_view subdirectory = {});

} // namespace platformdirs::rules::detail

#endif // PLATFORMDIRS_RULES_COMMON_H
