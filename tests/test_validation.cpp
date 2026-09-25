// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/validation.h>

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

using namespace platformdirs;
using namespace platformdirs::validation;
using namespace std::literals;

namespace {

struct Case {
    const char* label;
    std::string value;
};

std::string repeated(std::string_view unit, int times)
{
    std::string s;
    for (int i = 0; i < times; ++i) {
        s += unit;
    }
    return s;
}

// Design decision 3: every row here is refused on every platform.
const std::vector<Case> refused_components = {
    {"dot", "."},
    {"dot-dot", ".."},
    {"posix traversal", "../../../../etc"},
    {"traversal in the middle", "foo/../../bar"},
    {"windows traversal", "..\\..\\windows"},
    {"windows separator", "a\\b"},
    {"absolute posix", "/etc"},
    {"absolute windows", "C:\\Windows"},
    {"unc", "\\\\server\\share"},
    {"embedded nul", "myapp\0/../.."s},
    {"c0 control", "file\x1fname"},
    {"del", "file\x7fname"},
    {"c1 control U+0085", "file\xC2\x85name"},
    {"colon", "my:app"},
    {"question mark", "my?app"},
    {"asterisk", "my*app"},
    {"pipe", "my|app"},
    {"quote", "my\"app"},
    {"angle brackets", "<app>"},
    {"leading space", " myapp"},
    {"trailing space", "myapp "},
    {"trailing dot", "myapp."},
    {"device CON", "CON"},
    {"device lowercase", "con"},
    {"device NUL", "NUL"},
    {"device with extension", "COM1.log"},
    {"device LPT9", "LPT9"},
    {"device COM0", "COM0"},
    {"device superscript COM1", "COM\xC2\xB9"},
    {"device superscript LPT3", "LPT\xC2\xB3"},
    {"zero width space", "my\xE2\x80\x8B"
                         "app"},
    {"rtl override", "\xE2\x80\xAE"
                     "app"},
    {"bom", "\xEF\xBB\xBF"
            "app"},
    {"word joiner", "my\xE2\x81\xA0"
                    "app"},
    {"tag character", "app\xF3\xA0\x81\x81"},
    {"arabic letter mark", "app\xD8\x9C"},
    {"mongolian vowel separator", "app\xE1\xA0\x8E"},
    {"fullwidth solidus", "my\xEF\xBC\x8F"
                          "app"},
    {"fullwidth reverse solidus", "my\xEF\xBC\xBC"
                                  "app"},
    {"overlong slash", "my\xC0\xAF"
                       "app"},
    {"encoded surrogate", "my\xED\xA0\x80"
                          "app"},
    {"truncated utf8", "my\xE2\x82"},
    {"256 bytes", std::string(256, 'a')},
    {"256 bytes of emoji", repeated("\xF0\x9F\x98\x80", 64)},
};

const std::vector<Case> accepted_components = {
    {"empty means not supplied", ""},
    {"plain", "myapp"},
    {"inner space", "my app"},
    {"leading dot", ".hidden"},
    {"inner dots", "my.app.name"},
    {"dash and underscore", "my-app_2"},
    {"device-like prefix", "CONSOLE"},
    {"device-like with letters", "NULL"},
    {"device inside a word", "myNUL"},
    {"com without digit", "COM"},
    {"com with two digits", "COM10"},
    {"umlauts", "Gr\xC3\xBC\xC3\x9F"
                "e"},
    {"cjk", "\xE3\x83\x86\xE3\x82\xB9\xE3\x83\x88"},
    {"emoji", "app\xF0\x9F\x98\x80"},
    {"255 bytes", std::string(255, 'a')},
    {"252 bytes of emoji", repeated("\xF0\x9F\x98\x80", 63)},
};

} // namespace

TEST(Validation, refuses_every_hostile_component)
{
    for (const auto& c : refused_components) {
        EXPECT_FALSE(is_valid_component(c.value)) << c.label;
    }
}

TEST(Validation, accepts_every_ordinary_component)
{
    for (const auto& c : accepted_components) {
        EXPECT_TRUE(is_valid_component(c.value)) << c.label;
    }
}

TEST(Validation, check_component_reports_the_role)
{
    const Status s = check_component("..", "app name");
    ASSERT_FALSE(s.has_value());
    EXPECT_EQ(s.error().code, ErrorCode::InvalidName);
    EXPECT_NE(s.error().message.find("app name"), std::string::npos);
    EXPECT_NE(s.error().message.find("[..]"), std::string::npos);
}

TEST(Validation, check_component_accepts_a_good_name)
{
    EXPECT_TRUE(check_component("myapp", "app name").has_value());
    EXPECT_TRUE(check_component("", "version").has_value());
}

TEST(Validation, posix_absolute_starts_with_slash)
{
    EXPECT_TRUE(is_absolute(Platform::Linux, "/"));
    EXPECT_TRUE(is_absolute(Platform::MacOS, "/home/x"));
    EXPECT_TRUE(is_absolute(Platform::FreeBsd, "/usr/local"));
    EXPECT_FALSE(is_absolute(Platform::Linux, "home/x"));
    EXPECT_FALSE(is_absolute(Platform::Linux, ""));
    EXPECT_FALSE(is_absolute(Platform::Linux, "C:\\x"));
    EXPECT_FALSE(is_absolute(Platform::Linux, "~/x"));
}

TEST(Validation, windows_absolute_is_drive_rooted_or_unc)
{
    EXPECT_TRUE(is_absolute(Platform::Windows, "C:\\Users"));
    EXPECT_TRUE(is_absolute(Platform::Windows, "c:/Users"));
    EXPECT_TRUE(is_absolute(Platform::Windows, "C:\\"));
    EXPECT_TRUE(is_absolute(Platform::Windows, "\\\\server\\share"));
    EXPECT_TRUE(is_absolute(Platform::Windows, "//server/share"));
    EXPECT_FALSE(is_absolute(Platform::Windows, "C:foo"));
    EXPECT_FALSE(is_absolute(Platform::Windows, "\\foo"));
    EXPECT_FALSE(is_absolute(Platform::Windows, "/foo"));
    EXPECT_FALSE(is_absolute(Platform::Windows, "C:"));
    EXPECT_FALSE(is_absolute(Platform::Windows, "\\\\"));
    EXPECT_FALSE(is_absolute(Platform::Windows, "\\\\\\x"));
    EXPECT_FALSE(is_absolute(Platform::Windows, ""));
}

TEST(Validation, usable_base_refuses_blank_relative_nul_and_overlong)
{
    EXPECT_FALSE(is_usable_base(Platform::Linux, ""));
    EXPECT_FALSE(is_usable_base(Platform::Linux, "   "));
    EXPECT_FALSE(is_usable_base(Platform::Linux, "\t"));
    EXPECT_FALSE(is_usable_base(Platform::Linux, "relative/data"));
    EXPECT_FALSE(is_usable_base(Platform::Linux, "../../etc"));
    EXPECT_FALSE(is_usable_base(Platform::Linux, "/tmp/x\0/y"s));
    EXPECT_FALSE(is_usable_base(Platform::Linux, "/tmp/x\ny"));
    EXPECT_FALSE(is_usable_base(Platform::Linux, "/\xC0\xAF"));
    EXPECT_FALSE(is_usable_base(Platform::Linux, "/" + std::string(4096, 'a')));
    EXPECT_FALSE(is_usable_base(Platform::Windows, "C:foo"));
    EXPECT_FALSE(is_usable_base(Platform::Windows, "\\foo"));
    EXPECT_FALSE(is_usable_base(Platform::Windows, "relative\\data"));
}

TEST(Validation, usable_base_accepts_absolute_values)
{
    EXPECT_TRUE(is_usable_base(Platform::Linux, "/"));
    EXPECT_TRUE(is_usable_base(Platform::Linux, "/home/x/.local/share"));
    EXPECT_TRUE(is_usable_base(Platform::Linux, "/usr/../etc")); // absolute, not normalized
    EXPECT_TRUE(is_usable_base(Platform::Linux, "/home/x/with space"));
    EXPECT_TRUE(is_usable_base(Platform::Windows, "C:\\Users\\x"));
    EXPECT_TRUE(is_usable_base(Platform::Windows, "\\\\server\\share\\x"));
    EXPECT_TRUE(is_usable_base(Platform::Linux, "/" + std::string(4095, 'a')));
}

TEST(Validation, control_characters_are_detected)
{
    EXPECT_TRUE(contains_control_character("a\x01"));
    EXPECT_TRUE(contains_control_character("a\x7f"));
    EXPECT_TRUE(contains_control_character("\0"s));
    EXPECT_FALSE(contains_control_character("plain text"));
    EXPECT_FALSE(contains_control_character(""));
    EXPECT_FALSE(contains_control_character("\xC3\xA9")); // high bytes are not controls
}

TEST(Validation, path_length_limits_per_platform)
{
    EXPECT_TRUE(check_path_length(Platform::Linux, "/" + std::string(4095, 'a')).has_value());
    const Status over = check_path_length(Platform::Linux, "/" + std::string(4096, 'a'));
    ASSERT_FALSE(over.has_value());
    EXPECT_EQ(over.error().code, ErrorCode::PathTooLong);

    EXPECT_TRUE(check_path_length(Platform::Windows, "C:\\" + std::string(32764, 'a')).has_value());
    EXPECT_FALSE(
        check_path_length(Platform::Windows, "C:\\" + std::string(32765, 'a')).has_value());

    // 4-byte code points count double on Windows and quadruple on POSIX.
    std::string wide = repeated("\xF0\x9F\x98\x80", 16382);
    EXPECT_TRUE(check_path_length(Platform::Windows, "C:\\" + wide).has_value());
    wide += "\xF0\x9F\x98\x80";
    EXPECT_FALSE(check_path_length(Platform::Windows, "C:\\" + wide).has_value());
}

TEST(Validation, path_length_on_windows_counts_bytes_when_input_is_malformed)
{
    // Malformed UTF-8 cannot be measured in UTF-16 units; the byte
    // count is the conservative substitute and must not crash.
    EXPECT_TRUE(check_path_length(Platform::Windows, "C:\\\xC0\xAF").has_value());
}

TEST(Validation, windows_device_namespaces_are_not_absolute_bases)
{
    EXPECT_FALSE(is_absolute(Platform::Windows, "\\\\.\\NUL"));
    EXPECT_FALSE(is_absolute(Platform::Windows, "\\\\?\\C:\\x"));
    EXPECT_FALSE(is_usable_base(Platform::Windows, "//./pipe/x"));
    EXPECT_TRUE(is_absolute(Platform::Windows, "\\\\srv\\share"));
}

TEST(Validation, device_stems_with_trailing_spaces_and_console_names_are_refused)
{
    EXPECT_FALSE(is_valid_component("CON .txt"));
    EXPECT_FALSE(is_valid_component("nul  .log"));
    EXPECT_FALSE(is_valid_component("CONIN$"));
    EXPECT_FALSE(is_valid_component("conout$.txt"));
    EXPECT_TRUE(is_valid_component("CONIN"));
}

TEST(Validation, unc_without_a_share_is_not_absolute)
{
    EXPECT_FALSE(is_absolute(Platform::Windows, "\\\\srv"));
    EXPECT_FALSE(is_absolute(Platform::Windows, "\\\\srv\\"));
    EXPECT_FALSE(is_absolute(Platform::Windows, "\\\\srv\\\\x"));
    EXPECT_TRUE(is_absolute(Platform::Windows, "\\\\srv\\share"));
}

TEST(Validation, trim_whitespace_uses_pythons_set)
{
    EXPECT_EQ(trim_whitespace("  /x\t\r\n"), "/x");
    EXPECT_EQ(trim_whitespace("/x\xC2\xA0"), "/x");         // no-break space
    EXPECT_EQ(trim_whitespace("\xE3\x80\x80/x"), "/x");     // ideographic space
    EXPECT_EQ(trim_whitespace("/x\xC2\x85"), "/x");         // NEL
    EXPECT_EQ(trim_whitespace("/x\xC3\xBC"), "/x\xC3\xBC"); // an umlaut stays
    EXPECT_EQ(trim_whitespace(""), "");
    EXPECT_EQ(trim_whitespace(" \xC2\xA0 "), "");
}
