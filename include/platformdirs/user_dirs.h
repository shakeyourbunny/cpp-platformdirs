// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#ifndef PLATFORMDIRS_USER_DIRS_H
#define PLATFORMDIRS_USER_DIRS_H

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

// The user-dirs.dirs file of xdg-user-dirs: sh syntax, one
// XDG_*_DIR="$HOME/..." assignment per line, sourced by xdg-user-dir.
namespace platformdirs::user_dirs {

constexpr std::size_t max_lines = 4096;

// The last usable value assigned to key, read as the shell reads the file: "$HOME"
// expanded, keys case-sensitive, relative values and lines past max_lines ignored.
std::optional<std::string> lookup(std::string_view content, std::string_view key,
                                  std::string_view home);

} // namespace platformdirs::user_dirs

#endif // PLATFORMDIRS_USER_DIRS_H
