// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/error.h>
#include <platformdirs/platform.h>
#include <platformdirs/version.h>

#include <gtest/gtest.h>

#include <string>
#include <string_view>

using namespace platformdirs;

TEST(Version, library_version_is_the_project_version)
{
    EXPECT_EQ(version, "0.1.0");
}

TEST(Version, part_number_is_the_dated_house_number)
{
    // SJ-PKG-0100 plus the release date; a stale build cache would leave it empty.
    ASSERT_EQ(part_number.size(), std::string_view("SJ-PKG-0100-YYYYMMDD").size());
    EXPECT_TRUE(part_number.starts_with("SJ-PKG-0100-"));
    for (const char c : part_number.substr(std::string_view("SJ-PKG-0100-").size())) {
        EXPECT_TRUE(c >= '0' && c <= '9') << part_number;
    }
}

TEST(Platform, host_platform_is_a_compile_time_constant)
{
    constexpr Platform host = host_platform();
#if defined(_WIN32)
    EXPECT_EQ(host, Platform::Windows);
#elif defined(__APPLE__)
    EXPECT_EQ(host, Platform::MacOS);
#else
    EXPECT_TRUE(is_unix_like(host));
#endif
}

TEST(Platform, separators_follow_the_target_not_the_host)
{
    EXPECT_EQ(separator(Platform::Windows), '\\');
    EXPECT_EQ(separator(Platform::Linux), '/');
    EXPECT_EQ(list_separator(Platform::Windows), ';');
    EXPECT_EQ(list_separator(Platform::MacOS), ':');
}

TEST(Platform, every_platform_has_a_name)
{
    EXPECT_EQ(platform_name(Platform::Linux), "linux");
    EXPECT_EQ(platform_name(Platform::FreeBsd), "freebsd");
    EXPECT_EQ(platform_name(Platform::OpenBsd), "openbsd");
    EXPECT_EQ(platform_name(Platform::MacOS), "macos");
    EXPECT_EQ(platform_name(Platform::Windows), "windows");
}

TEST(Error, invalid_name_names_the_role_and_the_value)
{
    const Error e = Error::invalid_name("version", "1.0/../..");
    EXPECT_EQ(e.code, ErrorCode::InvalidName);
    EXPECT_NE(e.message.find("version"), std::string::npos);
    EXPECT_NE(e.message.find("[1.0/../..]"), std::string::npos);
}

TEST(Error, path_too_long_reports_both_numbers)
{
    const Error e = Error::path_too_long(5000, 4096);
    EXPECT_EQ(e.code, ErrorCode::PathTooLong);
    EXPECT_NE(e.message.find("5000"), std::string::npos);
    EXPECT_NE(e.message.find("4096"), std::string::npos);
}

TEST(Error, result_carries_a_value_or_an_error)
{
    const Result<int> ok = 42;
    ASSERT_TRUE(ok.has_value());
    EXPECT_EQ(*ok, 42);

    const Result<int> failed = std::unexpected(Error::environment("no home"));
    ASSERT_FALSE(failed.has_value());
    EXPECT_EQ(failed.error().code, ErrorCode::Environment);
    EXPECT_EQ(failed.error().message, "no home");
}

TEST(Error, messages_are_bounded_and_printable)
{
    const std::string hostile = "\x1b[31m" + std::string(1024 * 1024, 'x');
    const Error e = Error::invalid_name("app name", hostile);
    EXPECT_LT(e.message.size(), 300u);
    EXPECT_EQ(e.message.find('\x1b'), std::string::npos);
    EXPECT_NE(e.message.find("..."), std::string::npos);
    EXPECT_EQ(Error::excerpt("plain"), "plain");
}

TEST(Error, excerpt_cuts_on_a_character_boundary_and_hides_invisible_text)
{
    const std::string umlauts = std::string(119, 'a') + "\xC3\xBC";
    const std::string cut = Error::excerpt(umlauts);
    EXPECT_TRUE(cut.ends_with("...")) << cut;
    EXPECT_EQ(cut.find('\xC3'), std::string::npos);
    EXPECT_EQ(Error::excerpt("a\xE2\x80\xAE"
                             "b"),
              "a?b");
    EXPECT_EQ(Error::excerpt("a\xC2\x85"
                             "b"),
              "a?b");
    EXPECT_EQ(Error::excerpt("a\xC0\xAF"
                             "b"),
              "a??b");
    EXPECT_EQ(Error::excerpt("Gr\xC3\xBC\xC3\x9F"
                             "e"),
              "Gr\xC3\xBC\xC3\x9F"
              "e");
}
