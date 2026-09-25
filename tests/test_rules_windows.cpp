// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/rules/windows.h>
#include <platformdirs/testing.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace platformdirs;
namespace win = platformdirs::rules::windows;

namespace {

constexpr const char* local = "C:/Users/t/AppData/Local";
constexpr const char* roaming = "C:/Users/t/AppData/Roaming";

std::string value_of(const Result<std::string>& r)
{
    return r.has_value() ? *r : "<error: " + r.error().message + ">";
}

std::vector<std::string> values_of(const Result<std::vector<std::string>>& r)
{
    return r.has_value() ? *r : std::vector<std::string>{"<error: " + r.error().message + ">"};
}

class WindowsRules : public ::testing::Test {
protected:
    void SetUp() override
    {
        env.clear();
        sys.clear();
        sys.set_user_id(std::nullopt);
        sys.set_folder(KnownFolder::LocalAppData, "C:\\Users\\t\\AppData\\Local");
        sys.set_folder(KnownFolder::RoamingAppData, "C:\\Users\\t\\AppData\\Roaming");
        sys.set_folder(KnownFolder::ProgramData, "C:\\ProgramData");
        sys.set_folder(KnownFolder::Profile, "C:\\Users\\t");
    }

    rules::Context ctx(std::string name = "myapp", std::string version = "",
                       Author author = Author(), bool roaming_profile = false, bool opinion = true)
    {
        opts = Options{};
        opts.app_name = std::move(name);
        opts.version = std::move(version);
        opts.author = author;
        opts.roaming = roaming_profile;
        opts.opinion = opinion;
        opts.platform = Platform::Windows;
        return rules::Context{env, sys, opts};
    }

    FixedEnvironment env;
    FixedSystem sys;
    Options opts;
};

} // namespace

// Data, config, state, cache, log, runtime

TEST_F(WindowsRules, data_dir)
{
    EXPECT_EQ(value_of(win::user_data_dir(ctx())), std::string(local) + "/myapp/myapp");
}

TEST_F(WindowsRules, data_dir_roaming)
{
    EXPECT_EQ(value_of(win::user_data_dir(ctx("myapp", "", Author(), true))),
              std::string(roaming) + "/myapp/myapp");
}

TEST_F(WindowsRules, config_state_and_preference_match_data)
{
    EXPECT_EQ(value_of(win::user_config_dir(ctx())), value_of(win::user_data_dir(ctx())));
    EXPECT_EQ(value_of(win::user_state_dir(ctx())), value_of(win::user_data_dir(ctx())));
    EXPECT_EQ(value_of(win::user_preference_dir(ctx())), value_of(win::user_data_dir(ctx())));
}

TEST_F(WindowsRules, cache_appends_cache_segment)
{
    EXPECT_EQ(value_of(win::user_cache_dir(ctx())), std::string(local) + "/myapp/myapp/Cache");
}

TEST_F(WindowsRules, cache_segment_precedes_version)
{
    EXPECT_EQ(value_of(win::user_cache_dir(ctx("myapp", "1.0"))),
              std::string(local) + "/myapp/myapp/Cache/1.0");
}

TEST_F(WindowsRules, cache_ignores_roaming)
{
    EXPECT_EQ(value_of(win::user_cache_dir(ctx("myapp", "", Author(), true))),
              std::string(local) + "/myapp/myapp/Cache");
}

TEST_F(WindowsRules, log_appends_logs_segment)
{
    EXPECT_EQ(value_of(win::user_log_dir(ctx())), std::string(local) + "/myapp/myapp/Logs");
}

TEST_F(WindowsRules, log_segment_follows_version)
{
    EXPECT_EQ(value_of(win::user_log_dir(ctx("myapp", "1.0"))),
              std::string(local) + "/myapp/myapp/1.0/Logs");
}

TEST_F(WindowsRules, log_without_opinion)
{
    EXPECT_EQ(value_of(win::user_log_dir(ctx("myapp", "", Author(), false, false))),
              std::string(local) + "/myapp/myapp");
}

TEST_F(WindowsRules, runtime_uses_temp)
{
    EXPECT_EQ(value_of(win::user_runtime_dir(ctx())), std::string(local) + "/Temp/myapp/myapp");
    EXPECT_EQ(value_of(win::site_runtime_dir(ctx())), value_of(win::user_runtime_dir(ctx())));
}

// Site directories

TEST_F(WindowsRules, site_data_uses_program_data)
{
    EXPECT_EQ(values_of(win::site_data_dirs(ctx())),
              std::vector<std::string>{"C:/ProgramData/myapp/myapp"});
    EXPECT_EQ(values_of(win::site_config_dirs(ctx())), values_of(win::site_data_dirs(ctx())));
    EXPECT_EQ(value_of(win::site_state_dir(ctx())), "C:/ProgramData/myapp/myapp");
}

TEST_F(WindowsRules, site_cache_and_log_segments)
{
    EXPECT_EQ(value_of(win::site_cache_dir(ctx())), "C:/ProgramData/myapp/myapp/Cache");
    EXPECT_EQ(value_of(win::site_log_dir(ctx())), "C:/ProgramData/myapp/myapp/Logs");
    EXPECT_EQ(value_of(win::site_log_dir(ctx("myapp", "", Author(), false, false))),
              "C:/ProgramData/myapp/myapp");
}

TEST_F(WindowsRules, programdata_preferred_over_allusersprofile)
{
    sys.set_folder(KnownFolder::ProgramData, std::nullopt);
    env.set("ALLUSERSPROFILE", "C:\\AUP");
    EXPECT_EQ(value_of(win::site_cache_dir(ctx())), "C:/AUP/myapp/myapp/Cache");
    env.set("PROGRAMDATA", "C:\\PD");
    EXPECT_EQ(value_of(win::site_cache_dir(ctx())), "C:/PD/myapp/myapp/Cache");
}

// Author

TEST_F(WindowsRules, author_defaults_to_app_name)
{
    EXPECT_EQ(value_of(win::user_data_dir(ctx("myapp", "", Author::app_name_default()))),
              std::string(local) + "/myapp/myapp");
}

TEST_F(WindowsRules, author_explicit)
{
    EXPECT_EQ(value_of(win::user_data_dir(ctx("myapp", "", Author::named("Acme")))),
              std::string(local) + "/Acme/myapp");
}

TEST_F(WindowsRules, author_disabled)
{
    EXPECT_EQ(value_of(win::user_data_dir(ctx("myapp", "", Author::none()))),
              std::string(local) + "/myapp");
}

// Without an app name

TEST_F(WindowsRules, without_app_name)
{
    EXPECT_EQ(value_of(win::user_data_dir(ctx(""))), local);
    EXPECT_EQ(value_of(win::user_cache_dir(ctx(""))), local);
    EXPECT_EQ(value_of(win::user_log_dir(ctx(""))), std::string(local) + "/Logs");
}

// Folder answers and fallbacks

TEST_F(WindowsRules, trailing_backslash_in_folder)
{
    sys.set_folder(KnownFolder::LocalAppData, "C:\\Users\\t\\AppData\\Local\\");
    EXPECT_EQ(value_of(win::user_data_dir(ctx())), std::string(local) + "/myapp/myapp");
}

TEST_F(WindowsRules, forward_slashes_in_folder_are_normalized)
{
    sys.set_folder(KnownFolder::LocalAppData, "C:/Users/t/AppData/Local");
    EXPECT_EQ(value_of(win::user_data_dir(ctx())), std::string(local) + "/myapp/myapp");
}

TEST_F(WindowsRules, folder_outside_the_bmp_survives)
{
    sys.set_folder(KnownFolder::LocalAppData, "C:\\Users\\t\xF0\x9F\x98\x80\\AppData\\Local");
    EXPECT_EQ(value_of(win::user_data_dir(ctx())),
              "C:/Users/t\xF0\x9F\x98\x80/AppData/Local/myapp/myapp");
}

TEST_F(WindowsRules, falls_back_to_environment)
{
    sys.set_folder(KnownFolder::LocalAppData, std::nullopt);
    env.set("LOCALAPPDATA", "C:\\Env\\Local");
    EXPECT_EQ(value_of(win::user_data_dir(ctx())), "C:/Env/Local/myapp/myapp");
}

TEST_F(WindowsRules, roaming_falls_back_to_appdata)
{
    sys.set_folder(KnownFolder::RoamingAppData, std::nullopt);
    env.set("APPDATA", "C:\\Env\\Roaming");
    EXPECT_EQ(value_of(win::user_data_dir(ctx("myapp", "", Author(), true))),
              "C:/Env/Roaming/myapp/myapp");
}

TEST_F(WindowsRules, raises_when_nothing_resolves)
{
    sys.set_folder(KnownFolder::LocalAppData, std::nullopt);
    const auto r = win::user_data_dir(ctx());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, ErrorCode::Environment);
}

TEST_F(WindowsRules, relative_folder_answer_is_refused_and_falls_back)
{
    sys.set_folder(KnownFolder::LocalAppData, "AppData\\Local");
    env.set("LOCALAPPDATA", "C:\\Env\\Local");
    EXPECT_EQ(value_of(win::user_data_dir(ctx())), "C:/Env/Local/myapp/myapp");
}

TEST_F(WindowsRules, folder_answer_with_nul_falls_back)
{
    sys.set_folder(KnownFolder::LocalAppData, std::string("C:\\x\0y", 6));
    env.set("LOCALAPPDATA", "C:\\Env\\Local");
    EXPECT_EQ(value_of(win::user_data_dir(ctx())), "C:/Env/Local/myapp/myapp");
}

TEST_F(WindowsRules, unc_folder_is_accepted)
{
    sys.set_folder(KnownFolder::RoamingAppData, "\\\\server\\share\\t\\Roaming");
    EXPECT_EQ(value_of(win::user_data_dir(ctx("myapp", "", Author(), true))),
              "//server/share/t/Roaming/myapp/myapp");
}

TEST_F(WindowsRules, drive_relative_env_is_refused)
{
    sys.set_folder(KnownFolder::LocalAppData, std::nullopt);
    env.set("LOCALAPPDATA", "C:foo");
    EXPECT_FALSE(win::user_data_dir(ctx()).has_value());
}

TEST_F(WindowsRules, rootless_env_is_refused)
{
    sys.set_folder(KnownFolder::LocalAppData, std::nullopt);
    env.set("LOCALAPPDATA", "\\foo");
    EXPECT_FALSE(win::user_data_dir(ctx()).has_value());
}

TEST_F(WindowsRules, overlong_result_is_refused)
{
    sys.set_folder(KnownFolder::LocalAppData,
                   "C:\\" + std::string(max_path_units_windows - 6, 'a'));
    const auto r = win::user_data_dir(ctx());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, ErrorCode::PathTooLong);
}

// Media directories

TEST_F(WindowsRules, media_dirs_take_no_app_name)
{
    sys.set_folder(KnownFolder::Documents, "C:\\Users\\t\\Documents");
    sys.set_folder(KnownFolder::Downloads, "C:\\Users\\t\\Downloads");
    sys.set_folder(KnownFolder::Pictures, "C:\\Users\\t\\Pictures");
    sys.set_folder(KnownFolder::Videos, "C:\\Users\\t\\Videos");
    sys.set_folder(KnownFolder::Music, "C:\\Users\\t\\Music");
    sys.set_folder(KnownFolder::Desktop, "C:\\Users\\t\\Desktop");
    EXPECT_EQ(value_of(win::user_documents_dir(ctx("myapp", "1.0"))), "C:/Users/t/Documents");
    EXPECT_EQ(value_of(win::user_downloads_dir(ctx())), "C:/Users/t/Downloads");
    EXPECT_EQ(value_of(win::user_pictures_dir(ctx())), "C:/Users/t/Pictures");
    EXPECT_EQ(value_of(win::user_videos_dir(ctx())), "C:/Users/t/Videos");
    EXPECT_EQ(value_of(win::user_music_dir(ctx())), "C:/Users/t/Music");
    EXPECT_EQ(value_of(win::user_desktop_dir(ctx())), "C:/Users/t/Desktop");
}

TEST_F(WindowsRules, media_dirs_fall_back_to_the_profile_folder)
{
    EXPECT_EQ(value_of(win::user_documents_dir(ctx())), "C:/Users/t/Documents");
    EXPECT_EQ(value_of(win::user_downloads_dir(ctx())), "C:/Users/t/Downloads");
    EXPECT_EQ(value_of(win::user_pictures_dir(ctx())), "C:/Users/t/Pictures");
    EXPECT_EQ(value_of(win::user_videos_dir(ctx())), "C:/Users/t/Videos");
    EXPECT_EQ(value_of(win::user_music_dir(ctx())), "C:/Users/t/Music");
    EXPECT_EQ(value_of(win::user_desktop_dir(ctx())), "C:/Users/t/Desktop");
}

TEST_F(WindowsRules, media_dirs_fall_back_to_userprofile_env)
{
    sys.set_folder(KnownFolder::Profile, std::nullopt);
    env.set("USERPROFILE", "C:\\Env\\Profile");
    EXPECT_EQ(value_of(win::user_documents_dir(ctx())), "C:/Env/Profile/Documents");
}

TEST_F(WindowsRules, media_dirs_fall_back_to_the_system_home)
{
    sys.set_folder(KnownFolder::Profile, std::nullopt);
    sys.set_home("C:\\Sys\\Home");
    EXPECT_EQ(value_of(win::user_documents_dir(ctx())), "C:/Sys/Home/Documents");
}

TEST_F(WindowsRules, media_dirs_error_without_any_profile)
{
    sys.set_folder(KnownFolder::Profile, std::nullopt);
    EXPECT_FALSE(win::user_documents_dir(ctx()).has_value());
}

TEST_F(WindowsRules, projects_dir_is_under_the_profile)
{
    EXPECT_EQ(value_of(win::user_projects_dir(ctx())), "C:/Users/t/Projects");
}

TEST_F(WindowsRules, public_dir_from_folder_env_then_profile_parent)
{
    EXPECT_EQ(value_of(win::user_publicshare_dir(ctx())), "C:/Users/Public");
    env.set("PUBLIC", "C:\\Env\\Public");
    EXPECT_EQ(value_of(win::user_publicshare_dir(ctx())), "C:/Env/Public");
    sys.set_folder(KnownFolder::Public, "C:\\Folder\\Public");
    EXPECT_EQ(value_of(win::user_publicshare_dir(ctx())), "C:/Folder/Public");
}

TEST_F(WindowsRules, templates_dir_uses_roaming)
{
    EXPECT_EQ(value_of(win::user_templates_dir(ctx())),
              std::string(roaming) + "/Microsoft/Windows/Templates");
}

// Fonts, bin, applications

TEST_F(WindowsRules, fonts_dirs)
{
    EXPECT_EQ(value_of(win::user_fonts_dir(ctx())),
              std::string(local) + "/Microsoft/Windows/Fonts");
    sys.set_folder(KnownFolder::Fonts, "C:\\Windows\\Fonts");
    EXPECT_EQ(values_of(win::site_fonts_dirs(ctx())), std::vector<std::string>{"C:/Windows/Fonts"});
    sys.set_folder(KnownFolder::Fonts, std::nullopt);
    env.set("WINDIR", "C:\\Win");
    EXPECT_EQ(values_of(win::site_fonts_dirs(ctx())), std::vector<std::string>{"C:/Win/Fonts"});
    env.unset("WINDIR");
    EXPECT_FALSE(win::site_fonts_dirs(ctx()).has_value());
}

TEST_F(WindowsRules, bin_dirs)
{
    EXPECT_EQ(value_of(win::user_bin_dir(ctx())), std::string(local) + "/Programs");
    EXPECT_EQ(value_of(win::site_bin_dir(ctx())), "C:/ProgramData/bin");
}

TEST_F(WindowsRules, applications_dirs)
{
    sys.set_folder(KnownFolder::Programs,
                   "C:\\Users\\t\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs");
    sys.set_folder(KnownFolder::CommonPrograms,
                   "C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs");
    EXPECT_EQ(value_of(win::user_applications_dir(ctx())),
              std::string(roaming) + "/Microsoft/Windows/Start Menu/Programs");
    EXPECT_EQ(values_of(win::site_applications_dirs(ctx())),
              std::vector<std::string>{"C:/ProgramData/Microsoft/Windows/Start Menu/Programs"});
}

TEST_F(WindowsRules, applications_dirs_fall_back_to_environment)
{
    sys.set_folder(KnownFolder::RoamingAppData, std::nullopt);
    sys.set_folder(KnownFolder::ProgramData, std::nullopt);
    env.set("APPDATA", "C:\\Env\\Roaming");
    env.set("PROGRAMDATA", "C:\\PD");
    EXPECT_EQ(value_of(win::user_applications_dir(ctx())),
              "C:/Env/Roaming/Microsoft/Windows/Start Menu/Programs");
    EXPECT_EQ(values_of(win::site_applications_dirs(ctx())),
              std::vector<std::string>{"C:/PD/Microsoft/Windows/Start Menu/Programs"});
}

TEST_F(WindowsRules, public_dir_under_a_unc_profile_stays_on_the_share)
{
    sys.set_folder(KnownFolder::Profile, "\\\\srv\\share\\t");
    EXPECT_EQ(value_of(win::user_publicshare_dir(ctx())), "//srv/share/Public");
    sys.set_folder(KnownFolder::Profile, "\\\\srv\\share");
    EXPECT_EQ(value_of(win::user_publicshare_dir(ctx())), "//srv/share/Public");
    sys.set_folder(KnownFolder::Profile, "C:\\");
    EXPECT_EQ(value_of(win::user_publicshare_dir(ctx())), "C:/Public");
}
