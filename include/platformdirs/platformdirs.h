// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#ifndef PLATFORMDIRS_PLATFORMDIRS_H
#define PLATFORMDIRS_PLATFORMDIRS_H

#include <platformdirs/error.h>
#include <platformdirs/options.h>
#include <platformdirs/platform.h>
#include <platformdirs/sources.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace platformdirs {

namespace rules {
struct Context;
}

using Path = std::filesystem::path;
using PathList = std::vector<std::filesystem::path>;

// The directories of one application; resolves only, never creates. Accessors read the
// environment when called, the object is immutable and thread-safe, a foreign platform yields text.
class Dirs {
public:
    // Through the native sources; InvalidName refuses a name that could escape its base.
    static Result<Dirs> create(Options options);

    // Explicit sources, for tests and other environments; both must outlive the Dirs.
    static Result<Dirs> create(Options options, const EnvironmentSource& env,
                               const SystemSource& sys);

    const Options& options() const { return options_; }

    Result<Path> user_data_dir() const;
    Result<Path> user_config_dir() const;
    Result<Path> user_cache_dir() const;
    Result<Path> user_state_dir() const;
    Result<Path> user_log_dir() const;
    Result<Path> user_runtime_dir() const;
    Result<Path> user_preference_dir() const;
    Result<Path> user_bin_dir() const;
    Result<Path> user_applications_dir() const;
    Result<Path> user_fonts_dir() const;

    Result<Path> user_documents_dir() const;
    Result<Path> user_downloads_dir() const;
    Result<Path> user_pictures_dir() const;
    Result<Path> user_videos_dir() const;
    Result<Path> user_music_dir() const;
    Result<Path> user_desktop_dir() const;
    Result<Path> user_projects_dir() const;
    Result<Path> user_publicshare_dir() const;
    Result<Path> user_templates_dir() const;

    // The search paths. The singular form is the first entry, which is
    // what a caller writing a file wants.
    Result<Path> site_data_dir() const;
    Result<PathList> site_data_dirs() const;
    Result<Path> site_config_dir() const;
    Result<PathList> site_config_dirs() const;
    Result<Path> site_cache_dir() const;
    Result<Path> site_state_dir() const;
    Result<Path> site_log_dir() const;
    Result<Path> site_runtime_dir() const;
    Result<Path> site_bin_dir() const;
    Result<Path> site_applications_dir() const;
    Result<PathList> site_applications_dirs() const;
    Result<PathList> site_fonts_dirs() const;

    // User first, then site, for callers that search every location.
    // With use_site_for_root and uid 0 the user entry is left out.
    Result<PathList> iter_data_dirs() const;
    Result<PathList> iter_config_dirs() const;
    Result<PathList> iter_cache_dirs() const;
    Result<PathList> iter_state_dirs() const;
    Result<PathList> iter_log_dirs() const;
    Result<PathList> iter_runtime_dirs() const;
    // User, then site, then on Linux and the BSDs the legacy ~/.fonts.
    Result<PathList> iter_fonts_dirs() const;

private:
    using RuleFn = Result<std::string> (*)(const rules::Context&);
    using ListFn = Result<std::vector<std::string>> (*)(const rules::Context&);

    Dirs(Options options, const EnvironmentSource& env, const SystemSource& sys);

    bool use_site() const;
    Result<Path> single(RuleFn unix_fn, RuleFn macos_fn, RuleFn windows_fn) const;
    Result<PathList> many(ListFn unix_fn, ListFn macos_fn, ListFn windows_fn) const;
    Result<Path> first_of(const Result<PathList>& list) const;
    Result<PathList> user_then_site(const Result<Path>& user, const Result<PathList>& site) const;
    Path to_path(const std::string& generic) const;

    Options options_;
    const EnvironmentSource* env_;
    const SystemSource* sys_;
};

// Object-free forms: each builds a Dirs for the host platform through the native sources.
Result<Path> user_data_dir(std::string_view app_name, std::string_view version = {});
Result<Path> user_config_dir(std::string_view app_name, std::string_view version = {});
Result<Path> user_cache_dir(std::string_view app_name, std::string_view version = {});
Result<Path> user_state_dir(std::string_view app_name, std::string_view version = {});
Result<Path> user_log_dir(std::string_view app_name, std::string_view version = {});
Result<Path> user_runtime_dir(std::string_view app_name, std::string_view version = {});
Result<Path> site_data_dir(std::string_view app_name, std::string_view version = {});
Result<Path> site_config_dir(std::string_view app_name, std::string_view version = {});
Result<Path> site_cache_dir(std::string_view app_name, std::string_view version = {});
Result<Path> user_documents_dir();
Result<Path> user_downloads_dir();
Result<Path> user_pictures_dir();
Result<Path> user_videos_dir();
Result<Path> user_music_dir();
Result<Path> user_desktop_dir();
Result<Path> user_fonts_dir();
Result<PathList> site_fonts_dirs();
Result<PathList> iter_fonts_dirs();

} // namespace platformdirs

#endif // PLATFORMDIRS_PLATFORMDIRS_H
