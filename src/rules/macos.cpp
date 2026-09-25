// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/compose.h>
#include <platformdirs/rules/macos.h>

#include "rules/common.h"

#include <string>

namespace platformdirs::rules::macos {

namespace {

using compose::append;
using detail::search_path;
using detail::target;
using detail::under_base;
using detail::under_fixed;
using detail::under_home;
using detail::under_home_app;

constexpr std::string_view application_support = "Library/Application Support";
constexpr std::string_view site_application_support = "/Library/Application Support";

// The environment variable when usable, else the default under home.
// No user-dirs.dirs on macOS.
Result<std::string> media_dir(const Context& c, std::string_view variable,
                              std::string_view default_name)
{
    if (auto from_env = base_from_env(c, variable)) {
        return *from_env;
    }
    return under_home(c, default_name);
}

// XDG_DATA_HOME/subdirectory when the variable is set, else the Apple
// location under home.
Result<std::string> data_home_or(const Context& c, std::string_view subdirectory,
                                 std::string_view apple_default)
{
    if (const auto from_env = base_from_env(c, "XDG_DATA_HOME")) {
        return append(target(c), *from_env, subdirectory);
    }
    return under_home(c, apple_default);
}

} // namespace

Result<std::string> user_data_dir(const Context& c)
{
    return under_base(c, "XDG_DATA_HOME", application_support);
}

Result<std::string> user_config_dir(const Context& c)
{
    return under_base(c, "XDG_CONFIG_HOME", application_support);
}

Result<std::string> user_cache_dir(const Context& c)
{
    return under_base(c, "XDG_CACHE_HOME", "Library/Caches");
}

Result<std::string> user_state_dir(const Context& c)
{
    return under_base(c, "XDG_STATE_HOME", application_support);
}

Result<std::string> user_log_dir(const Context& c)
{
    // Python does not consult XDG_STATE_HOME here and opinion has no
    // effect: ~/Library/Logs is a log directory already.
    return under_home_app(c, "Library/Logs");
}

Result<std::string> user_runtime_dir(const Context& c)
{
    return under_base(c, "XDG_RUNTIME_DIR", "Library/Caches/TemporaryItems");
}

Result<std::string> user_preference_dir(const Context& c)
{
    return under_home_app(c, "Library/Preferences");
}

Result<std::string> user_bin_dir(const Context& c)
{
    return under_home(c, ".local/bin");
}

Result<std::string> user_applications_dir(const Context& c)
{
    return data_home_or(c, "applications", "Applications");
}

Result<std::string> user_fonts_dir(const Context& c)
{
    return data_home_or(c, "fonts", "Library/Fonts");
}

Result<std::string> user_documents_dir(const Context& c)
{
    return media_dir(c, "XDG_DOCUMENTS_DIR", "Documents");
}

Result<std::string> user_downloads_dir(const Context& c)
{
    return media_dir(c, "XDG_DOWNLOAD_DIR", "Downloads");
}

Result<std::string> user_pictures_dir(const Context& c)
{
    return media_dir(c, "XDG_PICTURES_DIR", "Pictures");
}

Result<std::string> user_videos_dir(const Context& c)
{
    // Apple calls this one Movies.
    return media_dir(c, "XDG_VIDEOS_DIR", "Movies");
}

Result<std::string> user_music_dir(const Context& c)
{
    return media_dir(c, "XDG_MUSIC_DIR", "Music");
}

Result<std::string> user_desktop_dir(const Context& c)
{
    return media_dir(c, "XDG_DESKTOP_DIR", "Desktop");
}

Result<std::string> user_projects_dir(const Context& c)
{
    return media_dir(c, "XDG_PROJECTS_DIR", "Projects");
}

Result<std::string> user_publicshare_dir(const Context& c)
{
    return media_dir(c, "XDG_PUBLICSHARE_DIR", "Public");
}

Result<std::string> user_templates_dir(const Context& c)
{
    return media_dir(c, "XDG_TEMPLATES_DIR", "Templates");
}

Result<std::vector<std::string>> site_data_dirs(const Context& c)
{
    return search_path(c, "XDG_DATA_DIRS", site_application_support, true);
}

Result<std::vector<std::string>> site_config_dirs(const Context& c)
{
    return search_path(c, "XDG_CONFIG_DIRS", site_application_support, true);
}

Result<std::string> site_cache_dir(const Context& c)
{
    return under_fixed(c, "/Library/Caches");
}

Result<std::string> site_state_dir(const Context& c)
{
    return under_fixed(c, site_application_support);
}

Result<std::string> site_log_dir(const Context& c)
{
    return under_fixed(c, "/Library/Logs");
}

Result<std::string> site_runtime_dir(const Context& c)
{
    return user_runtime_dir(c);
}

Result<std::string> site_bin_dir(const Context&)
{
    return std::string("/usr/local/bin");
}

Result<std::vector<std::string>> site_applications_dirs(const Context& c)
{
    // XDG_DATA_DIRS entries each get the subdirectory; the Apple default
    // is /Applications itself, not /Applications/applications.
    if (const auto raw = c.env.get("XDG_DATA_DIRS")) {
        if (!compose::split_path_list(target(c), *raw).empty()) {
            return search_path(c, "XDG_DATA_DIRS", {}, false, "applications");
        }
    }
    return std::vector<std::string>{"/Applications"};
}

Result<std::vector<std::string>> site_fonts_dirs(const Context&)
{
    return std::vector<std::string>{"/Library/Fonts", "/System/Library/Fonts",
                                    "/Network/Library/Fonts"};
}

} // namespace platformdirs::rules::macos
