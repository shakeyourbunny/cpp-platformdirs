// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

// Host tests against the real sources. Every variable touched is restored, HOME points
// at a path that does not exist, and files go into a private temp dir removed afterwards.

#include <platformdirs/platform.h>
#include <platformdirs/sources.h>
#include <platformdirs/validation.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <random>
#include <string>

#if !defined(_WIN32)
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace platformdirs;
namespace fs = std::filesystem;

namespace {

constexpr const char* fake_home = "/home/platformdirs-test-does-not-exist";

std::optional<std::string> read_env(const char* name)
{
    const char* v = std::getenv(name);
    return v == nullptr ? std::nullopt : std::optional<std::string>(v);
}

void write_env(const char* name, const std::optional<std::string>& value)
{
#if defined(_WIN32)
    _putenv_s(name, value ? value->c_str() : "");
#else
    if (value) {
        setenv(name, value->c_str(), 1);
    } else {
        unsetenv(name);
    }
#endif
}

class NativeSources : public ::testing::Test {
protected:
    static constexpr const char* touched[] = {"HOME", "PLATFORMDIRS_TEST_SET",
                                              "PLATFORMDIRS_TEST_EMPTY", "PLATFORMDIRS_TEST_UNSET"};

    void SetUp() override
    {
        for (const char* name : touched) {
            saved_[name] = read_env(name);
            write_env(name, std::nullopt);
        }
        write_env("HOME", fake_home);

        std::random_device rd;
        const fs::path base = fs::temp_directory_path();
        fs::path candidate;
        for (int i = 0; i < 100; ++i) {
            candidate = base / ("platformdirs-test-" + std::to_string(rd()));
            if (fs::create_directory(candidate)) {
                break;
            }
            candidate.clear();
        }
        ASSERT_FALSE(candidate.empty()) << "could not create a private temp dir";
        fs::permissions(candidate, fs::perms::owner_all, fs::perm_options::replace);
        temp_dir_ = candidate;
    }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(temp_dir_, ec);
        // Every resolved directory hangs off HOME, which points at nothing; a
        // directory the library created would show up here.
        EXPECT_FALSE(fs::exists(fake_home)) << "the library created a directory under HOME";
        for (const char* name : touched) {
            write_env(name, saved_[name]);
        }
    }

    fs::path temp_dir_;
    std::map<std::string, std::optional<std::string>> saved_;
};

} // namespace

TEST_F(NativeSources, environment_returns_a_set_variable)
{
    write_env("PLATFORMDIRS_TEST_SET", "value");
    EXPECT_EQ(native_environment().get("PLATFORMDIRS_TEST_SET"), "value");
}

TEST_F(NativeSources, environment_returns_nullopt_for_unset)
{
    EXPECT_FALSE(native_environment().get("PLATFORMDIRS_TEST_UNSET").has_value());
}

#if !defined(_WIN32)
TEST_F(NativeSources, environment_returns_empty_for_empty)
{
    // Windows cannot hold an empty variable; setting one deletes it.
    write_env("PLATFORMDIRS_TEST_EMPTY", "");
    const auto v = native_environment().get("PLATFORMDIRS_TEST_EMPTY");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "");
}
#endif

TEST_F(NativeSources, home_from_the_environment_is_returned_as_is)
{
    const auto home = native_system().home_directory();
    ASSERT_TRUE(home.has_value());
#if defined(_WIN32)
    EXPECT_TRUE(validation::is_absolute(Platform::Windows, *home));
#else
    EXPECT_EQ(*home, fake_home);
#endif
}

#if !defined(_WIN32)
TEST_F(NativeSources, home_falls_back_to_passwd_when_home_is_unset)
{
    write_env("HOME", std::nullopt);
    const auto home = native_system().home_directory();
    ASSERT_TRUE(home.has_value());
    EXPECT_TRUE(validation::is_absolute(host_platform(), *home));
}

TEST_F(NativeSources, home_ignores_a_relative_home)
{
    write_env("HOME", "relative/home");
    const auto home = native_system().home_directory();
    ASSERT_TRUE(home.has_value());
    EXPECT_NE(*home, "relative/home");
    EXPECT_TRUE(validation::is_absolute(host_platform(), *home));
}

TEST_F(NativeSources, user_id_is_the_real_uid)
{
    EXPECT_EQ(native_system().user_id(), static_cast<unsigned long>(getuid()));
}

TEST_F(NativeSources, known_folders_do_not_exist_on_posix)
{
    EXPECT_FALSE(native_system().known_folder(KnownFolder::LocalAppData).has_value());
}
#else
TEST_F(NativeSources, user_id_is_absent_on_windows)
{
    EXPECT_FALSE(native_system().user_id().has_value());
}

TEST_F(NativeSources, every_known_folder_resolves_to_an_absolute_utf8_path)
{
    for (const KnownFolder f :
         {KnownFolder::LocalAppData, KnownFolder::RoamingAppData, KnownFolder::ProgramData,
          KnownFolder::Profile, KnownFolder::Documents, KnownFolder::Downloads,
          KnownFolder::Pictures, KnownFolder::Videos, KnownFolder::Music, KnownFolder::Desktop,
          KnownFolder::Programs, KnownFolder::CommonPrograms, KnownFolder::Fonts,
          KnownFolder::Public}) {
        const auto p = native_system().known_folder(f);
        ASSERT_TRUE(p.has_value()) << static_cast<int>(f);
        EXPECT_TRUE(validation::is_usable_base(Platform::Windows, *p)) << *p;
    }
}
#endif

TEST_F(NativeSources, read_text_file_caps_at_max_bytes)
{
    const fs::path file = temp_dir_ / "big.txt";
    {
        std::ofstream out(file, std::ios::binary);
        out << std::string(2 * 1024 * 1024, 'x');
    }
    const auto content = native_system().read_text_file(file.string(), max_config_file_bytes);
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(content->size(), max_config_file_bytes);
}

TEST_F(NativeSources, read_text_file_returns_a_small_file_whole)
{
    const fs::path file = temp_dir_ / "small.txt";
    {
        std::ofstream out(file, std::ios::binary);
        out << "XDG_DOCUMENTS_DIR=\"$HOME/Docs\"\n";
    }
    const auto content = native_system().read_text_file(file.string(), max_config_file_bytes);
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, "XDG_DOCUMENTS_DIR=\"$HOME/Docs\"\n");
}

TEST_F(NativeSources, read_text_file_nullopt_for_a_directory)
{
    EXPECT_FALSE(native_system().read_text_file(temp_dir_.string(), 1024).has_value());
}

TEST_F(NativeSources, read_text_file_nullopt_for_missing)
{
    EXPECT_FALSE(
        native_system().read_text_file((temp_dir_ / "missing").string(), 1024).has_value());
}

TEST_F(NativeSources, writable_directory_true_for_the_private_temp_dir)
{
    EXPECT_TRUE(native_system().is_writable_directory(temp_dir_.string()));
}

TEST_F(NativeSources, writable_directory_false_for_a_file_and_for_missing)
{
    const fs::path file = temp_dir_ / "file.txt";
    std::ofstream(file) << "x";
    EXPECT_FALSE(native_system().is_writable_directory(file.string()));
    EXPECT_FALSE(native_system().is_writable_directory((temp_dir_ / "missing").string()));
}

TEST_F(NativeSources, sources_are_singletons)
{
    EXPECT_EQ(&native_environment(), &native_environment());
    EXPECT_EQ(&native_system(), &native_system());
}

#if !defined(_WIN32)
TEST_F(NativeSources, home_with_a_control_character_falls_back_to_passwd)
{
    write_env("HOME", "/bad\x01home");
    const auto home = native_system().home_directory();
    ASSERT_TRUE(home.has_value());
    EXPECT_NE(*home, "/bad\x01home");
    EXPECT_TRUE(validation::is_usable_base(host_platform(), *home));
}

TEST_F(NativeSources, read_text_file_does_not_block_on_a_fifo)
{
    const fs::path fifo = temp_dir_ / "fifo";
    ASSERT_EQ(mkfifo(fifo.c_str(), 0600), 0);
    EXPECT_FALSE(native_system().read_text_file(fifo.string(), 1024).has_value());
}
#endif
