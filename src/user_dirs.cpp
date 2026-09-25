// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/user_dirs.h>
#include <platformdirs/validation.h>

#include <optional>
#include <string>

namespace platformdirs::user_dirs {

namespace {

constexpr std::string_view home_marker = "$HOME";

std::string_view trim(std::string_view s)
{
    const auto is_blank = [](char c) { return c == ' ' || c == '\t' || c == '\r'; };
    while (!s.empty() && is_blank(s.front())) {
        s.remove_prefix(1);
    }
    while (!s.empty() && is_blank(s.back())) {
        s.remove_suffix(1);
    }
    return s;
}

struct Parsed {
    std::string value;
    bool literal_start = false; // the first character was escaped, so "$HOME" is text
};

// The right-hand side as sh reads it: quoted up to the closing quote (backslash escapes),
// then only a comment may follow; unquoted up to the first blank, '#' being ordinary.
std::optional<Parsed> parse_value(std::string_view raw)
{
    raw = trim(raw);
    Parsed out;
    if (raw.empty()) {
        return out;
    }
    if (raw.front() != '"') {
        // Unquoted: a backslash escapes the next character, an unescaped blank ends it.
        for (std::size_t i = 0; i < raw.size(); ++i) {
            if (raw[i] == '\\' && i + 1 < raw.size()) {
                if (out.value.empty()) {
                    out.literal_start = true;
                }
                out.value += raw[++i];
            } else if (raw[i] == ' ' || raw[i] == '\t') {
                break;
            } else {
                out.value += raw[i];
            }
        }
        return out;
    }
    std::size_t i = 1;
    bool closed = false;
    while (i < raw.size()) {
        const char c = raw[i];
        if (c == '\\' && i + 1 < raw.size()) {
            if (out.value.empty()) {
                out.literal_start = true;
            }
            out.value += raw[i + 1];
            i += 2;
            continue;
        }
        if (c == '"') {
            closed = true;
            break;
        }
        out.value += c;
        ++i;
    }
    if (!closed) {
        return std::nullopt;
    }
    const std::string_view rest = trim(raw.substr(i + 1));
    if (!rest.empty() && rest.front() != '#') {
        return std::nullopt;
    }
    return out;
}

// $HOME expands only as the whole value or as the first component;
// $HOMEBOY is a different variable and \$HOME is text.
std::string expand_home(const Parsed& parsed, std::string_view home)
{
    const std::string& value = parsed.value;
    if (parsed.literal_start) {
        return value;
    }
    if (value == home_marker) {
        return std::string(home);
    }
    if (value.starts_with(home_marker) && value[home_marker.size()] == '/') {
        return std::string(home) + value.substr(home_marker.size());
    }
    return value;
}

// The usable value of one line for key, or nullopt. Keys are
// case-sensitive, as they are to the shell that sources this file.
std::optional<std::string> evaluate(std::string_view line, std::string_view key,
                                    std::string_view home)
{
    const std::string_view text = trim(line);
    if (text.empty() || text.front() == '#') {
        return std::nullopt;
    }
    const std::size_t eq = text.find('=');
    if (eq == std::string_view::npos || trim(text.substr(0, eq)) != key) {
        return std::nullopt;
    }
    const auto parsed = parse_value(text.substr(eq + 1));
    if (!parsed) {
        return std::nullopt;
    }
    std::string expanded = expand_home(*parsed, home);
    // A relative entry has the same problem as a relative XDG variable
    // and is ignored the same way.
    if (!validation::is_usable_base(Platform::Linux, expanded)) {
        return std::nullopt;
    }
    return expanded;
}

} // namespace

std::optional<std::string> lookup(std::string_view content, std::string_view key,
                                  std::string_view home)
{
    if (content.empty() || key.empty()) {
        return std::nullopt;
    }
    std::optional<std::string> found;
    std::size_t lines_seen = 0;
    std::size_t pos = 0;
    while (lines_seen < max_lines) {
        const std::size_t nl = content.find('\n', pos);
        const std::string_view line =
            nl == std::string_view::npos ? content.substr(pos) : content.substr(pos, nl - pos);
        ++lines_seen;
        if (auto value = evaluate(line, key, home)) {
            found = std::move(value);
        }
        if (nl == std::string_view::npos) {
            break;
        }
        pos = nl + 1;
    }
    return found;
}

} // namespace platformdirs::user_dirs
