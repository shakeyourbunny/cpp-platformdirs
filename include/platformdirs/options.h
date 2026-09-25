// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#ifndef PLATFORMDIRS_OPTIONS_H
#define PLATFORMDIRS_OPTIONS_H

#include <platformdirs/platform.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace platformdirs {

// The author segment of a Windows path, mirroring Python's appauthor: None (the app
// name), False (no segment), or a name.
class Author {
public:
    enum class Mode : std::uint8_t { DefaultToAppName, Disabled, Explicit };

    Author() = default;
    static Author app_name_default() { return Author(Mode::DefaultToAppName, {}); }
    static Author none() { return Author(Mode::Disabled, {}); }
    static Author named(std::string_view name) { return Author(Mode::Explicit, std::string(name)); }

    Mode mode() const { return mode_; }
    const std::string& name() const { return name_; }

private:
    Author(Mode mode, std::string name) : mode_(mode), name_(std::move(name)) {}

    Mode mode_ = Mode::DefaultToAppName;
    std::string name_;
};

struct Options {
    std::string app_name;
    Author author;
    std::string version;
    bool roaming = false;           // Windows: use the roaming profile
    bool opinion = true;            // add the Cache, Logs and log segments
    bool use_site_for_root = false; // Unix: uid 0 gets the site directories
    Platform platform = host_platform();
};

} // namespace platformdirs

#endif // PLATFORMDIRS_OPTIONS_H
