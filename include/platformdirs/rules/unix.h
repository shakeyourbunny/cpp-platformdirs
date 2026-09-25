// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#ifndef PLATFORMDIRS_RULES_UNIX_H
#define PLATFORMDIRS_RULES_UNIX_H

#include <platformdirs/context.h>

#include <string>
#include <vector>

// The XDG Base Directory rules for Linux, FreeBSD and OpenBSD (the platform picks the
// runtime constants). Results are generic text; use_site_for_root is applied by the API.
namespace platformdirs::rules::unix {

Result<std::string> user_data_dir(const Context& c);
Result<std::string> user_config_dir(const Context& c);
Result<std::string> user_cache_dir(const Context& c);
Result<std::string> user_state_dir(const Context& c);
Result<std::string> user_log_dir(const Context& c);
Result<std::string> user_runtime_dir(const Context& c);
Result<std::string> user_preference_dir(const Context& c);
Result<std::string> user_bin_dir(const Context& c);
Result<std::string> user_applications_dir(const Context& c);
Result<std::string> user_fonts_dir(const Context& c);

Result<std::string> user_documents_dir(const Context& c);
Result<std::string> user_downloads_dir(const Context& c);
Result<std::string> user_pictures_dir(const Context& c);
Result<std::string> user_videos_dir(const Context& c);
Result<std::string> user_music_dir(const Context& c);
Result<std::string> user_desktop_dir(const Context& c);
Result<std::string> user_projects_dir(const Context& c);
Result<std::string> user_publicshare_dir(const Context& c);
Result<std::string> user_templates_dir(const Context& c);

Result<std::vector<std::string>> site_data_dirs(const Context& c);
Result<std::vector<std::string>> site_config_dirs(const Context& c);
Result<std::string> site_cache_dir(const Context& c);
Result<std::string> site_state_dir(const Context& c);
Result<std::string> site_log_dir(const Context& c);
Result<std::string> site_runtime_dir(const Context& c);
Result<std::string> site_bin_dir(const Context& c);
Result<std::vector<std::string>> site_applications_dirs(const Context& c);
Result<std::vector<std::string>> site_fonts_dirs(const Context& c);

// Legacy ~/.fonts, which fontconfig still reads; the public
// iter_fonts_dirs appends it last.
Result<std::string> legacy_user_fonts_dir(const Context& c);

} // namespace platformdirs::rules::unix

#endif // PLATFORMDIRS_RULES_UNIX_H
