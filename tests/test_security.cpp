// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

// The attack scenarios of design section 4 through the public API: the Ada port's
// sixteen security tests first, then the additions.

#include <platformdirs/platformdirs.h>
#include <platformdirs/testing.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

using namespace platformdirs;
using namespace std::literals;

namespace {

constexpr const char* home = "/home/test";
constexpr const char* data_default = "/home/test/.local/share/myapp";

// Generic form regardless of host: a Windows result on Linux carries
// backslashes that generic_string() leaves alone.
std::string text(const Result<Path>& r)
{
    if (!r.has_value()) {
        return "<error: " + r.error().message + ">";
    }
    // u8string(), not string(): on Windows string() converts to the
    // ANSI code page and throws for a character it cannot represent.
    const std::u8string u = r->u8string();
    std::string s(u.begin(), u.end());
    std::ranges::replace(s, '\\', '/');
    return s;
}

class Security : public ::testing::Test {
protected:
    void SetUp() override
    {
        env.clear();
        sys.clear();
        sys.set_home(home);
        sys.set_user_id(1000);
    }

    Result<Dirs> make(std::string name, std::string version = "", Author author = Author(),
                      Platform platform = Platform::Linux)
    {
        Options o;
        o.app_name = std::move(name);
        o.version = std::move(version);
        o.author = author;
        o.platform = platform;
        return Dirs::create(std::move(o), env, sys);
    }

    void refuses_name(const std::string& name, const char* label)
    {
        const auto d = make(name);
        EXPECT_FALSE(d.has_value())
            << label << ": accepted, resolved to " << (d ? text(d->user_data_dir()) : "");
        if (!d) {
            EXPECT_EQ(d.error().code, ErrorCode::InvalidName) << label;
        }
    }

    void refuses_author(const std::string& name, const char* label)
    {
        const auto d = make("myapp", "", Author::named(name));
        EXPECT_FALSE(d.has_value()) << label;
    }

    void refuses_version(const std::string& version, const char* label)
    {
        const auto d = make("myapp", version);
        EXPECT_FALSE(d.has_value()) << label;
    }

    std::string data_dir()
    {
        const auto d = make("myapp");
        return d ? text(d->user_data_dir()) : "<create failed: " + d.error().message + ">";
    }

    FixedEnvironment env;
    FixedSystem sys;
};

} // namespace

// Application metadata

TEST_F(Security, app_name_cannot_traverse_up)
{
    refuses_name("../../../../etc", "posix traversal");
    refuses_name("..", "bare dot dot");
    refuses_name("foo/../../bar", "traversal in the middle");
}

TEST_F(Security, app_name_cannot_traverse_with_backslash)
{
    // Rejected on every platform, so a name written on Linux cannot
    // turn into an escape once the same config runs on Windows.
    refuses_name("..\\..\\windows", "windows traversal");
    refuses_name("a\\b", "windows separator");
}

TEST_F(Security, app_name_cannot_be_absolute)
{
    refuses_name("/etc", "absolute posix name");
    refuses_name("C:\\Windows", "absolute windows name");
    refuses_name("\\\\server\\share", "unc name");
}

TEST_F(Security, app_name_cannot_hide_behind_nul)
{
    refuses_name("myapp\0/../.."s, "embedded NUL");
}

TEST_F(Security, app_name_cannot_be_a_device)
{
    refuses_name("NUL", "device name");
    refuses_name("COM1.log", "device name with extension");
    refuses_name("com\xC2\xB2", "superscript device name");
}

TEST_F(Security, app_name_cannot_end_in_dot_or_space)
{
    refuses_name("myapp.", "trailing dot");
    refuses_name("myapp ", "trailing space");
    refuses_name(" myapp", "leading space");
}

TEST_F(Security, author_cannot_traverse_up)
{
    refuses_author("..", "author dot dot");
    refuses_author("../../etc", "author traversal");
}

TEST_F(Security, version_cannot_traverse_up)
{
    refuses_version("../../..", "version traversal");
    refuses_version("1.0/../..", "version traversal in the middle");
}

TEST_F(Security, invisible_characters_in_names_are_refused)
{
    refuses_name("my\xE2\x80\x8B"
                 "app",
                 "zero width space");
    refuses_name("\xE2\x80\xAE"
                 "app",
                 "rtl override");
    refuses_name("app\xF3\xA0\x81\x81", "tag character");
    refuses_name("\xEF\xBB\xBF"
                 "app",
                 "bom");
}

TEST_F(Security, lookalike_separators_are_refused)
{
    refuses_name("my\xEF\xBC\x8F"
                 "app",
                 "fullwidth solidus");
    refuses_name("my\xEF\xBC\xBC"
                 "app",
                 "fullwidth reverse solidus");
}

TEST_F(Security, malformed_utf8_in_names_is_refused)
{
    refuses_name("my\xC0\xAF"
                 "app",
                 "overlong slash");
    refuses_name("my\xED\xA0\x80"
                 "app",
                 "encoded surrogate");
    refuses_name("my\xE2\x82", "truncated sequence");
}

TEST_F(Security, overlong_name_is_refused_on_every_platform)
{
    const std::string long_name(256, 'a');
    for (const Platform p : {Platform::Linux, Platform::FreeBsd, Platform::OpenBsd, Platform::MacOS,
                             Platform::Windows}) {
        EXPECT_FALSE(make(long_name, "", Author(), p).has_value()) << platform_name(p);
    }
}

// Hostile environment

TEST_F(Security, relative_env_value_is_ignored)
{
    env.set("XDG_DATA_HOME", "relative/data");
    EXPECT_EQ(data_dir(), data_default);
}

TEST_F(Security, env_value_with_nul_is_ignored)
{
    env.set("XDG_DATA_HOME", "/tmp/x\0/y"s);
    EXPECT_EQ(data_dir(), data_default);
}

TEST_F(Security, traversing_env_value_is_ignored)
{
    env.set("XDG_DATA_HOME", "../../etc");
    EXPECT_EQ(data_dir(), data_default);
}

TEST_F(Security, overlong_env_value_is_ignored)
{
    env.set("XDG_DATA_HOME", "/" + std::string(5000, 'a'));
    EXPECT_EQ(data_dir(), data_default);
}

TEST_F(Security, relative_search_path_entry_is_dropped)
{
    env.set("XDG_DATA_DIRS", "/a:../evil:/b");
    const auto d = make("myapp");
    ASSERT_TRUE(d.has_value());
    const auto dirs = d->site_data_dirs();
    ASSERT_TRUE(dirs.has_value());
    ASSERT_EQ(dirs->size(), 2u);
    EXPECT_EQ((*dirs)[0].generic_string(), "/a/myapp");
    EXPECT_EQ((*dirs)[1].generic_string(), "/b/myapp");
}

TEST_F(Security, overlong_result_is_refused)
{
    env.set("XDG_DATA_HOME", "/" + std::string(max_path_bytes_posix - 4, 'a'));
    const auto d = make("myapp");
    ASSERT_TRUE(d.has_value());
    const auto r = d->user_data_dir();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, ErrorCode::PathTooLong);
}

TEST_F(Security, home_from_the_system_that_is_relative_is_refused)
{
    sys.set_home("relative");
    const auto d = make("myapp");
    ASSERT_TRUE(d.has_value());
    const auto r = d->user_data_dir();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, ErrorCode::Environment);
}

TEST_F(Security, unc_env_base_is_accepted_on_windows)
{
    // Folder redirection puts APPDATA on a share in real setups
    // (design decision 4).
    env.set("LOCALAPPDATA", "\\\\server\\share\\t\\Local");
    const auto d = make("myapp", "", Author(), Platform::Windows);
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(text(d->user_data_dir()), "//server/share/t/Local/myapp/myapp");
}

TEST_F(Security, drive_relative_env_base_is_refused_on_windows)
{
    env.set("LOCALAPPDATA", "C:foo");
    const auto d = make("myapp", "", Author(), Platform::Windows);
    ASSERT_TRUE(d.has_value());
    EXPECT_FALSE(d->user_data_dir().has_value());
}

// Hostile config file

TEST_F(Security, user_dirs_file_cannot_point_outside)
{
    sys.add_file("/home/test/.config/user-dirs.dirs", "XDG_DOCUMENTS_DIR=\"../../../etc\"\n");
    const auto d = make("myapp");
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(text(d->user_documents_dir()), "/home/test/Documents");
}

TEST_F(Security, huge_user_dirs_file_is_bounded)
{
    std::string junk;
    const std::string line = "# " + std::string(200, 'x') + "\n";
    for (int i = 0; i < 20000; ++i) {
        junk += line;
    }
    sys.add_file("/home/test/.config/user-dirs.dirs", junk + "XDG_DOCUMENTS_DIR=\"/late\"\n");
    const auto d = make("myapp");
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(text(d->user_documents_dir()), "/home/test/Documents");
}

// Containment

TEST_F(Security, resolved_path_is_always_under_the_base)
{
    const std::vector<std::string> names = {"myapp",
                                            "my app",
                                            ".hidden",
                                            "my.app.name",
                                            "my-app_2",
                                            "Gr\xC3\xBC\xC3\x9F"
                                            "e",
                                            "app\xF0\x9F\x98\x80"};
    for (const std::string& name : names) {
        for (const std::string& version : {std::string(), std::string("1.0")}) {
            const auto d = make(name, version);
            ASSERT_TRUE(d.has_value()) << name;
            for (const auto& r : {d->user_data_dir(), d->user_config_dir(), d->user_cache_dir(),
                                  d->user_state_dir(), d->user_log_dir()}) {
                ASSERT_TRUE(r.has_value()) << name;
                EXPECT_TRUE(text(r).starts_with("/home/test/.")) << text(r);
                EXPECT_EQ(text(r).find(".."), std::string::npos) << text(r);
            }
        }
    }
}

TEST_F(Security, nothing_is_created)
{
    // SystemSource has no call that creates anything, so the doubles cannot; the
    // host test in test_native.cpp checks the real sources the same way.
    const auto d = make("myapp");
    ASSERT_TRUE(d.has_value());
    EXPECT_TRUE(d->user_data_dir().has_value());
    EXPECT_FALSE(sys.is_writable_directory("/home/test/.local/share/myapp"));
}
