// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/compose.h>
#include <platformdirs/rules/windows.h>
#include <platformdirs/validation.h>

#include <initializer_list>
#include <string>

namespace platformdirs::rules::windows {

namespace {

using compose::append;
using compose::append_windows_parts;

constexpr Platform p = Platform::Windows;
constexpr std::string_view start_menu_programs = "Microsoft/Windows/Start Menu/Programs";

// The Known Folder answer when it is usable as a base, normalized.
std::optional<std::string> usable_folder(const Context& c, KnownFolder f)
{
    const auto answer = c.sys.known_folder(f);
    if (!answer || !validation::is_usable_base(p, *answer)) {
        return std::nullopt;
    }
    return compose::normalize_base(p, *answer);
}

std::string_view folder_name(KnownFolder f)
{
    switch (f) {
    case KnownFolder::LocalAppData: return "LocalAppData";
    case KnownFolder::RoamingAppData: return "RoamingAppData";
    case KnownFolder::ProgramData: return "ProgramData";
    case KnownFolder::Profile: return "Profile";
    case KnownFolder::Documents: return "Documents";
    case KnownFolder::Downloads: return "Downloads";
    case KnownFolder::Pictures: return "Pictures";
    case KnownFolder::Videos: return "Videos";
    case KnownFolder::Music: return "Music";
    case KnownFolder::Desktop: return "Desktop";
    case KnownFolder::Programs: return "Programs";
    case KnownFolder::CommonPrograms: return "CommonPrograms";
    case KnownFolder::Fonts: return "Fonts";
    case KnownFolder::Public: return "Public";
    }
    return "?";
}

Error unresolved(KnownFolder f, std::initializer_list<std::string_view> variables)
{
    std::string message = "cannot resolve the Windows folder ";
    message += folder_name(f);
    message += ": the Known Folder lookup failed and ";
    std::string names;
    for (const std::string_view v : variables) {
        if (!names.empty()) {
            names += ", ";
        }
        names += v;
    }
    message += names;
    message += variables.size() == 1 ? " is not set" : " are not set";
    return Error::environment(message);
}

// The folder, else the first usable variable, else an error. This is
// the pair of sources Python uses, in the same order.
Result<std::string> folder_or_env(const Context& c, KnownFolder f,
                                  std::initializer_list<std::string_view> variables)
{
    if (auto folder = usable_folder(c, f)) {
        return *folder;
    }
    for (const std::string_view v : variables) {
        if (auto from_env = base_from_env(c, v)) {
            return *from_env;
        }
    }
    return std::unexpected(unresolved(f, variables));
}

Result<std::string> local_app_data(const Context& c)
{
    return folder_or_env(c, KnownFolder::LocalAppData, {"LOCALAPPDATA"});
}

Result<std::string> roaming_app_data(const Context& c)
{
    return folder_or_env(c, KnownFolder::RoamingAppData, {"APPDATA"});
}

Result<std::string> program_data(const Context& c)
{
    return folder_or_env(c, KnownFolder::ProgramData, {"PROGRAMDATA", "ALLUSERSPROFILE"});
}

Result<std::string> app_data(const Context& c)
{
    return c.options.roaming ? roaming_app_data(c) : local_app_data(c);
}

// The user profile: the folder, USERPROFILE, then the system source's own answer
// (on a real Windows the same chain plus HOMEDRIVE and HOMEPATH).
Result<std::string> profile(const Context& c)
{
    if (auto folder = usable_folder(c, KnownFolder::Profile)) {
        return *folder;
    }
    if (auto from_env = base_from_env(c, "USERPROFILE")) {
        return *from_env;
    }
    return home(c);
}

Result<std::string> with_app_parts(Result<std::string> base, const Context& c,
                                   std::string_view opinion_segment = {})
{
    if (!base) {
        return std::unexpected(base.error());
    }
    return append_windows_parts(*base, c.options, opinion_segment);
}

Result<std::string> with_suffix(Result<std::string> base, std::string_view suffix)
{
    if (!base) {
        return std::unexpected(base.error());
    }
    return append(p, *base, suffix);
}

// Media folders: the Known Folder, else the profile plus the English
// name Windows itself uses.
Result<std::string> media_dir(const Context& c, KnownFolder f, std::string_view name)
{
    if (auto folder = usable_folder(c, f)) {
        return *folder;
    }
    return with_suffix(profile(c), name);
}

Result<std::string> logs_if_opinion(Result<std::string> base, const Context& c)
{
    if (!base || !c.options.opinion) {
        return base;
    }
    return append(p, *base, "Logs");
}

} // namespace

Result<std::string> user_data_dir(const Context& c)
{
    return with_app_parts(app_data(c), c);
}

Result<std::string> user_config_dir(const Context& c)
{
    return user_data_dir(c);
}

Result<std::string> user_state_dir(const Context& c)
{
    return user_data_dir(c);
}

Result<std::string> user_preference_dir(const Context& c)
{
    return user_data_dir(c);
}

Result<std::string> user_cache_dir(const Context& c)
{
    // The cache never roams, and the segment goes through the parts
    // helper, so it lands before the version and only with an app name.
    return with_app_parts(local_app_data(c), c, "Cache");
}

Result<std::string> user_log_dir(const Context& c)
{
    // Added outside the parts helper, so unlike Cache it lands after
    // the version and even without an application name.
    return logs_if_opinion(user_data_dir(c), c);
}

Result<std::string> user_runtime_dir(const Context& c)
{
    return with_app_parts(with_suffix(local_app_data(c), "Temp"), c);
}

Result<std::string> site_runtime_dir(const Context& c)
{
    return user_runtime_dir(c);
}

Result<std::string> user_bin_dir(const Context& c)
{
    return with_suffix(local_app_data(c), "Programs");
}

Result<std::string> site_bin_dir(const Context& c)
{
    return with_suffix(program_data(c), "bin");
}

Result<std::string> user_applications_dir(const Context& c)
{
    if (auto folder = usable_folder(c, KnownFolder::Programs)) {
        return *folder;
    }
    return with_suffix(roaming_app_data(c), start_menu_programs);
}

Result<std::vector<std::string>> site_applications_dirs(const Context& c)
{
    Result<std::string> dir = std::string();
    if (auto folder = usable_folder(c, KnownFolder::CommonPrograms)) {
        dir = *folder;
    } else {
        dir = with_suffix(program_data(c), start_menu_programs);
    }
    if (!dir) {
        return std::unexpected(dir.error());
    }
    return std::vector<std::string>{*dir};
}

Result<std::string> user_fonts_dir(const Context& c)
{
    // The per-user font folder since Windows 10 1809.
    return with_suffix(local_app_data(c), "Microsoft/Windows/Fonts");
}

Result<std::vector<std::string>> site_fonts_dirs(const Context& c)
{
    if (auto folder = usable_folder(c, KnownFolder::Fonts)) {
        return std::vector<std::string>{*folder};
    }
    if (auto windir = base_from_env(c, "WINDIR")) {
        const auto fonts = append(p, *windir, "Fonts");
        if (!fonts) {
            return std::unexpected(fonts.error());
        }
        return std::vector<std::string>{*fonts};
    }
    return std::unexpected(unresolved(KnownFolder::Fonts, {"WINDIR"}));
}

Result<std::string> user_templates_dir(const Context& c)
{
    return with_suffix(roaming_app_data(c), "Microsoft/Windows/Templates");
}

Result<std::string> user_projects_dir(const Context& c)
{
    return with_suffix(profile(c), "Projects");
}

Result<std::string> user_publicshare_dir(const Context& c)
{
    if (auto folder = usable_folder(c, KnownFolder::Public)) {
        return *folder;
    }
    if (auto from_env = base_from_env(c, "PUBLIC")) {
        return *from_env;
    }
    // Python's last resort: the Public profile beside the user's own.
    // The parent never climbs above the drive root or the UNC share.
    const auto own = profile(c);
    if (!own) {
        return std::unexpected(own.error());
    }
    std::size_t root_len = 3; // "C:/"
    if (own->starts_with("//")) {
        const std::size_t after_server = own->find('/', 2);
        const std::size_t after_share = after_server == std::string::npos
                                            ? std::string::npos
                                            : own->find('/', after_server + 1);
        root_len = after_share == std::string::npos ? own->size() : after_share;
    }
    const std::size_t cut = own->find_last_of('/');
    const std::string parent =
        cut == std::string::npos || cut < root_len ? own->substr(0, root_len) : own->substr(0, cut);
    return append(p, parent, "Public");
}

Result<std::string> user_documents_dir(const Context& c)
{
    return media_dir(c, KnownFolder::Documents, "Documents");
}

Result<std::string> user_downloads_dir(const Context& c)
{
    return media_dir(c, KnownFolder::Downloads, "Downloads");
}

Result<std::string> user_pictures_dir(const Context& c)
{
    return media_dir(c, KnownFolder::Pictures, "Pictures");
}

Result<std::string> user_videos_dir(const Context& c)
{
    return media_dir(c, KnownFolder::Videos, "Videos");
}

Result<std::string> user_music_dir(const Context& c)
{
    return media_dir(c, KnownFolder::Music, "Music");
}

Result<std::string> user_desktop_dir(const Context& c)
{
    return media_dir(c, KnownFolder::Desktop, "Desktop");
}

Result<std::vector<std::string>> site_data_dirs(const Context& c)
{
    const auto dir = with_app_parts(program_data(c), c);
    if (!dir) {
        return std::unexpected(dir.error());
    }
    return std::vector<std::string>{*dir};
}

Result<std::vector<std::string>> site_config_dirs(const Context& c)
{
    return site_data_dirs(c);
}

Result<std::string> site_state_dir(const Context& c)
{
    return with_app_parts(program_data(c), c);
}

Result<std::string> site_cache_dir(const Context& c)
{
    return with_app_parts(program_data(c), c, "Cache");
}

Result<std::string> site_log_dir(const Context& c)
{
    return logs_if_opinion(site_state_dir(c), c);
}

} // namespace platformdirs::rules::windows
