// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/compose.h>
#include <platformdirs/options.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace platformdirs;
using namespace platformdirs::compose;

namespace {

Options app(std::string name, std::string version = "", Author author = Author(),
            bool opinion = true)
{
    Options o;
    o.app_name = std::move(name);
    o.version = std::move(version);
    o.author = author;
    o.opinion = opinion;
    return o;
}

std::string value_of(const Result<std::string>& r)
{
    return r.has_value() ? *r : "<error: " + r.error().message + ">";
}

} // namespace

// Author

TEST(Author, default_falls_back_to_the_app_name)
{
    const Author a;
    EXPECT_EQ(a.mode(), Author::Mode::DefaultToAppName);
    EXPECT_EQ(Author::app_name_default().mode(), Author::Mode::DefaultToAppName);
}

TEST(Author, none_disables_the_segment)
{
    EXPECT_EQ(Author::none().mode(), Author::Mode::Disabled);
}

TEST(Author, named_keeps_the_name)
{
    const Author a = Author::named("Acme");
    EXPECT_EQ(a.mode(), Author::Mode::Explicit);
    EXPECT_EQ(a.name(), "Acme");
}

// normalize_base

TEST(Compose, normalize_strips_trailing_separators)
{
    EXPECT_EQ(normalize_base(Platform::Linux, "/base/"), "/base");
    EXPECT_EQ(normalize_base(Platform::Linux, "/base///"), "/base");
}

TEST(Compose, normalize_keeps_a_bare_root)
{
    EXPECT_EQ(normalize_base(Platform::Linux, "/"), "/");
    EXPECT_EQ(normalize_base(Platform::Linux, "///"), "/");
    EXPECT_EQ(normalize_base(Platform::Windows, "C:/"), "C:/");
    EXPECT_EQ(normalize_base(Platform::Windows, "C:\\"), "C:/");
}

TEST(Compose, normalize_folds_backslashes_for_windows_only)
{
    EXPECT_EQ(normalize_base(Platform::Windows, "C:\\Users\\x\\"), "C:/Users/x");
    EXPECT_EQ(normalize_base(Platform::Linux, "/a\\b"), "/a\\b");
}

TEST(Compose, normalize_empty_stays_empty)
{
    EXPECT_EQ(normalize_base(Platform::Linux, ""), "");
}

// append_app_parts

TEST(Compose, base_only)
{
    EXPECT_EQ(value_of(append_app_parts(Platform::Linux, "/base", app(""))), "/base");
}

TEST(Compose, base_and_name)
{
    EXPECT_EQ(value_of(append_app_parts(Platform::Linux, "/base", app("myapp"))), "/base/myapp");
}

TEST(Compose, base_name_and_version)
{
    EXPECT_EQ(value_of(append_app_parts(Platform::Linux, "/base", app("myapp", "1.0"))),
              "/base/myapp/1.0");
}

TEST(Compose, version_without_name_is_ignored)
{
    EXPECT_EQ(value_of(append_app_parts(Platform::Linux, "/base", app("", "1.2"))), "/base");
}

TEST(Compose, trailing_separator_does_not_double)
{
    EXPECT_EQ(value_of(append_app_parts(Platform::Linux, "/base/", app("myapp"))), "/base/myapp");
}

TEST(Compose, repeated_trailing_separators)
{
    EXPECT_EQ(value_of(append_app_parts(Platform::Linux, "/base///", app("myapp"))), "/base/myapp");
}

TEST(Compose, root_base)
{
    EXPECT_EQ(value_of(append_app_parts(Platform::Linux, "/", app("myapp"))), "/myapp");
    EXPECT_EQ(value_of(append_app_parts(Platform::Linux, "/", app(""))), "/");
}

TEST(Compose, windows_root_base_keeps_the_drive)
{
    EXPECT_EQ(value_of(append_app_parts(Platform::Windows, "C:/", app("myapp"))), "C:/myapp");
    EXPECT_EQ(value_of(append_app_parts(Platform::Windows, "C:\\", app(""))), "C:/");
}

TEST(Compose, append_refuses_overlong_result)
{
    const std::string base = "/" + std::string(max_path_bytes_posix - 4, 'a'); // 4093 bytes
    const auto r = append_app_parts(Platform::Linux, base, app("myapp"));
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, ErrorCode::PathTooLong);
}

TEST(Compose, append_accepts_result_at_the_limit)
{
    const std::string base = "/" + std::string(max_path_bytes_posix - 7, 'a'); // 4090 bytes
    const auto r = append_app_parts(Platform::Linux, base, app("myapp"));      // + "/myapp" = 4096
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->size(), max_path_bytes_posix);
}

// append_windows_parts

TEST(Compose, windows_author_defaults_to_name)
{
    EXPECT_EQ(value_of(append_windows_parts("C:/base", app("myapp"))), "C:/base/myapp/myapp");
}

TEST(Compose, windows_explicit_author)
{
    EXPECT_EQ(value_of(append_windows_parts("C:/base", app("myapp", "", Author::named("Acme")))),
              "C:/base/Acme/myapp");
}

TEST(Compose, windows_disabled_author)
{
    EXPECT_EQ(value_of(append_windows_parts("C:/base", app("myapp", "", Author::none()))),
              "C:/base/myapp");
}

TEST(Compose, windows_empty_author_falls_back_to_name)
{
    // Python reads appauthor with "or", so an empty author is an unset one.
    EXPECT_EQ(value_of(append_windows_parts("C:/base", app("myapp", "", Author::named("")))),
              "C:/base/myapp/myapp");
}

TEST(Compose, windows_opinion_segment)
{
    EXPECT_EQ(value_of(append_windows_parts("C:/base", app("myapp"), "Cache")),
              "C:/base/myapp/myapp/Cache");
}

TEST(Compose, windows_opinion_before_version)
{
    EXPECT_EQ(value_of(append_windows_parts("C:/base", app("myapp", "1.0"), "Cache")),
              "C:/base/myapp/myapp/Cache/1.0");
}

TEST(Compose, windows_opinion_off)
{
    EXPECT_EQ(
        value_of(append_windows_parts("C:/base", app("myapp", "1.0", Author(), false), "Cache")),
        "C:/base/myapp/myapp/1.0");
}

TEST(Compose, windows_no_name_appends_nothing)
{
    EXPECT_EQ(
        value_of(append_windows_parts("C:/base", app("", "1.0", Author::named("Acme")), "Cache")),
        "C:/base");
}

TEST(Compose, windows_backslash_base_is_folded)
{
    EXPECT_EQ(value_of(append_windows_parts("C:\\base\\", app("myapp"))), "C:/base/myapp/myapp");
}

TEST(Compose, windows_parts_refuse_overlong_result)
{
    const std::string base = "C:/" + std::string(max_path_units_windows - 6, 'a');
    const auto r = append_windows_parts(base, app("myapp"));
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, ErrorCode::PathTooLong);
}

// append

TEST(Compose, append_one_component)
{
    EXPECT_EQ(value_of(append(Platform::Linux, "/base", "log")), "/base/log");
    EXPECT_EQ(value_of(append(Platform::Linux, "/base/", "log")), "/base/log");
    EXPECT_EQ(value_of(append(Platform::Windows, "C:\\base", "Logs")), "C:/base/Logs");
}

// split_path_list

TEST(Compose, split_two_entries)
{
    const std::vector<std::string> expected = {"/a", "/b"};
    EXPECT_EQ(split_path_list(Platform::Linux, "/a:/b"), expected);
}

TEST(Compose, split_drops_empty_entries)
{
    const std::vector<std::string> expected = {"/a", "/b"};
    EXPECT_EQ(split_path_list(Platform::Linux, "/a::/b:"), expected);
}

TEST(Compose, split_drops_relative_entries)
{
    const std::vector<std::string> expected = {"/a", "/b"};
    EXPECT_EQ(split_path_list(Platform::Linux, "/a:../evil:/b"), expected);
}

TEST(Compose, split_normalizes_entries)
{
    const std::vector<std::string> expected = {"/a", "/b"};
    EXPECT_EQ(split_path_list(Platform::Linux, "/a/:/b//"), expected);
}

TEST(Compose, split_empty_input)
{
    EXPECT_TRUE(split_path_list(Platform::Linux, "").empty());
}

TEST(Compose, split_only_separators)
{
    EXPECT_TRUE(split_path_list(Platform::Linux, ":::").empty());
}

TEST(Compose, split_windows_uses_semicolon)
{
    const std::vector<std::string> expected = {"C:/a", "D:/b"};
    EXPECT_EQ(split_path_list(Platform::Windows, "C:\\a;D:\\b"), expected);
}

TEST(Compose, split_windows_ignores_colons_inside_drives)
{
    const std::vector<std::string> expected = {"C:/a"};
    EXPECT_EQ(split_path_list(Platform::Windows, "C:\\a"), expected);
}

TEST(Compose, split_bounded_on_ten_thousand_entries)
{
    std::string list;
    for (int i = 0; i < 10000; ++i) {
        list += "/a:";
    }
    EXPECT_EQ(split_path_list(Platform::Linux, list).size(), 10000u);
}

// render

TEST(Compose, render_windows_uses_backslashes)
{
    EXPECT_EQ(render(Platform::Windows, "C:/a/b"), "C:\\a\\b");
    EXPECT_EQ(render(Platform::Windows, "//server/share/x"), "\\\\server\\share\\x");
}

TEST(Compose, render_posix_leaves_slashes)
{
    EXPECT_EQ(render(Platform::Linux, "/a/b"), "/a/b");
    EXPECT_EQ(render(Platform::MacOS, "/a/b"), "/a/b");
}
