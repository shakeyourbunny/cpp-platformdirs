// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/compose.h>
#include <platformdirs/context.h>
#include <platformdirs/platformdirs.h>
#include <platformdirs/rules/macos.h>
#include <platformdirs/rules/unix.h>
#include <platformdirs/rules/windows.h>
#include <platformdirs/validation.h>

#include <utility>

namespace platformdirs {

namespace unix_rules = rules::unix;
namespace macos_rules = rules::macos;
namespace windows_rules = rules::windows;

Dirs::Dirs(Options options, const EnvironmentSource& env, const SystemSource& sys)
    : options_(std::move(options)), env_(&env), sys_(&sys)
{}

Result<Dirs> Dirs::create(Options options)
{
    return create(std::move(options), native_environment(), native_system());
}

Result<Dirs> Dirs::create(Options options, const EnvironmentSource& env, const SystemSource& sys)
{
    if (const Status s = validation::check_component(options.app_name, "app name"); !s) {
        return std::unexpected(s.error());
    }
    if (options.author.mode() == Author::Mode::Explicit) {
        if (const Status s = validation::check_component(options.author.name(), "app author"); !s) {
            return std::unexpected(s.error());
        }
    }
    if (const Status s = validation::check_component(options.version, "version"); !s) {
        return std::unexpected(s.error());
    }
    return Dirs(std::move(options), env, sys);
}

bool Dirs::use_site() const
{
    if (!options_.use_site_for_root || !is_unix_like(options_.platform)) {
        return false;
    }
    const auto uid = sys_->user_id();
    return uid.has_value() && *uid == 0;
}

Path Dirs::to_path(const std::string& generic) const
{
    const std::string rendered = compose::render(options_.platform, generic);
    return Path(std::u8string(rendered.begin(), rendered.end()));
}

Result<Path> Dirs::single(RuleFn unix_fn, RuleFn macos_fn, RuleFn windows_fn) const
{
    const rules::Context c{*env_, *sys_, options_};
    const Platform p = options_.platform;
    const auto r = is_unix_like(p)        ? unix_fn(c)
                   : p == Platform::MacOS ? macos_fn(c)
                                          : windows_fn(c);
    if (!r) {
        return std::unexpected(r.error());
    }
    return to_path(*r);
}

Result<PathList> Dirs::many(ListFn unix_fn, ListFn macos_fn, ListFn windows_fn) const
{
    const rules::Context c{*env_, *sys_, options_};
    const Platform p = options_.platform;
    const auto r = is_unix_like(p)        ? unix_fn(c)
                   : p == Platform::MacOS ? macos_fn(c)
                                          : windows_fn(c);
    if (!r) {
        return std::unexpected(r.error());
    }
    PathList out;
    out.reserve(r->size());
    for (const std::string& s : *r) {
        out.push_back(to_path(s));
    }
    return out;
}

Result<Path> Dirs::first_of(const Result<PathList>& list) const
{
    if (!list) {
        return std::unexpected(list.error());
    }
    if (list->empty()) {
        return std::unexpected(Error::environment("the search path resolved to no entries"));
    }
    return list->front();
}

Result<PathList> Dirs::user_then_site(const Result<Path>& user, const Result<PathList>& site) const
{
    if (!site) {
        return std::unexpected(site.error());
    }
    PathList out;
    if (!use_site()) {
        if (!user) {
            return std::unexpected(user.error());
        }
        out.push_back(*user);
    }
    out.insert(out.end(), site->begin(), site->end());
    return out;
}

// User directories. The Python Unix class redirects these eight to the
// site equivalents when use_site_for_root is set and the uid is 0.

Result<Path> Dirs::user_data_dir() const
{
    if (use_site()) {
        return site_data_dir();
    }
    return single(unix_rules::user_data_dir, macos_rules::user_data_dir,
                  windows_rules::user_data_dir);
}

Result<Path> Dirs::user_config_dir() const
{
    if (use_site()) {
        return site_config_dir();
    }
    return single(unix_rules::user_config_dir, macos_rules::user_config_dir,
                  windows_rules::user_config_dir);
}

Result<Path> Dirs::user_cache_dir() const
{
    if (use_site()) {
        return site_cache_dir();
    }
    return single(unix_rules::user_cache_dir, macos_rules::user_cache_dir,
                  windows_rules::user_cache_dir);
}

Result<Path> Dirs::user_state_dir() const
{
    if (use_site()) {
        return site_state_dir();
    }
    return single(unix_rules::user_state_dir, macos_rules::user_state_dir,
                  windows_rules::user_state_dir);
}

Result<Path> Dirs::user_log_dir() const
{
    if (use_site()) {
        return site_log_dir();
    }
    return single(unix_rules::user_log_dir, macos_rules::user_log_dir, windows_rules::user_log_dir);
}

Result<Path> Dirs::user_runtime_dir() const
{
    if (use_site()) {
        return site_runtime_dir();
    }
    return single(unix_rules::user_runtime_dir, macos_rules::user_runtime_dir,
                  windows_rules::user_runtime_dir);
}

Result<Path> Dirs::user_bin_dir() const
{
    if (use_site()) {
        return site_bin_dir();
    }
    return single(unix_rules::user_bin_dir, macos_rules::user_bin_dir, windows_rules::user_bin_dir);
}

Result<Path> Dirs::user_applications_dir() const
{
    if (use_site()) {
        return site_applications_dir();
    }
    return single(unix_rules::user_applications_dir, macos_rules::user_applications_dir,
                  windows_rules::user_applications_dir);
}

Result<Path> Dirs::user_preference_dir() const
{
    return single(unix_rules::user_preference_dir, macos_rules::user_preference_dir,
                  windows_rules::user_preference_dir);
}

Result<Path> Dirs::user_fonts_dir() const
{
    return single(unix_rules::user_fonts_dir, macos_rules::user_fonts_dir,
                  windows_rules::user_fonts_dir);
}

Result<Path> Dirs::user_documents_dir() const
{
    return single(unix_rules::user_documents_dir, macos_rules::user_documents_dir,
                  windows_rules::user_documents_dir);
}

Result<Path> Dirs::user_downloads_dir() const
{
    return single(unix_rules::user_downloads_dir, macos_rules::user_downloads_dir,
                  windows_rules::user_downloads_dir);
}

Result<Path> Dirs::user_pictures_dir() const
{
    return single(unix_rules::user_pictures_dir, macos_rules::user_pictures_dir,
                  windows_rules::user_pictures_dir);
}

Result<Path> Dirs::user_videos_dir() const
{
    return single(unix_rules::user_videos_dir, macos_rules::user_videos_dir,
                  windows_rules::user_videos_dir);
}

Result<Path> Dirs::user_music_dir() const
{
    return single(unix_rules::user_music_dir, macos_rules::user_music_dir,
                  windows_rules::user_music_dir);
}

Result<Path> Dirs::user_desktop_dir() const
{
    return single(unix_rules::user_desktop_dir, macos_rules::user_desktop_dir,
                  windows_rules::user_desktop_dir);
}

Result<Path> Dirs::user_projects_dir() const
{
    return single(unix_rules::user_projects_dir, macos_rules::user_projects_dir,
                  windows_rules::user_projects_dir);
}

Result<Path> Dirs::user_publicshare_dir() const
{
    return single(unix_rules::user_publicshare_dir, macos_rules::user_publicshare_dir,
                  windows_rules::user_publicshare_dir);
}

Result<Path> Dirs::user_templates_dir() const
{
    return single(unix_rules::user_templates_dir, macos_rules::user_templates_dir,
                  windows_rules::user_templates_dir);
}

// Site directories

Result<PathList> Dirs::site_data_dirs() const
{
    return many(unix_rules::site_data_dirs, macos_rules::site_data_dirs,
                windows_rules::site_data_dirs);
}

Result<Path> Dirs::site_data_dir() const
{
    return first_of(site_data_dirs());
}

Result<PathList> Dirs::site_config_dirs() const
{
    return many(unix_rules::site_config_dirs, macos_rules::site_config_dirs,
                windows_rules::site_config_dirs);
}

Result<Path> Dirs::site_config_dir() const
{
    return first_of(site_config_dirs());
}

Result<Path> Dirs::site_cache_dir() const
{
    return single(unix_rules::site_cache_dir, macos_rules::site_cache_dir,
                  windows_rules::site_cache_dir);
}

Result<Path> Dirs::site_state_dir() const
{
    return single(unix_rules::site_state_dir, macos_rules::site_state_dir,
                  windows_rules::site_state_dir);
}

Result<Path> Dirs::site_log_dir() const
{
    return single(unix_rules::site_log_dir, macos_rules::site_log_dir, windows_rules::site_log_dir);
}

Result<Path> Dirs::site_runtime_dir() const
{
    return single(unix_rules::site_runtime_dir, macos_rules::site_runtime_dir,
                  windows_rules::site_runtime_dir);
}

Result<Path> Dirs::site_bin_dir() const
{
    return single(unix_rules::site_bin_dir, macos_rules::site_bin_dir, windows_rules::site_bin_dir);
}

Result<PathList> Dirs::site_applications_dirs() const
{
    return many(unix_rules::site_applications_dirs, macos_rules::site_applications_dirs,
                windows_rules::site_applications_dirs);
}

Result<Path> Dirs::site_applications_dir() const
{
    return first_of(site_applications_dirs());
}

Result<PathList> Dirs::site_fonts_dirs() const
{
    return many(unix_rules::site_fonts_dirs, macos_rules::site_fonts_dirs,
                windows_rules::site_fonts_dirs);
}

// Iterators

Result<PathList> Dirs::iter_data_dirs() const
{
    return user_then_site(user_data_dir(), site_data_dirs());
}

Result<PathList> Dirs::iter_config_dirs() const
{
    return user_then_site(user_config_dir(), site_config_dirs());
}

namespace {

Result<PathList> one(const Result<Path>& p)
{
    if (!p) {
        return std::unexpected(p.error());
    }
    return PathList{*p};
}

} // namespace

Result<PathList> Dirs::iter_cache_dirs() const
{
    return user_then_site(user_cache_dir(), one(site_cache_dir()));
}

Result<PathList> Dirs::iter_state_dirs() const
{
    return user_then_site(user_state_dir(), one(site_state_dir()));
}

Result<PathList> Dirs::iter_log_dirs() const
{
    return user_then_site(user_log_dir(), one(site_log_dir()));
}

Result<PathList> Dirs::iter_runtime_dirs() const
{
    return user_then_site(user_runtime_dir(), one(site_runtime_dir()));
}

Result<PathList> Dirs::iter_fonts_dirs() const
{
    auto out = user_then_site(user_fonts_dir(), site_fonts_dirs());
    if (!out) {
        return out;
    }
    // ~/.fonts is a per-user location; when the user directories are
    // redirected to the site ones it has no place in the list.
    if (is_unix_like(options_.platform) && !use_site()) {
        const rules::Context c{*env_, *sys_, options_};
        const auto legacy = unix_rules::legacy_user_fonts_dir(c);
        if (!legacy) {
            return std::unexpected(legacy.error());
        }
        out->push_back(to_path(*legacy));
    }
    return out;
}

// Free functions

namespace {

Result<Dirs> for_app(std::string_view app_name, std::string_view version)
{
    Options o;
    o.app_name = std::string(app_name);
    o.version = std::string(version);
    return Dirs::create(std::move(o));
}

template <typename Accessor>
auto through(std::string_view app_name, std::string_view version, Accessor accessor)
    -> decltype(accessor(std::declval<const Dirs&>()))
{
    const auto d = for_app(app_name, version);
    if (!d) {
        return std::unexpected(d.error());
    }
    return accessor(*d);
}

} // namespace

Result<Path> user_data_dir(std::string_view app_name, std::string_view version)
{
    return through(app_name, version, [](const Dirs& d) { return d.user_data_dir(); });
}

Result<Path> user_config_dir(std::string_view app_name, std::string_view version)
{
    return through(app_name, version, [](const Dirs& d) { return d.user_config_dir(); });
}

Result<Path> user_cache_dir(std::string_view app_name, std::string_view version)
{
    return through(app_name, version, [](const Dirs& d) { return d.user_cache_dir(); });
}

Result<Path> user_state_dir(std::string_view app_name, std::string_view version)
{
    return through(app_name, version, [](const Dirs& d) { return d.user_state_dir(); });
}

Result<Path> user_log_dir(std::string_view app_name, std::string_view version)
{
    return through(app_name, version, [](const Dirs& d) { return d.user_log_dir(); });
}

Result<Path> user_runtime_dir(std::string_view app_name, std::string_view version)
{
    return through(app_name, version, [](const Dirs& d) { return d.user_runtime_dir(); });
}

Result<Path> site_data_dir(std::string_view app_name, std::string_view version)
{
    return through(app_name, version, [](const Dirs& d) { return d.site_data_dir(); });
}

Result<Path> site_config_dir(std::string_view app_name, std::string_view version)
{
    return through(app_name, version, [](const Dirs& d) { return d.site_config_dir(); });
}

Result<Path> site_cache_dir(std::string_view app_name, std::string_view version)
{
    return through(app_name, version, [](const Dirs& d) { return d.site_cache_dir(); });
}

Result<Path> user_documents_dir()
{
    return through({}, {}, [](const Dirs& d) { return d.user_documents_dir(); });
}

Result<Path> user_downloads_dir()
{
    return through({}, {}, [](const Dirs& d) { return d.user_downloads_dir(); });
}

Result<Path> user_pictures_dir()
{
    return through({}, {}, [](const Dirs& d) { return d.user_pictures_dir(); });
}

Result<Path> user_videos_dir()
{
    return through({}, {}, [](const Dirs& d) { return d.user_videos_dir(); });
}

Result<Path> user_music_dir()
{
    return through({}, {}, [](const Dirs& d) { return d.user_music_dir(); });
}

Result<Path> user_desktop_dir()
{
    return through({}, {}, [](const Dirs& d) { return d.user_desktop_dir(); });
}

Result<Path> user_fonts_dir()
{
    return through({}, {}, [](const Dirs& d) { return d.user_fonts_dir(); });
}

Result<PathList> site_fonts_dirs()
{
    return through({}, {}, [](const Dirs& d) { return d.site_fonts_dirs(); });
}

Result<PathList> iter_fonts_dirs()
{
    return through({}, {}, [](const Dirs& d) { return d.iter_fonts_dirs(); });
}

} // namespace platformdirs
