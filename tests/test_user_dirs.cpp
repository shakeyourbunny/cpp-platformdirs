// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/user_dirs.h>

#include <gtest/gtest.h>

#include <string>

using namespace platformdirs::user_dirs;

namespace {
constexpr const char* home = "/home/test";
}

TEST(UserDirs, quoted_home_relative_value)
{
    EXPECT_EQ(lookup("XDG_DOCUMENTS_DIR=\"$HOME/Docs\"\n", "XDG_DOCUMENTS_DIR", home),
              "/home/test/Docs");
}

TEST(UserDirs, unquoted_absolute_value)
{
    EXPECT_EQ(lookup("XDG_MUSIC_DIR=/mnt/music\n", "XDG_MUSIC_DIR", home), "/mnt/music");
}

TEST(UserDirs, bare_home_is_the_home_directory)
{
    EXPECT_EQ(lookup("XDG_DESKTOP_DIR=\"$HOME\"\n", "XDG_DESKTOP_DIR", home), "/home/test");
}

TEST(UserDirs, comment_and_blank_lines_are_skipped)
{
    const std::string content = "# comment\n\n   \n# XDG_MUSIC_DIR=\"$HOME/Nope\"\n"
                                "XDG_MUSIC_DIR=\"$HOME/Music\"\n";
    EXPECT_EQ(lookup(content, "XDG_MUSIC_DIR", home), "/home/test/Music");
}

TEST(UserDirs, crlf_line_endings)
{
    EXPECT_EQ(lookup("XDG_DOCUMENTS_DIR=\"$HOME/Docs\"\r\nXDG_MUSIC_DIR=\"$HOME/M\"\r\n",
                     "XDG_MUSIC_DIR", home),
              "/home/test/M");
}

TEST(UserDirs, surrounding_whitespace_is_trimmed)
{
    EXPECT_EQ(lookup("  XDG_MUSIC_DIR = \"$HOME/Music\"  \n", "XDG_MUSIC_DIR", home),
              "/home/test/Music");
}

TEST(UserDirs, last_assignment_wins)
{
    const std::string content =
        "XDG_DOCUMENTS_DIR=\"$HOME/First\"\nXDG_DOCUMENTS_DIR=\"$HOME/Second\"\n";
    EXPECT_EQ(lookup(content, "XDG_DOCUMENTS_DIR", home), "/home/test/Second");
}

TEST(UserDirs, last_usable_assignment_wins_over_a_later_unusable_one)
{
    const std::string content = "XDG_DOCUMENTS_DIR=\"$HOME/Good\"\nXDG_DOCUMENTS_DIR=\"bad\"\n";
    EXPECT_EQ(lookup(content, "XDG_DOCUMENTS_DIR", home), "/home/test/Good");
}

TEST(UserDirs, missing_key_is_nullopt)
{
    EXPECT_FALSE(lookup("XDG_MUSIC_DIR=\"$HOME/Music\"\n", "XDG_DOCUMENTS_DIR", home).has_value());
}

TEST(UserDirs, relative_value_is_ignored)
{
    EXPECT_FALSE(lookup("XDG_DOCUMENTS_DIR=\"Docs\"\n", "XDG_DOCUMENTS_DIR", home).has_value());
}

TEST(UserDirs, traversing_value_is_ignored)
{
    EXPECT_FALSE(
        lookup("XDG_DOCUMENTS_DIR=\"../../../etc\"\n", "XDG_DOCUMENTS_DIR", home).has_value());
}

TEST(UserDirs, home_in_the_middle_is_not_expanded)
{
    // Absolute as written, so accepted as written; the marker is text.
    EXPECT_EQ(lookup("XDG_DOCUMENTS_DIR=\"/x/$HOME/y\"\n", "XDG_DOCUMENTS_DIR", home),
              "/x/$HOME/y");
}

TEST(UserDirs, home_then_traversal_is_accepted_as_given)
{
    EXPECT_EQ(lookup("XDG_DOCUMENTS_DIR=\"$HOME/../..\"\n", "XDG_DOCUMENTS_DIR", home),
              "/home/test/../..");
}

TEST(UserDirs, unclosed_quote_is_taken_literally)
{
    // The value starts with a quote, which makes it relative, so it is ignored.
    EXPECT_FALSE(lookup("XDG_DOCUMENTS_DIR=\"$HOME/Docs\n", "XDG_DOCUMENTS_DIR", home).has_value());
}

TEST(UserDirs, line_without_equals_is_skipped)
{
    EXPECT_EQ(lookup("garbage line\nXDG_MUSIC_DIR=\"$HOME/Music\"\n", "XDG_MUSIC_DIR", home),
              "/home/test/Music");
}

TEST(UserDirs, stops_after_max_lines)
{
    std::string content;
    for (std::size_t i = 0; i < max_lines + 1; ++i) {
        content += "# filler\n";
    }
    content += "XDG_DOCUMENTS_DIR=\"$HOME/Late\"\n";
    EXPECT_FALSE(lookup(content, "XDG_DOCUMENTS_DIR", home).has_value());
}

TEST(UserDirs, key_on_the_last_allowed_line_is_found)
{
    std::string content;
    for (std::size_t i = 0; i < max_lines - 1; ++i) {
        content += "# filler\n";
    }
    content += "XDG_DOCUMENTS_DIR=\"$HOME/JustInTime\"\n";
    EXPECT_EQ(lookup(content, "XDG_DOCUMENTS_DIR", home), "/home/test/JustInTime");
}

TEST(UserDirs, one_megabyte_single_line)
{
    const std::string content = "XDG_DOCUMENTS_DIR=" + std::string(1024 * 1024, 'a');
    EXPECT_FALSE(lookup(content, "XDG_DOCUMENTS_DIR", home).has_value());
}

TEST(UserDirs, last_line_without_newline_is_read)
{
    EXPECT_EQ(lookup("XDG_MUSIC_DIR=\"$HOME/Music\"", "XDG_MUSIC_DIR", home), "/home/test/Music");
}

TEST(UserDirs, empty_content_is_nullopt)
{
    EXPECT_FALSE(lookup("", "XDG_DOCUMENTS_DIR", home).has_value());
}

TEST(UserDirs, empty_key_is_nullopt)
{
    EXPECT_FALSE(lookup("=\"/x\"\n", "", home).has_value());
}

TEST(UserDirs, value_with_control_character_is_ignored)
{
    EXPECT_FALSE(
        lookup("XDG_DOCUMENTS_DIR=\"$HOME/D\x01ocs\"\n", "XDG_DOCUMENTS_DIR", home).has_value());
}

TEST(UserDirs, empty_value_is_ignored)
{
    EXPECT_FALSE(lookup("XDG_DOCUMENTS_DIR=\"\"\n", "XDG_DOCUMENTS_DIR", home).has_value());
    EXPECT_FALSE(lookup("XDG_DOCUMENTS_DIR=\n", "XDG_DOCUMENTS_DIR", home).has_value());
}

TEST(UserDirs, home_marker_must_be_the_whole_value_or_a_first_component)
{
    EXPECT_FALSE(
        lookup("XDG_DOCUMENTS_DIR=\"$HOMEBOY/x\"\n", "XDG_DOCUMENTS_DIR", home).has_value());
    EXPECT_EQ(lookup("XDG_DOCUMENTS_DIR=\"$HOME/x\"\n", "XDG_DOCUMENTS_DIR", home), "/home/test/x");
}

TEST(UserDirs, escaped_quote_inside_a_quoted_value)
{
    EXPECT_EQ(lookup("XDG_DOCUMENTS_DIR=\"$HOME/a\\\"b\"\n", "XDG_DOCUMENTS_DIR", home),
              "/home/test/a\"b");
}

TEST(UserDirs, comment_after_a_quoted_value_is_ignored)
{
    EXPECT_EQ(lookup("XDG_MUSIC_DIR=\"$HOME/Music\" # tunes\n", "XDG_MUSIC_DIR", home),
              "/home/test/Music");
}

TEST(UserDirs, junk_after_a_quoted_value_rejects_the_line)
{
    EXPECT_FALSE(lookup("XDG_MUSIC_DIR=\"$HOME/Music\" junk\n", "XDG_MUSIC_DIR", home).has_value());
}

TEST(UserDirs, unquoted_value_stops_at_blank_or_hash)
{
    EXPECT_EQ(lookup("XDG_MUSIC_DIR=/mnt/music # tunes\n", "XDG_MUSIC_DIR", home), "/mnt/music");
    EXPECT_EQ(lookup("XDG_MUSIC_DIR=/mnt/music extra\n", "XDG_MUSIC_DIR", home), "/mnt/music");
}

TEST(UserDirs, keys_are_case_sensitive_like_the_shell)
{
    EXPECT_FALSE(
        lookup("xdg_documents_dir=\"$HOME/Docs\"\n", "XDG_DOCUMENTS_DIR", home).has_value());
}

TEST(UserDirs, escaped_dollar_is_not_expanded)
{
    EXPECT_FALSE(
        lookup("XDG_DOCUMENTS_DIR=\"\\$HOME/x\"\n", "XDG_DOCUMENTS_DIR", home).has_value());
}

TEST(UserDirs, hash_inside_an_unquoted_value_is_part_of_it)
{
    EXPECT_EQ(lookup("XDG_MUSIC_DIR=/mnt/a#b\n", "XDG_MUSIC_DIR", home), "/mnt/a#b");
}

TEST(UserDirs, escaped_blank_in_an_unquoted_value_is_kept)
{
    EXPECT_EQ(lookup("XDG_MUSIC_DIR=/mnt/a\\ b\n", "XDG_MUSIC_DIR", home), "/mnt/a b");
}
