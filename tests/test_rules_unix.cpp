// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/rules/unix.h>
#include <platformdirs/testing.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace platformdirs;
namespace unix_rules = platformdirs::rules::unix;

namespace {

constexpr const char* home = "/home/test";

std::string value_of(const Result<std::string>& r)
{
    return r.has_value() ? *r : "<error: " + r.error().message + ">";
}

std::vector<std::string> values_of(const Result<std::vector<std::string>>& r)
{
    return r.has_value() ? *r : std::vector<std::string>{"<error: " + r.error().message + ">"};
}

class UnixRules : public ::testing::Test {
protected:
    void SetUp() override
    {
        env.clear();
        sys.clear();
        sys.set_home(home);
        sys.set_user_id(1000);
    }

    rules::Context ctx(std::string name = "myapp", std::string version = "",
                       Platform platform = Platform::Linux, bool opinion = true)
    {
        opts = Options{};
        opts.app_name = std::move(name);
        opts.version = std::move(version);
        opts.platform = platform;
        opts.opinion = opinion;
        return rules::Context{env, sys, opts};
    }

    FixedEnvironment env;
    FixedSystem sys;
    Options opts;
};

} // namespace

// Data, config, cache, state

TEST_F(UnixRules, data_default)
{
    EXPECT_EQ(value_of(unix_rules::user_data_dir(ctx())), "/home/test/.local/share/myapp");
}

TEST_F(UnixRules, data_from_env)
{
    env.set("XDG_DATA_HOME", "/custom");
    EXPECT_EQ(value_of(unix_rules::user_data_dir(ctx())), "/custom/myapp");
}

TEST_F(UnixRules, data_empty_env_falls_back)
{
    env.set("XDG_DATA_HOME", "");
    EXPECT_EQ(value_of(unix_rules::user_data_dir(ctx())), "/home/test/.local/share/myapp");
}

TEST_F(UnixRules, data_blank_env_falls_back)
{
    env.set("XDG_DATA_HOME", "   ");
    EXPECT_EQ(value_of(unix_rules::user_data_dir(ctx())), "/home/test/.local/share/myapp");
}

TEST_F(UnixRules, data_relative_env_falls_back)
{
    env.set("XDG_DATA_HOME", "relative/data");
    EXPECT_EQ(value_of(unix_rules::user_data_dir(ctx())), "/home/test/.local/share/myapp");
}

TEST_F(UnixRules, data_trailing_slash_env)
{
    env.set("XDG_DATA_HOME", "/custom/");
    EXPECT_EQ(value_of(unix_rules::user_data_dir(ctx())), "/custom/myapp");
}

TEST_F(UnixRules, data_with_version)
{
    EXPECT_EQ(value_of(unix_rules::user_data_dir(ctx("myapp", "1.0"))),
              "/home/test/.local/share/myapp/1.0");
}

TEST_F(UnixRules, data_without_app_name)
{
    EXPECT_EQ(value_of(unix_rules::user_data_dir(ctx(""))), "/home/test/.local/share");
}

TEST_F(UnixRules, config_default_and_env)
{
    EXPECT_EQ(value_of(unix_rules::user_config_dir(ctx())), "/home/test/.config/myapp");
    env.set("XDG_CONFIG_HOME", "/cfg");
    EXPECT_EQ(value_of(unix_rules::user_config_dir(ctx())), "/cfg/myapp");
}

TEST_F(UnixRules, cache_default_and_env)
{
    EXPECT_EQ(value_of(unix_rules::user_cache_dir(ctx())), "/home/test/.cache/myapp");
    env.set("XDG_CACHE_HOME", "/cache");
    EXPECT_EQ(value_of(unix_rules::user_cache_dir(ctx())), "/cache/myapp");
}

TEST_F(UnixRules, state_default_and_env)
{
    EXPECT_EQ(value_of(unix_rules::user_state_dir(ctx())), "/home/test/.local/state/myapp");
    env.set("XDG_STATE_HOME", "/state");
    EXPECT_EQ(value_of(unix_rules::user_state_dir(ctx())), "/state/myapp");
}

TEST_F(UnixRules, preference_is_config)
{
    EXPECT_EQ(value_of(unix_rules::user_preference_dir(ctx())), "/home/test/.config/myapp");
}

// Log

TEST_F(UnixRules, log_appends_log_segment)
{
    EXPECT_EQ(value_of(unix_rules::user_log_dir(ctx())), "/home/test/.local/state/myapp/log");
}

TEST_F(UnixRules, log_segment_follows_version)
{
    EXPECT_EQ(value_of(unix_rules::user_log_dir(ctx("myapp", "1.0"))),
              "/home/test/.local/state/myapp/1.0/log");
}

TEST_F(UnixRules, log_without_app_name)
{
    // Python adds the segment outside the app name branch.
    EXPECT_EQ(value_of(unix_rules::user_log_dir(ctx(""))), "/home/test/.local/state/log");
}

TEST_F(UnixRules, log_without_opinion)
{
    EXPECT_EQ(value_of(unix_rules::user_log_dir(ctx("myapp", "", Platform::Linux, false))),
              "/home/test/.local/state/myapp");
}

// Runtime

TEST_F(UnixRules, runtime_default_uses_uid_when_writable)
{
    sys.add_writable_directory("/run/user/1000");
    EXPECT_EQ(value_of(unix_rules::user_runtime_dir(ctx())), "/run/user/1000/myapp");
}

TEST_F(UnixRules, runtime_falls_back_to_tmp_when_not_writable)
{
    EXPECT_EQ(value_of(unix_rules::user_runtime_dir(ctx())), "/tmp/runtime-1000/myapp");
}

TEST_F(UnixRules, runtime_fallback_honours_tmpdir)
{
    env.set("TMPDIR", "/var/tmp");
    EXPECT_EQ(value_of(unix_rules::user_runtime_dir(ctx())), "/var/tmp/runtime-1000/myapp");
}

TEST_F(UnixRules, runtime_fallback_ignores_a_relative_tmpdir)
{
    env.set("TMPDIR", "tmp");
    EXPECT_EQ(value_of(unix_rules::user_runtime_dir(ctx())), "/tmp/runtime-1000/myapp");
}

TEST_F(UnixRules, runtime_from_env_skips_the_writability_check)
{
    env.set("XDG_RUNTIME_DIR", "/rt");
    EXPECT_EQ(value_of(unix_rules::user_runtime_dir(ctx())), "/rt/myapp");
}

TEST_F(UnixRules, runtime_freebsd_default)
{
    sys.add_writable_directory("/var/run/user/1000");
    EXPECT_EQ(value_of(unix_rules::user_runtime_dir(ctx("myapp", "", Platform::FreeBsd))),
              "/var/run/user/1000/myapp");
}

TEST_F(UnixRules, runtime_openbsd_default)
{
    sys.add_writable_directory("/tmp/run/user/1000");
    EXPECT_EQ(value_of(unix_rules::user_runtime_dir(ctx("myapp", "", Platform::OpenBsd))),
              "/tmp/run/user/1000/myapp");
}

TEST_F(UnixRules, runtime_without_a_user_id_is_an_error)
{
    sys.set_user_id(std::nullopt);
    const auto r = unix_rules::user_runtime_dir(ctx());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, ErrorCode::Environment);
}

TEST_F(UnixRules, site_runtime_default_and_bsd)
{
    EXPECT_EQ(value_of(unix_rules::site_runtime_dir(ctx())), "/run/myapp");
    EXPECT_EQ(value_of(unix_rules::site_runtime_dir(ctx("myapp", "", Platform::FreeBsd))),
              "/var/run/myapp");
    EXPECT_EQ(value_of(unix_rules::site_runtime_dir(ctx("myapp", "", Platform::OpenBsd))),
              "/var/run/myapp");
}

TEST_F(UnixRules, site_runtime_from_env)
{
    env.set("XDG_RUNTIME_DIR", "/rt");
    EXPECT_EQ(value_of(unix_rules::site_runtime_dir(ctx())), "/rt/myapp");
}

// Site search paths

TEST_F(UnixRules, site_data_defaults)
{
    const std::vector<std::string> expected = {"/usr/local/share/myapp", "/usr/share/myapp"};
    EXPECT_EQ(values_of(unix_rules::site_data_dirs(ctx())), expected);
}

TEST_F(UnixRules, site_data_from_env)
{
    env.set("XDG_DATA_DIRS", "/a:/b");
    const std::vector<std::string> expected = {"/a/myapp", "/b/myapp"};
    EXPECT_EQ(values_of(unix_rules::site_data_dirs(ctx())), expected);
}

TEST_F(UnixRules, site_data_relative_entry_dropped)
{
    env.set("XDG_DATA_DIRS", "/a:../evil:/b");
    const std::vector<std::string> expected = {"/a/myapp", "/b/myapp"};
    EXPECT_EQ(values_of(unix_rules::site_data_dirs(ctx())), expected);
}

TEST_F(UnixRules, site_data_only_separators_falls_back)
{
    env.set("XDG_DATA_DIRS", ":::");
    const std::vector<std::string> expected = {"/usr/local/share/myapp", "/usr/share/myapp"};
    EXPECT_EQ(values_of(unix_rules::site_data_dirs(ctx())), expected);
}

TEST_F(UnixRules, site_data_with_version)
{
    const std::vector<std::string> expected = {"/usr/local/share/myapp/2", "/usr/share/myapp/2"};
    EXPECT_EQ(values_of(unix_rules::site_data_dirs(ctx("myapp", "2"))), expected);
}

TEST_F(UnixRules, site_config_default_and_env)
{
    EXPECT_EQ(values_of(unix_rules::site_config_dirs(ctx())),
              std::vector<std::string>{"/etc/xdg/myapp"});
    env.set("XDG_CONFIG_DIRS", "/c1:/c2");
    const std::vector<std::string> expected = {"/c1/myapp", "/c2/myapp"};
    EXPECT_EQ(values_of(unix_rules::site_config_dirs(ctx())), expected);
}

TEST_F(UnixRules, site_cache_state_log_defaults)
{
    EXPECT_EQ(value_of(unix_rules::site_cache_dir(ctx())), "/var/cache/myapp");
    EXPECT_EQ(value_of(unix_rules::site_state_dir(ctx())), "/var/lib/myapp");
    EXPECT_EQ(value_of(unix_rules::site_log_dir(ctx())), "/var/log/myapp");
}

TEST_F(UnixRules, site_log_ignores_opinion)
{
    EXPECT_EQ(value_of(unix_rules::site_log_dir(ctx("myapp", "", Platform::Linux, false))),
              "/var/log/myapp");
}

// Bin, applications, fonts

TEST_F(UnixRules, bin_dirs)
{
    EXPECT_EQ(value_of(unix_rules::user_bin_dir(ctx())), "/home/test/.local/bin");
    EXPECT_EQ(value_of(unix_rules::site_bin_dir(ctx())), "/usr/local/bin");
}

TEST_F(UnixRules, applications_dirs_default_and_env)
{
    EXPECT_EQ(value_of(unix_rules::user_applications_dir(ctx())),
              "/home/test/.local/share/applications");
    const std::vector<std::string> defaults = {"/usr/local/share/applications",
                                               "/usr/share/applications"};
    EXPECT_EQ(values_of(unix_rules::site_applications_dirs(ctx())), defaults);

    env.set("XDG_DATA_HOME", "/d");
    env.set("XDG_DATA_DIRS", "/x");
    EXPECT_EQ(value_of(unix_rules::user_applications_dir(ctx())), "/d/applications");
    EXPECT_EQ(values_of(unix_rules::site_applications_dirs(ctx())),
              std::vector<std::string>{"/x/applications"});
}

TEST_F(UnixRules, fonts_dirs_default_and_env)
{
    EXPECT_EQ(value_of(unix_rules::user_fonts_dir(ctx())), "/home/test/.local/share/fonts");
    const std::vector<std::string> defaults = {"/usr/local/share/fonts", "/usr/share/fonts"};
    EXPECT_EQ(values_of(unix_rules::site_fonts_dirs(ctx())), defaults);

    env.set("XDG_DATA_HOME", "/d");
    env.set("XDG_DATA_DIRS", "/x:/y");
    EXPECT_EQ(value_of(unix_rules::user_fonts_dir(ctx())), "/d/fonts");
    const std::vector<std::string> from_env = {"/x/fonts", "/y/fonts"};
    EXPECT_EQ(values_of(unix_rules::site_fonts_dirs(ctx())), from_env);
}

TEST_F(UnixRules, legacy_fonts_dir)
{
    EXPECT_EQ(value_of(unix_rules::legacy_user_fonts_dir(ctx())), "/home/test/.fonts");
}

// Media directories

TEST_F(UnixRules, documents_default)
{
    EXPECT_EQ(value_of(unix_rules::user_documents_dir(ctx())), "/home/test/Documents");
}

TEST_F(UnixRules, documents_from_env_wins_over_file)
{
    env.set("XDG_DOCUMENTS_DIR", "/docs");
    sys.add_file("/home/test/.config/user-dirs.dirs", "XDG_DOCUMENTS_DIR=\"$HOME/FromFile\"\n");
    EXPECT_EQ(value_of(unix_rules::user_documents_dir(ctx())), "/docs");
}

TEST_F(UnixRules, documents_from_user_dirs_file)
{
    sys.add_file("/home/test/.config/user-dirs.dirs", "XDG_DOCUMENTS_DIR=\"$HOME/FromFile\"\n");
    EXPECT_EQ(value_of(unix_rules::user_documents_dir(ctx())), "/home/test/FromFile");
}

TEST_F(UnixRules, documents_relative_env_then_file)
{
    env.set("XDG_DOCUMENTS_DIR", "Docs");
    sys.add_file("/home/test/.config/user-dirs.dirs", "XDG_DOCUMENTS_DIR=\"$HOME/FromFile\"\n");
    EXPECT_EQ(value_of(unix_rules::user_documents_dir(ctx())), "/home/test/FromFile");
}

TEST_F(UnixRules, user_dirs_file_under_xdg_config_home)
{
    env.set("XDG_CONFIG_HOME", "/cfg");
    sys.add_file("/cfg/user-dirs.dirs", "XDG_DOCUMENTS_DIR=\"$HOME/Cfg\"\n");
    sys.add_file("/home/test/.config/user-dirs.dirs", "XDG_DOCUMENTS_DIR=\"$HOME/Default\"\n");
    EXPECT_EQ(value_of(unix_rules::user_documents_dir(ctx())), "/home/test/Cfg");
}

TEST_F(UnixRules, user_dirs_file_missing_key_falls_back)
{
    sys.add_file("/home/test/.config/user-dirs.dirs", "XDG_MUSIC_DIR=\"$HOME/Tunes\"\n");
    EXPECT_EQ(value_of(unix_rules::user_documents_dir(ctx())), "/home/test/Documents");
    EXPECT_EQ(value_of(unix_rules::user_music_dir(ctx())), "/home/test/Tunes");
}

TEST_F(UnixRules, user_dirs_file_relative_value_ignored)
{
    sys.add_file("/home/test/.config/user-dirs.dirs", "XDG_DOCUMENTS_DIR=\"Docs\"\n");
    EXPECT_EQ(value_of(unix_rules::user_documents_dir(ctx())), "/home/test/Documents");
}

TEST_F(UnixRules, user_dirs_file_cannot_point_outside)
{
    sys.add_file("/home/test/.config/user-dirs.dirs", "XDG_DOCUMENTS_DIR=\"../../../etc\"\n");
    EXPECT_EQ(value_of(unix_rules::user_documents_dir(ctx())), "/home/test/Documents");
}

TEST_F(UnixRules, huge_user_dirs_file_is_bounded)
{
    // Around 4 MB of comments with the key past the line cap: the read cap and
    // the line cap together must end this quickly with the default.
    std::string junk;
    const std::string line = "# " + std::string(200, 'x') + "\n";
    for (int i = 0; i < 20000; ++i) {
        junk += line;
    }
    sys.add_file("/home/test/.config/user-dirs.dirs", junk + "XDG_DOCUMENTS_DIR=\"/late\"\n");
    EXPECT_EQ(value_of(unix_rules::user_documents_dir(ctx())), "/home/test/Documents");
}

TEST_F(UnixRules, media_dirs_ignore_app_name_and_version)
{
    EXPECT_EQ(value_of(unix_rules::user_music_dir(ctx("myapp", "1.0"))), "/home/test/Music");
}

TEST_F(UnixRules, remaining_media_defaults)
{
    EXPECT_EQ(value_of(unix_rules::user_downloads_dir(ctx())), "/home/test/Downloads");
    EXPECT_EQ(value_of(unix_rules::user_pictures_dir(ctx())), "/home/test/Pictures");
    EXPECT_EQ(value_of(unix_rules::user_videos_dir(ctx())), "/home/test/Videos");
    EXPECT_EQ(value_of(unix_rules::user_desktop_dir(ctx())), "/home/test/Desktop");
    EXPECT_EQ(value_of(unix_rules::user_projects_dir(ctx())), "/home/test/Projects");
    EXPECT_EQ(value_of(unix_rules::user_publicshare_dir(ctx())), "/home/test/Public");
    EXPECT_EQ(value_of(unix_rules::user_templates_dir(ctx())), "/home/test/Templates");
}

TEST_F(UnixRules, remaining_media_from_env)
{
    env.set("XDG_DOWNLOAD_DIR", "/dl");
    env.set("XDG_PICTURES_DIR", "/pics");
    env.set("XDG_VIDEOS_DIR", "/vids");
    env.set("XDG_DESKTOP_DIR", "/desk");
    env.set("XDG_PROJECTS_DIR", "/proj");
    env.set("XDG_PUBLICSHARE_DIR", "/pub");
    env.set("XDG_TEMPLATES_DIR", "/tpl");
    EXPECT_EQ(value_of(unix_rules::user_downloads_dir(ctx())), "/dl");
    EXPECT_EQ(value_of(unix_rules::user_pictures_dir(ctx())), "/pics");
    EXPECT_EQ(value_of(unix_rules::user_videos_dir(ctx())), "/vids");
    EXPECT_EQ(value_of(unix_rules::user_desktop_dir(ctx())), "/desk");
    EXPECT_EQ(value_of(unix_rules::user_projects_dir(ctx())), "/proj");
    EXPECT_EQ(value_of(unix_rules::user_publicshare_dir(ctx())), "/pub");
    EXPECT_EQ(value_of(unix_rules::user_templates_dir(ctx())), "/tpl");
}

// Home and hostile environment

TEST_F(UnixRules, home_must_be_absolute)
{
    sys.set_home("relative");
    const auto r = unix_rules::user_data_dir(ctx());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, ErrorCode::Environment);
}

TEST_F(UnixRules, home_missing_is_environment_error)
{
    sys.set_home(std::nullopt);
    const auto r = unix_rules::user_config_dir(ctx());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, ErrorCode::Environment);
}

TEST_F(UnixRules, home_with_trailing_slash_is_normalized)
{
    sys.set_home("/home/test/");
    EXPECT_EQ(value_of(unix_rules::user_data_dir(ctx())), "/home/test/.local/share/myapp");
}

TEST_F(UnixRules, site_dirs_do_not_need_a_home)
{
    sys.set_home(std::nullopt);
    EXPECT_EQ(value_of(unix_rules::site_cache_dir(ctx())), "/var/cache/myapp");
    EXPECT_EQ(values_of(unix_rules::site_data_dirs(ctx())),
              (std::vector<std::string>{"/usr/local/share/myapp", "/usr/share/myapp"}));
}

TEST_F(UnixRules, overlong_env_value_is_ignored)
{
    env.set("XDG_CONFIG_HOME", "/" + std::string(5000, 'a'));
    EXPECT_EQ(value_of(unix_rules::user_config_dir(ctx())), "/home/test/.config/myapp");
}

TEST_F(UnixRules, env_value_with_nul_is_ignored)
{
    env.set("XDG_DATA_HOME", std::string("/tmp/x\0/y", 9));
    EXPECT_EQ(value_of(unix_rules::user_data_dir(ctx())), "/home/test/.local/share/myapp");
}

TEST_F(UnixRules, env_value_with_newline_is_ignored)
{
    env.set("XDG_DATA_HOME", "/tmp/x\ny");
    EXPECT_EQ(value_of(unix_rules::user_data_dir(ctx())), "/home/test/.local/share/myapp");
}

TEST_F(UnixRules, one_megabyte_env_value_is_ignored)
{
    env.set("XDG_DATA_HOME", "/" + std::string(1024 * 1024, 'a'));
    EXPECT_EQ(value_of(unix_rules::user_data_dir(ctx())), "/home/test/.local/share/myapp");
}

TEST_F(UnixRules, overlong_result_is_refused)
{
    // Just inside the limit on its own, over it once the app name is appended.
    env.set("XDG_DATA_HOME", "/" + std::string(max_path_bytes_posix - 4, 'a'));
    const auto r = unix_rules::user_data_dir(ctx());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, ErrorCode::PathTooLong);
}

TEST_F(UnixRules, site_data_overlong_entry_is_dropped_not_fatal)
{
    env.set("XDG_DATA_DIRS", "/usr/share:/" + std::string(max_path_bytes_posix - 4, 'a'));
    EXPECT_EQ(values_of(unix_rules::site_data_dirs(ctx())),
              std::vector<std::string>{"/usr/share/myapp"});
}

TEST_F(UnixRules, runtime_fallback_with_root_tmpdir_has_no_double_slash)
{
    env.set("TMPDIR", "/");
    EXPECT_EQ(value_of(unix_rules::user_runtime_dir(ctx())), "/runtime-1000/myapp");
}

TEST_F(UnixRules, env_values_are_trimmed_like_python)
{
    env.set("XDG_DATA_HOME", "  /custom\t");
    EXPECT_EQ(value_of(unix_rules::user_data_dir(ctx())), "/custom/myapp");
    env.set("XDG_DATA_DIRS", " /a : /b ");
    EXPECT_EQ(values_of(unix_rules::site_data_dirs(ctx())),
              (std::vector<std::string>{"/a/myapp", "/b/myapp"}));
}

TEST_F(UnixRules, search_path_falls_back_when_every_entry_overflows)
{
    env.set("XDG_DATA_DIRS", "/" + std::string(max_path_bytes_posix - 4, 'a'));
    EXPECT_EQ(values_of(unix_rules::site_data_dirs(ctx())),
              (std::vector<std::string>{"/usr/local/share/myapp", "/usr/share/myapp"}));
}

TEST_F(UnixRules, env_values_are_trimmed_of_all_python_whitespace)
{
    env.set("XDG_CONFIG_HOME", "/x\r\n");
    EXPECT_EQ(value_of(unix_rules::user_config_dir(ctx())), "/x/myapp");
}

TEST_F(UnixRules, env_values_are_trimmed_of_unicode_spaces)
{
    env.set("XDG_CONFIG_HOME", "/x\xC2\xA0");
    EXPECT_EQ(value_of(unix_rules::user_config_dir(ctx())), "/x/myapp");
}
