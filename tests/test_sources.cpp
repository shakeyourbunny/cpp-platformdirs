// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/sources.h>
#include <platformdirs/testing.h>

#include <gtest/gtest.h>

#include <string>

using namespace platformdirs;

TEST(FixedEnvironment, returns_set_value)
{
    FixedEnvironment env;
    env.set("XDG_DATA_HOME", "/custom/data");
    EXPECT_EQ(env.get("XDG_DATA_HOME"), "/custom/data");
}

TEST(FixedEnvironment, returns_nullopt_when_unset)
{
    const FixedEnvironment env;
    EXPECT_FALSE(env.get("XDG_DATA_HOME").has_value());
}

TEST(FixedEnvironment, empty_value_is_set_not_unset)
{
    FixedEnvironment env;
    env.set("XDG_DATA_HOME", "");
    ASSERT_TRUE(env.get("XDG_DATA_HOME").has_value());
    EXPECT_EQ(*env.get("XDG_DATA_HOME"), "");
}

TEST(FixedEnvironment, unset_removes)
{
    FixedEnvironment env;
    env.set("HOME", "/home/x");
    env.unset("HOME");
    EXPECT_FALSE(env.get("HOME").has_value());
}

TEST(FixedEnvironment, clear_removes_everything)
{
    FixedEnvironment env;
    env.set("A", "1");
    env.set("B", "2");
    env.clear();
    EXPECT_FALSE(env.get("A").has_value());
    EXPECT_FALSE(env.get("B").has_value());
}

TEST(FixedEnvironment, set_overwrites)
{
    FixedEnvironment env;
    env.set("A", "1");
    env.set("A", "2");
    EXPECT_EQ(env.get("A"), "2");
}

TEST(FixedSystem, home_is_nullopt_when_unconfigured)
{
    const FixedSystem sys;
    EXPECT_FALSE(sys.home_directory().has_value());
}

TEST(FixedSystem, home_returns_what_was_set)
{
    FixedSystem sys;
    sys.set_home("/home/test");
    EXPECT_EQ(sys.home_directory(), "/home/test");
    sys.set_home(std::nullopt);
    EXPECT_FALSE(sys.home_directory().has_value());
}

TEST(FixedSystem, user_id_defaults_to_a_normal_user)
{
    const FixedSystem sys;
    EXPECT_EQ(sys.user_id(), 1000u);
}

TEST(FixedSystem, user_id_can_be_root_or_absent)
{
    FixedSystem sys;
    sys.set_user_id(0);
    EXPECT_EQ(sys.user_id(), 0u);
    sys.set_user_id(std::nullopt);
    EXPECT_FALSE(sys.user_id().has_value());
}

TEST(FixedSystem, reads_registered_file)
{
    FixedSystem sys;
    sys.add_file("/home/test/.config/user-dirs.dirs", "XDG_DOCUMENTS_DIR=\"$HOME/Docs\"\n");
    const auto content = sys.read_text_file("/home/test/.config/user-dirs.dirs", 1024);
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, "XDG_DOCUMENTS_DIR=\"$HOME/Docs\"\n");
}

TEST(FixedSystem, read_is_capped_at_max_bytes)
{
    FixedSystem sys;
    sys.add_file("/big", std::string(2 * 1024 * 1024, 'x'));
    const auto content = sys.read_text_file("/big", max_config_file_bytes);
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(content->size(), max_config_file_bytes);
}

TEST(FixedSystem, unknown_file_is_nullopt)
{
    const FixedSystem sys;
    EXPECT_FALSE(sys.read_text_file("/nowhere", 1024).has_value());
}

TEST(FixedSystem, known_folder_nullopt_when_unset)
{
    const FixedSystem sys;
    EXPECT_FALSE(sys.known_folder(KnownFolder::LocalAppData).has_value());
}

TEST(FixedSystem, known_folder_returns_what_was_set)
{
    FixedSystem sys;
    sys.set_folder(KnownFolder::LocalAppData, "C:\\Users\\t\\AppData\\Local");
    EXPECT_EQ(sys.known_folder(KnownFolder::LocalAppData), "C:\\Users\\t\\AppData\\Local");
    EXPECT_FALSE(sys.known_folder(KnownFolder::Fonts).has_value());
    sys.set_folder(KnownFolder::LocalAppData, std::nullopt);
    EXPECT_FALSE(sys.known_folder(KnownFolder::LocalAppData).has_value());
}

TEST(FixedSystem, writable_directory_only_when_added)
{
    FixedSystem sys;
    EXPECT_FALSE(sys.is_writable_directory("/run/user/1000"));
    sys.add_writable_directory("/run/user/1000");
    EXPECT_TRUE(sys.is_writable_directory("/run/user/1000"));
}

TEST(FixedSystem, clear_resets_everything)
{
    FixedSystem sys;
    sys.set_home("/home/test");
    sys.set_user_id(0);
    sys.add_file("/f", "x");
    sys.add_writable_directory("/d");
    sys.set_folder(KnownFolder::Profile, "C:\\Users\\t");
    sys.clear();
    EXPECT_FALSE(sys.home_directory().has_value());
    EXPECT_EQ(sys.user_id(), 1000u);
    EXPECT_FALSE(sys.read_text_file("/f", 10).has_value());
    EXPECT_FALSE(sys.is_writable_directory("/d"));
    EXPECT_FALSE(sys.known_folder(KnownFolder::Profile).has_value());
}
