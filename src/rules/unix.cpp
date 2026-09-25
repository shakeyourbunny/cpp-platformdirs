// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/compose.h>
#include <platformdirs/rules/unix.h>
#include <platformdirs/user_dirs.h>

#include "rules/common.h"

#include <string>

namespace platformdirs::rules::unix {

namespace {

using compose::append;
using detail::search_path;
using detail::target;
using detail::under_base;
using detail::under_fixed;
using detail::under_home;
using detail::xdg_base;

constexpr std::string_view data_dirs_default = "/usr/local/share:/usr/share";

// Media directories: the variable first (Python's order), then user-dirs.dirs under the
// config base, then the default under home. Never the application name.
Result<std::string> media_dir(const Context& c, std::string_view variable,
                              std::string_view default_name)
{
    if (auto from_env = base_from_env(c, variable)) {
        return *from_env;
    }
    const auto h = home(c);
    if (!h) {
        return std::unexpected(h.error());
    }
    if (const auto config_base = xdg_base(c, "XDG_CONFIG_HOME", ".config")) {
        if (const auto file = append(target(c), *config_base, "user-dirs.dirs")) {
            if (const auto content = c.sys.read_text_file(*file, max_config_file_bytes)) {
                if (auto found = user_dirs::lookup(*content, variable, *h)) {
                    return compose::normalize_base(target(c), *found);
                }
            }
        }
    }
    return append(target(c), *h, default_name);
}

Result<std::string> user_id_text(const Context& c)
{
    const auto uid = c.sys.user_id();
    if (!uid) {
        return std::unexpected(
            Error::environment("no numeric user id, cannot place the runtime directory"));
    }
    return std::to_string(*uid);
}

} // namespace

Result<std::string> user_data_dir(const Context& c)
{
    return under_base(c, "XDG_DATA_HOME", ".local/share");
}

Result<std::string> user_config_dir(const Context& c)
{
    return under_base(c, "XDG_CONFIG_HOME", ".config");
}

Result<std::string> user_cache_dir(const Context& c)
{
    return under_base(c, "XDG_CACHE_HOME", ".cache");
}

Result<std::string> user_state_dir(const Context& c)
{
    return under_base(c, "XDG_STATE_HOME", ".local/state");
}

Result<std::string> user_log_dir(const Context& c)
{
    const auto state = user_state_dir(c);
    if (!state || !c.options.opinion) {
        return state;
    }
    // Python adds this segment outside the app name branch, so it lands
    // after the version and even without an application name.
    return append(target(c), *state, "log");
}

Result<std::string> user_runtime_dir(const Context& c)
{
    if (auto from_env = base_from_env(c, "XDG_RUNTIME_DIR")) {
        return under_fixed(c, *from_env);
    }
    const auto uid = user_id_text(c);
    if (!uid) {
        return std::unexpected(uid.error());
    }
    std::string_view prefix = "/run/user";
    if (target(c) == Platform::FreeBsd) {
        prefix = "/var/run/user";
    } else if (target(c) == Platform::OpenBsd) {
        prefix = "/tmp/run/user";
    }
    std::string base = std::string(prefix) + "/" + *uid;
    if (!c.sys.is_writable_directory(base)) {
        // Python's fallback when the login session directory is absent.
        const std::string tmp = base_from_env(c, "TMPDIR").value_or("/tmp");
        const auto under_tmp = append(target(c), tmp, "runtime-" + *uid);
        if (!under_tmp) {
            return std::unexpected(under_tmp.error());
        }
        base = *under_tmp;
    }
    return under_fixed(c, base);
}

Result<std::string> user_preference_dir(const Context& c)
{
    return user_config_dir(c);
}

Result<std::string> user_bin_dir(const Context& c)
{
    return under_home(c, ".local/bin");
}

Result<std::string> user_applications_dir(const Context& c)
{
    const auto base = xdg_base(c, "XDG_DATA_HOME", ".local/share");
    if (!base) {
        return std::unexpected(base.error());
    }
    return append(target(c), *base, "applications");
}

Result<std::string> user_fonts_dir(const Context& c)
{
    const auto base = xdg_base(c, "XDG_DATA_HOME", ".local/share");
    if (!base) {
        return std::unexpected(base.error());
    }
    return append(target(c), *base, "fonts");
}

Result<std::string> legacy_user_fonts_dir(const Context& c)
{
    return under_home(c, ".fonts");
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
    return media_dir(c, "XDG_VIDEOS_DIR", "Videos");
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
    return search_path(c, "XDG_DATA_DIRS", data_dirs_default, true);
}

Result<std::vector<std::string>> site_config_dirs(const Context& c)
{
    return search_path(c, "XDG_CONFIG_DIRS", "/etc/xdg", true);
}

Result<std::string> site_cache_dir(const Context& c)
{
    return under_fixed(c, "/var/cache");
}

Result<std::string> site_state_dir(const Context& c)
{
    return under_fixed(c, "/var/lib");
}

Result<std::string> site_log_dir(const Context& c)
{
    // /var/log is a log directory already; opinion has no effect.
    return under_fixed(c, "/var/log");
}

Result<std::string> site_runtime_dir(const Context& c)
{
    if (auto from_env = base_from_env(c, "XDG_RUNTIME_DIR")) {
        return under_fixed(c, *from_env);
    }
    return under_fixed(c, target(c) == Platform::Linux ? "/run" : "/var/run");
}

Result<std::string> site_bin_dir(const Context&)
{
    return std::string("/usr/local/bin");
}

Result<std::vector<std::string>> site_applications_dirs(const Context& c)
{
    return search_path(c, "XDG_DATA_DIRS", data_dirs_default, false, "applications");
}

Result<std::vector<std::string>> site_fonts_dirs(const Context& c)
{
    return search_path(c, "XDG_DATA_DIRS", data_dirs_default, false, "fonts");
}

} // namespace platformdirs::rules::unix
