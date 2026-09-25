// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/platformdirs.h>
#include <platformdirs/testing.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

using namespace platformdirs;

namespace {

std::string text(const Result<Path>& r)
{
    return r.has_value() ? r->string() : "<error: " + r.error().message + ">";
}

std::vector<std::string> texts(const Result<PathList>& r)
{
    if (!r.has_value()) {
        return {"<error: " + r.error().message + ">"};
    }
    std::vector<std::string> out;
    for (const Path& p : *r) {
        out.push_back(p.string());
    }
    return out;
}

class Api : public ::testing::Test {
protected:
    void SetUp() override
    {
        env.clear();
        sys.clear();
        sys.set_home("/home/test");
        sys.set_user_id(1000);
    }

    Result<Dirs> make(Options o) { return Dirs::create(std::move(o), env, sys); }

    static Options options(std::string name, Platform p = Platform::Linux)
    {
        Options o;
        o.app_name = std::move(name);
        o.platform = p;
        return o;
    }

    FixedEnvironment env;
    FixedSystem sys;
};

} // namespace

TEST_F(Api, create_refuses_a_bad_app_name)
{
    const auto d = make(options(".."));
    ASSERT_FALSE(d.has_value());
    EXPECT_EQ(d.error().code, ErrorCode::InvalidName);
    EXPECT_NE(d.error().message.find("app name"), std::string::npos);
}

TEST_F(Api, create_refuses_a_bad_author)
{
    Options o = options("myapp");
    o.author = Author::named("../Acme");
    const auto d = make(o);
    ASSERT_FALSE(d.has_value());
    EXPECT_EQ(d.error().code, ErrorCode::InvalidName);
    EXPECT_NE(d.error().message.find("app author"), std::string::npos);
}

TEST_F(Api, create_refuses_a_bad_version)
{
    Options o = options("myapp");
    o.version = "1.0/../..";
    const auto d = make(o);
    ASSERT_FALSE(d.has_value());
    EXPECT_EQ(d.error().code, ErrorCode::InvalidName);
    EXPECT_NE(d.error().message.find("version"), std::string::npos);
}

TEST_F(Api, create_accepts_ordinary_options)
{
    Options o = options("myapp");
    o.version = "1.0";
    o.author = Author::named("Acme");
    ASSERT_TRUE(make(o).has_value());
}

TEST_F(Api, default_platform_is_host)
{
    const Options o;
    EXPECT_EQ(o.platform, host_platform());
}

TEST_F(Api, options_are_readable_back)
{
    Options o = options("myapp", Platform::FreeBsd);
    o.version = "2";
    o.opinion = false;
    const auto d = make(o);
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(d->options().app_name, "myapp");
    EXPECT_EQ(d->options().version, "2");
    EXPECT_EQ(d->options().platform, Platform::FreeBsd);
    EXPECT_FALSE(d->options().opinion);
}

TEST_F(Api, linux_paths_come_back_as_filesystem_paths)
{
    const auto d = make(options("myapp"));
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(text(d->user_data_dir()), "/home/test/.local/share/myapp");
    EXPECT_EQ(text(d->user_config_dir()), "/home/test/.config/myapp");
    EXPECT_EQ(text(d->user_cache_dir()), "/home/test/.cache/myapp");
    EXPECT_EQ(text(d->user_state_dir()), "/home/test/.local/state/myapp");
    EXPECT_EQ(text(d->user_log_dir()), "/home/test/.local/state/myapp/log");
    EXPECT_EQ(text(d->user_documents_dir()), "/home/test/Documents");
    EXPECT_EQ(text(d->user_fonts_dir()), "/home/test/.local/share/fonts");
    EXPECT_EQ(text(d->site_cache_dir()), "/var/cache/myapp");
}

TEST_F(Api, foreign_platform_returns_rendered_text)
{
    sys.set_folder(KnownFolder::LocalAppData, "C:\\Users\\t\\AppData\\Local");
    const auto d = make(options("myapp", Platform::Windows));
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(text(d->user_data_dir()), "C:\\Users\\t\\AppData\\Local\\myapp\\myapp");
}

TEST_F(Api, macos_platform_can_be_asked_for)
{
    const auto d = make(options("myapp", Platform::MacOS));
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(text(d->user_data_dir()), "/home/test/Library/Application Support/myapp");
}

TEST_F(Api, site_data_dir_is_the_first_entry)
{
    const auto d = make(options("myapp"));
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(text(d->site_data_dir()), "/usr/local/share/myapp");
    EXPECT_EQ(texts(d->site_data_dirs()),
              (std::vector<std::string>{"/usr/local/share/myapp", "/usr/share/myapp"}));
    EXPECT_EQ(text(d->site_config_dir()), "/etc/xdg/myapp");
    EXPECT_EQ(text(d->site_applications_dir()), "/usr/local/share/applications");
}

TEST_F(Api, iter_data_dirs_is_user_then_site)
{
    const auto d = make(options("myapp"));
    ASSERT_TRUE(d.has_value());
    const std::vector<std::string> expected = {"/home/test/.local/share/myapp",
                                               "/usr/local/share/myapp", "/usr/share/myapp"};
    EXPECT_EQ(texts(d->iter_data_dirs()), expected);
    EXPECT_EQ(texts(d->iter_config_dirs()),
              (std::vector<std::string>{"/home/test/.config/myapp", "/etc/xdg/myapp"}));
    EXPECT_EQ(texts(d->iter_cache_dirs()),
              (std::vector<std::string>{"/home/test/.cache/myapp", "/var/cache/myapp"}));
    EXPECT_EQ(texts(d->iter_state_dirs()),
              (std::vector<std::string>{"/home/test/.local/state/myapp", "/var/lib/myapp"}));
    EXPECT_EQ(texts(d->iter_log_dirs()),
              (std::vector<std::string>{"/home/test/.local/state/myapp/log", "/var/log/myapp"}));
}

TEST_F(Api, iter_runtime_dirs_is_user_then_site)
{
    env.set("XDG_RUNTIME_DIR", "/rt");
    const auto d = make(options("myapp"));
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(texts(d->iter_runtime_dirs()), (std::vector<std::string>{"/rt/myapp", "/rt/myapp"}));
}

TEST_F(Api, iter_fonts_dirs_ends_with_legacy_on_linux)
{
    const auto d = make(options("myapp"));
    ASSERT_TRUE(d.has_value());
    const std::vector<std::string> expected = {"/home/test/.local/share/fonts",
                                               "/usr/local/share/fonts", "/usr/share/fonts",
                                               "/home/test/.fonts"};
    EXPECT_EQ(texts(d->iter_fonts_dirs()), expected);
}

TEST_F(Api, iter_fonts_dirs_has_no_legacy_entry_on_macos)
{
    const auto d = make(options("myapp", Platform::MacOS));
    ASSERT_TRUE(d.has_value());
    const std::vector<std::string> expected = {"/home/test/Library/Fonts", "/Library/Fonts",
                                               "/System/Library/Fonts", "/Network/Library/Fonts"};
    EXPECT_EQ(texts(d->iter_fonts_dirs()), expected);
}

TEST_F(Api, use_site_for_root_redirects_when_uid_is_zero)
{
    sys.set_user_id(0);
    Options o = options("myapp");
    o.use_site_for_root = true;
    const auto d = make(o);
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(text(d->user_data_dir()), "/usr/local/share/myapp");
    EXPECT_EQ(text(d->user_config_dir()), "/etc/xdg/myapp");
    EXPECT_EQ(text(d->user_cache_dir()), "/var/cache/myapp");
    EXPECT_EQ(text(d->user_state_dir()), "/var/lib/myapp");
    EXPECT_EQ(text(d->user_log_dir()), "/var/log/myapp");
    EXPECT_EQ(text(d->user_runtime_dir()), "/run/myapp");
    EXPECT_EQ(text(d->user_bin_dir()), "/usr/local/bin");
    EXPECT_EQ(text(d->user_applications_dir()), "/usr/local/share/applications");
    // Not redirected: media, fonts, preference.
    EXPECT_EQ(text(d->user_documents_dir()), "/home/test/Documents");
    EXPECT_EQ(text(d->user_fonts_dir()), "/home/test/.local/share/fonts");
}

TEST_F(Api, use_site_for_root_is_inert_for_a_normal_user)
{
    Options o = options("myapp");
    o.use_site_for_root = true;
    const auto d = make(o);
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(text(d->user_data_dir()), "/home/test/.local/share/myapp");
}

TEST_F(Api, use_site_for_root_skips_the_user_entry_in_iterators)
{
    sys.set_user_id(0);
    Options o = options("myapp");
    o.use_site_for_root = true;
    const auto d = make(o);
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(texts(d->iter_data_dirs()),
              (std::vector<std::string>{"/usr/local/share/myapp", "/usr/share/myapp"}));
    EXPECT_EQ(texts(d->iter_config_dirs()), std::vector<std::string>{"/etc/xdg/myapp"});
}

TEST_F(Api, use_site_for_root_is_inert_on_macos_and_windows)
{
    sys.set_user_id(0);
    Options o = options("myapp", Platform::MacOS);
    o.use_site_for_root = true;
    const auto d = make(o);
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(text(d->user_data_dir()), "/home/test/Library/Application Support/myapp");
}

TEST_F(Api, accessor_reports_missing_home_as_environment_error)
{
    sys.set_home(std::nullopt);
    const auto d = make(options("myapp"));
    ASSERT_TRUE(d.has_value());
    const auto r = d->user_data_dir();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, ErrorCode::Environment);
}

TEST_F(Api, path_type_round_trips_utf8)
{
    const auto d = make(options("Gr\xC3\xBC\xC3\x9F"
                                "e"));
    ASSERT_TRUE(d.has_value());
    const auto r = d->user_data_dir();
    ASSERT_TRUE(r.has_value());
    // u8string() on both sides: string() converts to the ANSI code page
    // on Windows and would not round-trip the umlauts.
    const auto as_utf8 = [](const Path& p) {
        const std::u8string u = p.u8string();
        return std::string(u.begin(), u.end());
    };
    EXPECT_EQ(as_utf8(r->filename()), "Gr\xC3\xBC\xC3\x9F"
                                      "e");
    EXPECT_EQ(as_utf8(*r), "/home/test/.local/share/Gr\xC3\xBC\xC3\x9F"
                           "e");
}

TEST_F(Api, dirs_is_copyable_and_the_copy_answers_the_same)
{
    const auto d = make(options("myapp"));
    ASSERT_TRUE(d.has_value());
    const Dirs copy = *d;
    EXPECT_EQ(text(copy.user_data_dir()), text(d->user_data_dir()));
}

TEST_F(Api, accessors_see_a_variable_set_after_create)
{
    const auto d = make(options("myapp"));
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(text(d->user_data_dir()), "/home/test/.local/share/myapp");
    env.set("XDG_DATA_HOME", "/late");
    EXPECT_EQ(text(d->user_data_dir()), "/late/myapp");
}

// Free functions through the native sources. HOME is pointed at a
// path that does not exist and restored afterwards.

namespace {

class FreeFunctions : public ::testing::Test {
protected:
    void SetUp() override
    {
#if !defined(_WIN32)
        const char* h = std::getenv("HOME");
        saved_home_ = h == nullptr ? std::nullopt : std::optional<std::string>(h);
        setenv("HOME", "/home/platformdirs-test-does-not-exist", 1);
#endif
    }
    void TearDown() override
    {
#if !defined(_WIN32)
        if (saved_home_) {
            setenv("HOME", saved_home_->c_str(), 1);
        } else {
            unsetenv("HOME");
        }
        EXPECT_FALSE(std::filesystem::exists("/home/platformdirs-test-does-not-exist"));
#endif
    }
    std::optional<std::string> saved_home_;
};

} // namespace

TEST_F(FreeFunctions, object_and_free_functions_agree)
{
    Options o;
    o.app_name = "myapp";
    o.version = "1.0";
    const auto d = Dirs::create(o);
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(text(d->user_data_dir()), text(user_data_dir("myapp", "1.0")));
    EXPECT_EQ(text(d->user_config_dir()), text(user_config_dir("myapp", "1.0")));
    EXPECT_EQ(text(d->user_cache_dir()), text(user_cache_dir("myapp", "1.0")));
    EXPECT_EQ(text(d->user_state_dir()), text(user_state_dir("myapp", "1.0")));
    EXPECT_EQ(text(d->user_log_dir()), text(user_log_dir("myapp", "1.0")));
    EXPECT_EQ(text(d->user_runtime_dir()), text(user_runtime_dir("myapp", "1.0")));
    EXPECT_EQ(text(d->site_data_dir()), text(site_data_dir("myapp", "1.0")));
    EXPECT_EQ(text(d->site_config_dir()), text(site_config_dir("myapp", "1.0")));
    EXPECT_EQ(text(d->site_cache_dir()), text(site_cache_dir("myapp", "1.0")));
    EXPECT_EQ(text(d->user_documents_dir()), text(user_documents_dir()));
    EXPECT_EQ(text(d->user_downloads_dir()), text(user_downloads_dir()));
    EXPECT_EQ(text(d->user_pictures_dir()), text(user_pictures_dir()));
    EXPECT_EQ(text(d->user_videos_dir()), text(user_videos_dir()));
    EXPECT_EQ(text(d->user_music_dir()), text(user_music_dir()));
    EXPECT_EQ(text(d->user_desktop_dir()), text(user_desktop_dir()));
    EXPECT_EQ(text(d->user_fonts_dir()), text(user_fonts_dir()));
    EXPECT_EQ(texts(d->site_fonts_dirs()), texts(site_fonts_dirs()));
    EXPECT_EQ(texts(d->iter_fonts_dirs()), texts(iter_fonts_dirs()));
}

TEST_F(FreeFunctions, free_functions_validate_names_too)
{
    const auto r = user_data_dir("../etc");
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, ErrorCode::InvalidName);
}

TEST_F(FreeFunctions, answers_are_absolute_on_the_host)
{
    const auto r = user_config_dir("myapp");
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r->is_absolute()) << r->string();
}

TEST_F(Api, use_site_for_root_leaves_out_the_legacy_fonts_dir)
{
    sys.set_user_id(0);
    sys.set_home(std::nullopt);
    Options o = options("myapp");
    o.use_site_for_root = true;
    const auto d = make(o);
    ASSERT_TRUE(d.has_value());
    const auto fonts = d->iter_fonts_dirs();
    ASSERT_TRUE(fonts.has_value()) << fonts.error().message;
    EXPECT_EQ(texts(*fonts),
              (std::vector<std::string>{"/usr/local/share/fonts", "/usr/share/fonts"}));
}
