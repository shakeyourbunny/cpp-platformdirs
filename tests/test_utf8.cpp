// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include "utf8.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

using namespace platformdirs::utf8;
using namespace std::literals;

TEST(Utf8, ascii_decodes_to_one_byte)
{
    const auto d = decode_one("a");
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(d->code_point, U'a');
    EXPECT_EQ(d->length, 1u);
}

TEST(Utf8, two_byte_sequence)
{
    const auto d = decode_one("\xC3\xA9");
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(d->code_point, U'é');
    EXPECT_EQ(d->length, 2u);
}

TEST(Utf8, three_byte_sequence)
{
    const auto d = decode_one("\xE2\x82\xAC");
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(d->code_point, U'€');
    EXPECT_EQ(d->length, 3u);
}

TEST(Utf8, four_byte_sequence)
{
    const auto d = decode_one("\xF0\x9F\x98\x80");
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(d->code_point, U'\U0001F600');
    EXPECT_EQ(d->length, 4u);
}

TEST(Utf8, decode_only_consumes_the_first_code_point)
{
    const auto d = decode_one("\xC3\xA9tat");
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(d->length, 2u);
}

TEST(Utf8, empty_input_is_nullopt)
{
    EXPECT_FALSE(decode_one("").has_value());
}

TEST(Utf8, overlong_slash_is_malformed)
{
    EXPECT_FALSE(decode_one("\xC0\xAF").has_value());
}

TEST(Utf8, overlong_three_byte_is_malformed)
{
    EXPECT_FALSE(decode_one("\xE0\x80\xAF").has_value());
}

TEST(Utf8, overlong_four_byte_is_malformed)
{
    EXPECT_FALSE(decode_one("\xF0\x80\x80\xAF").has_value());
}

TEST(Utf8, surrogate_is_malformed)
{
    EXPECT_FALSE(decode_one("\xED\xA0\x80").has_value());
    EXPECT_FALSE(decode_one("\xED\xBF\xBF").has_value());
}

TEST(Utf8, above_u10ffff_is_malformed)
{
    EXPECT_FALSE(decode_one("\xF4\x90\x80\x80").has_value());
    EXPECT_FALSE(decode_one("\xF5\x80\x80\x80").has_value());
}

TEST(Utf8, stray_continuation_is_malformed)
{
    EXPECT_FALSE(decode_one("\x80").has_value());
    EXPECT_FALSE(decode_one("\xBF").has_value());
}

TEST(Utf8, truncated_sequence_is_malformed)
{
    EXPECT_FALSE(decode_one("\xE2\x82").has_value());
    EXPECT_FALSE(decode_one("\xF0\x9F\x98").has_value());
    EXPECT_FALSE(decode_one("\xC3").has_value());
}

TEST(Utf8, continuation_byte_out_of_range_is_malformed)
{
    EXPECT_FALSE(decode_one("\xC3\x41").has_value());
    EXPECT_FALSE(decode_one("\xE2\x82\xC0").has_value());
}

TEST(Utf8, is_valid_accepts_mixed_text_and_rejects_a_bad_byte)
{
    EXPECT_TRUE(is_valid("Gr\xC3\xBC\xC3\x9F"
                         "e \xF0\x9F\x98\x80"));
    EXPECT_TRUE(is_valid(""));
    EXPECT_FALSE(is_valid("ok\xC0\xAF"
                          "bad"));
}

TEST(Utf8, nul_is_valid_utf8)
{
    EXPECT_TRUE(is_valid("a\0b"s));
}

TEST(Utf8, utf16_length_counts_surrogate_pairs)
{
    EXPECT_EQ(utf16_length("a\xF0\x9F\x98\x80"), 3u);
    EXPECT_EQ(utf16_length("\xE2\x82\xAC"), 1u);
    EXPECT_EQ(utf16_length(""), 0u);
}

TEST(Utf8, utf16_length_is_nullopt_for_malformed)
{
    EXPECT_FALSE(utf16_length("a\xED\xA0\x80").has_value());
}
