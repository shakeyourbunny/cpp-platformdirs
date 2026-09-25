// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/rules/macos.h>
#include <platformdirs/testing.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace platformdirs;
namespace apple = platformdirs::rules::macos;

namespace {

constexpr const char* home = "/Users/test";
constexpr const char* support = "/Users/test/Library/Application Support";

std::string value_of(const Result<std::string>& r)
{
    return r.has_value() ? *r : "<error: " + r.error().message + ">";
}

std::vector<std::string> values_of(const Result<std::vector<std::string>>& r)
{
    return r.has_value() ? *r : std::vector<std::string>{"<error: " + r.error().message + ">"};
}

class MacOSRules : public ::testing::Test {
protected:
    void SetUp() override
    {
        env.clear();
        sys.clear();
        sys.set_home(home);
        sys.set_user_id(501);
    }

    rules::Context ctx(std::string name = "myapp", std::string version = "", bool opinion = true)
    {
        opts = Options{};
        opts.app_name = std::move(name);
        opts.version = std::move(version);
        opts.platform = Platform::MacOS;
        opts.opinion = opinion;
        return rules::Context{env, sys, opts};
    }

    FixedEnvironment env;
    FixedSystem sys;
    Options opts;
};

} // namespace

TEST_F(MacOSRules, data_dir)
{
    EXPECT_EQ(value_of(apple::user_data_dir(ctx())), std::string(support) + "/myapp");
}

TEST_F(MacOSRules, config_and_state_match_data)
{
    EXPECT_EQ(value_of(apple::user_config_dir(ctx())), value_of(apple::user_data_dir(ctx())));
    EXPECT_EQ(value_of(apple::user_state_dir(ctx())), value_of(apple::user_data_dir(ctx())));
}

TEST_F(MacOSRules, cache_dir)
{
    EXPECT_EQ(value_of(apple::user_cache_dir(ctx())), "/Users/test/Library/Caches/myapp");
}

TEST_F(MacOSRules, log_dir_ignores_opinion)
{
    EXPECT_EQ(value_of(apple::user_log_dir(ctx())), "/Users/test/Library/Logs/myapp");
    EXPECT_EQ(value_of(apple::user_log_dir(ctx("myapp", "1.0"))),
              "/Users/test/Library/Logs/myapp/1.0");
    EXPECT_EQ(value_of(apple::user_log_dir(ctx("myapp", "", false))),
              "/Users/test/Library/Logs/myapp");
}

TEST_F(MacOSRules, log_ignores_xdg_state_home)
{
    env.set("XDG_STATE_HOME", "/state");
    EXPECT_EQ(value_of(apple::user_log_dir(ctx())), "/Users/test/Library/Logs/myapp");
}

TEST_F(MacOSRules, runtime_dir_and_site_runtime_match)
{
    EXPECT_EQ(value_of(apple::user_runtime_dir(ctx())),
              "/Users/test/Library/Caches/TemporaryItems/myapp");
    EXPECT_EQ(value_of(apple::site_runtime_dir(ctx())), value_of(apple::user_runtime_dir(ctx())));
}

TEST_F(MacOSRules, preference_dir)
{
    EXPECT_EQ(value_of(apple::user_preference_dir(ctx())), "/Users/test/Library/Preferences/myapp");
}

TEST_F(MacOSRules, site_dirs)
{
    EXPECT_EQ(values_of(apple::site_data_dirs(ctx())),
              std::vector<std::string>{"/Library/Application Support/myapp"});
    EXPECT_EQ(values_of(apple::site_config_dirs(ctx())), values_of(apple::site_data_dirs(ctx())));
    EXPECT_EQ(value_of(apple::site_cache_dir(ctx())), "/Library/Caches/myapp");
    EXPECT_EQ(value_of(apple::site_state_dir(ctx())), "/Library/Application Support/myapp");
    EXPECT_EQ(value_of(apple::site_log_dir(ctx())), "/Library/Logs/myapp");
}

TEST_F(MacOSRules, version_appended)
{
    EXPECT_EQ(value_of(apple::user_data_dir(ctx("myapp", "1.0"))),
              std::string(support) + "/myapp/1.0");
}

TEST_F(MacOSRules, without_app_name)
{
    EXPECT_EQ(value_of(apple::user_data_dir(ctx(""))), support);
}

TEST_F(MacOSRules, media_dirs)
{
    EXPECT_EQ(value_of(apple::user_documents_dir(ctx())), "/Users/test/Documents");
    EXPECT_EQ(value_of(apple::user_downloads_dir(ctx())), "/Users/test/Downloads");
    EXPECT_EQ(value_of(apple::user_pictures_dir(ctx())), "/Users/test/Pictures");
    EXPECT_EQ(value_of(apple::user_music_dir(ctx())), "/Users/test/Music");
    EXPECT_EQ(value_of(apple::user_desktop_dir(ctx())), "/Users/test/Desktop");
    EXPECT_EQ(value_of(apple::user_projects_dir(ctx())), "/Users/test/Projects");
    EXPECT_EQ(value_of(apple::user_publicshare_dir(ctx())), "/Users/test/Public");
    EXPECT_EQ(value_of(apple::user_templates_dir(ctx())), "/Users/test/Templates");
}

TEST_F(MacOSRules, videos_is_movies)
{
    EXPECT_EQ(value_of(apple::user_videos_dir(ctx())), "/Users/test/Movies");
}

TEST_F(MacOSRules, media_dirs_ignore_app_name)
{
    EXPECT_EQ(value_of(apple::user_music_dir(ctx("myapp", "1.0"))), "/Users/test/Music");
}

TEST_F(MacOSRules, xdg_variables_take_precedence)
{
    env.set("XDG_DATA_HOME", "/d");
    env.set("XDG_CONFIG_HOME", "/cfg");
    env.set("XDG_CACHE_HOME", "/cache");
    env.set("XDG_STATE_HOME", "/state");
    env.set("XDG_RUNTIME_DIR", "/rt");
    env.set("XDG_DATA_DIRS", "/x:/y");
    env.set("XDG_CONFIG_DIRS", "/c");
    env.set("XDG_DOCUMENTS_DIR", "/docs");
    env.set("XDG_VIDEOS_DIR", "/vids");
    EXPECT_EQ(value_of(apple::user_data_dir(ctx())), "/d/myapp");
    EXPECT_EQ(value_of(apple::user_config_dir(ctx())), "/cfg/myapp");
    EXPECT_EQ(value_of(apple::user_cache_dir(ctx())), "/cache/myapp");
    EXPECT_EQ(value_of(apple::user_state_dir(ctx())), "/state/myapp");
    EXPECT_EQ(value_of(apple::user_runtime_dir(ctx())), "/rt/myapp");
    EXPECT_EQ(value_of(apple::site_runtime_dir(ctx())), "/rt/myapp");
    EXPECT_EQ(values_of(apple::site_data_dirs(ctx())),
              (std::vector<std::string>{"/x/myapp", "/y/myapp"}));
    EXPECT_EQ(values_of(apple::site_config_dirs(ctx())), std::vector<std::string>{"/c/myapp"});
    EXPECT_EQ(value_of(apple::user_documents_dir(ctx())), "/docs");
    EXPECT_EQ(value_of(apple::user_videos_dir(ctx())), "/vids");
}

TEST_F(MacOSRules, relative_xdg_variable_is_ignored)
{
    env.set("XDG_DATA_HOME", "relative");
    EXPECT_EQ(value_of(apple::user_data_dir(ctx())), std::string(support) + "/myapp");
}

TEST_F(MacOSRules, no_user_dirs_file_lookup)
{
    sys.add_file("/Users/test/.config/user-dirs.dirs", "XDG_DOCUMENTS_DIR=\"$HOME/Docs\"\n");
    EXPECT_EQ(value_of(apple::user_documents_dir(ctx())), "/Users/test/Documents");
}

TEST_F(MacOSRules, applications_dirs)
{
    EXPECT_EQ(value_of(apple::user_applications_dir(ctx())), "/Users/test/Applications");
    EXPECT_EQ(values_of(apple::site_applications_dirs(ctx())),
              std::vector<std::string>{"/Applications"});
    env.set("XDG_DATA_HOME", "/d");
    env.set("XDG_DATA_DIRS", "/x");
    EXPECT_EQ(value_of(apple::user_applications_dir(ctx())), "/d/applications");
    EXPECT_EQ(values_of(apple::site_applications_dirs(ctx())),
              std::vector<std::string>{"/x/applications"});
}

TEST_F(MacOSRules, fonts_dirs)
{
    EXPECT_EQ(value_of(apple::user_fonts_dir(ctx())), "/Users/test/Library/Fonts");
    const std::vector<std::string> site = {"/Library/Fonts", "/System/Library/Fonts",
                                           "/Network/Library/Fonts"};
    EXPECT_EQ(values_of(apple::site_fonts_dirs(ctx())), site);
    env.set("XDG_DATA_HOME", "/d");
    EXPECT_EQ(value_of(apple::user_fonts_dir(ctx())), "/d/fonts");
    EXPECT_EQ(values_of(apple::site_fonts_dirs(ctx())), site);
}

TEST_F(MacOSRules, bin_dirs)
{
    EXPECT_EQ(value_of(apple::user_bin_dir(ctx())), "/Users/test/.local/bin");
    EXPECT_EQ(value_of(apple::site_bin_dir(ctx())), "/usr/local/bin");
}

TEST_F(MacOSRules, home_missing_is_environment_error)
{
    sys.set_home(std::nullopt);
    const auto r = apple::user_data_dir(ctx());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, ErrorCode::Environment);
}

TEST_F(MacOSRules, site_dirs_do_not_need_a_home)
{
    sys.set_home(std::nullopt);
    EXPECT_EQ(value_of(apple::site_cache_dir(ctx())), "/Library/Caches/myapp");
}
