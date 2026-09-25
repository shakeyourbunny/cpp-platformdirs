// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

// Prints every directory the library resolves for one application; with --platform,
// another platform's rules run against this machine's environment, errors included.

#include <platformdirs/platformdirs.h>
#include <platformdirs/version.h>

#include <cstring>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

constexpr int exit_ok = 0;
constexpr int exit_error = 1;
constexpr int exit_usage = 2;

void usage(std::ostream& out)
{
    out << "usage: show_dirs [--platform NAME] [APP_NAME [VERSION]]\n"
           "       show_dirs --version\n"
           "       show_dirs --help\n"
           "\n"
           "Prints the directories the platformdirs library resolves for\n"
           "APP_NAME (default: myapp) and VERSION (default: none).\n"
           "\n"
           "  --platform NAME  linux, freebsd, openbsd, macos or windows;\n"
           "                   default is the platform this binary was built for\n"
           "  --version        library version and product number\n"
           "  --help           this text\n";
}

std::optional<platformdirs::Platform> parse_platform(std::string_view name)
{
    using platformdirs::Platform;
    if (name == "linux") {
        return Platform::Linux;
    }
    if (name == "freebsd") {
        return Platform::FreeBsd;
    }
    if (name == "openbsd") {
        return Platform::OpenBsd;
    }
    if (name == "macos") {
        return Platform::MacOS;
    }
    if (name == "windows") {
        return Platform::Windows;
    }
    return std::nullopt;
}

void print(std::string_view name, const platformdirs::Result<platformdirs::Path>& r)
{
    std::cout << name << ": ";
    if (r) {
        std::cout << r->string() << '\n';
    } else {
        std::cout << "error: " << r.error().message << '\n';
    }
}

void print(std::string_view name, const platformdirs::Result<platformdirs::PathList>& r)
{
    std::cout << name << ": ";
    if (!r) {
        std::cout << "error: " << r.error().message << '\n';
        return;
    }
    bool first = true;
    for (const auto& p : *r) {
        std::cout << (first ? "" : ", ") << p.string();
        first = false;
    }
    std::cout << '\n';
}

} // namespace

int main(int argc, char** argv)
{
    platformdirs::Options options;
    options.app_name = "myapp";
    int positional = 0;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            usage(std::cout);
            return exit_ok;
        }
        if (arg == "--version") {
            std::cout << "cpp-platformdirs " << platformdirs::version;
            if (!platformdirs::part_number.empty()) {
                std::cout << " (" << platformdirs::part_number << ")";
            }
            std::cout << '\n';
            return exit_ok;
        }
        if (arg == "--platform") {
            if (i + 1 >= argc) {
                std::cerr << "show_dirs: --platform needs a name\n";
                usage(std::cerr);
                return exit_usage;
            }
            const auto p = parse_platform(argv[++i]);
            if (!p) {
                std::cerr << "show_dirs: unknown platform '" << argv[i] << "'\n";
                usage(std::cerr);
                return exit_usage;
            }
            options.platform = *p;
            continue;
        }
        if (arg.starts_with("-")) {
            std::cerr << "show_dirs: unknown option '" << arg << "'\n";
            usage(std::cerr);
            return exit_usage;
        }
        if (positional == 0) {
            options.app_name = std::string(arg);
        } else if (positional == 1) {
            options.version = std::string(arg);
        } else {
            std::cerr << "show_dirs: too many arguments\n";
            usage(std::cerr);
            return exit_usage;
        }
        ++positional;
    }

    const auto dirs = platformdirs::Dirs::create(options);
    if (!dirs) {
        std::cerr << "show_dirs: " << dirs.error().message << '\n';
        return exit_error;
    }
    const platformdirs::Dirs& d = *dirs;

    std::cout << "platform: " << platformdirs::platform_name(options.platform) << '\n';
    print("user_data_dir", d.user_data_dir());
    print("user_config_dir", d.user_config_dir());
    print("user_cache_dir", d.user_cache_dir());
    print("user_state_dir", d.user_state_dir());
    print("user_log_dir", d.user_log_dir());
    print("user_runtime_dir", d.user_runtime_dir());
    print("user_preference_dir", d.user_preference_dir());
    print("user_bin_dir", d.user_bin_dir());
    print("user_applications_dir", d.user_applications_dir());
    print("user_fonts_dir", d.user_fonts_dir());
    print("user_documents_dir", d.user_documents_dir());
    print("user_downloads_dir", d.user_downloads_dir());
    print("user_pictures_dir", d.user_pictures_dir());
    print("user_videos_dir", d.user_videos_dir());
    print("user_music_dir", d.user_music_dir());
    print("user_desktop_dir", d.user_desktop_dir());
    print("user_projects_dir", d.user_projects_dir());
    print("user_publicshare_dir", d.user_publicshare_dir());
    print("user_templates_dir", d.user_templates_dir());
    print("site_data_dirs", d.site_data_dirs());
    print("site_config_dirs", d.site_config_dirs());
    print("site_cache_dir", d.site_cache_dir());
    print("site_state_dir", d.site_state_dir());
    print("site_log_dir", d.site_log_dir());
    print("site_runtime_dir", d.site_runtime_dir());
    print("site_bin_dir", d.site_bin_dir());
    print("site_applications_dirs", d.site_applications_dirs());
    print("site_fonts_dirs", d.site_fonts_dirs());
    print("iter_fonts_dirs", d.iter_fonts_dirs());
    return exit_ok;
}
