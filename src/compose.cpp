// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/compose.h>
#include <platformdirs/validation.h>

#include <algorithm>

namespace platformdirs::compose {

namespace {

void join(std::string& path, std::string_view component)
{
    if (!path.empty() && path.back() != '/') {
        path += '/';
    }
    path += component;
}

// Python reads appauthor with "or", so an empty explicit author falls
// back to the application name just like an unset one.
std::string_view author_segment(const Options& o)
{
    switch (o.author.mode()) {
    case Author::Mode::Disabled: return {};
    case Author::Mode::DefaultToAppName: return o.app_name;
    case Author::Mode::Explicit: return o.author.name().empty() ? o.app_name : o.author.name();
    }
    return {};
}

Result<std::string> finish(Platform p, std::string path)
{
    if (const Status s = validation::check_path_length(p, path); !s) {
        return std::unexpected(s.error());
    }
    return path;
}

} // namespace

std::string normalize_base(Platform p, std::string_view base)
{
    std::string folded(base);
    if (folded.empty()) {
        return folded;
    }
    if (p == Platform::Windows) {
        std::ranges::replace(folded, '\\', '/');
    }
    std::size_t last = folded.size();
    while (last > 1 && folded[last - 1] == '/') {
        --last;
    }
    folded.resize(last);
    // "C:/" must not become "C:", which would be drive relative.
    if (p == Platform::Windows && folded.size() == 2 && folded[1] == ':' && base.size() > 2) {
        folded += '/';
    }
    return folded;
}

Result<std::string> append_app_parts(Platform p, std::string_view base, const Options& o)
{
    std::string path = normalize_base(p, base);
    if (!o.app_name.empty()) {
        join(path, o.app_name);
        if (!o.version.empty()) {
            join(path, o.version);
        }
    }
    return finish(p, std::move(path));
}

Result<std::string> append_windows_parts(std::string_view base, const Options& o,
                                         std::string_view opinion_segment)
{
    std::string path = normalize_base(Platform::Windows, base);
    if (!o.app_name.empty()) {
        if (const std::string_view author = author_segment(o); !author.empty()) {
            join(path, author);
        }
        join(path, o.app_name);
        if (!opinion_segment.empty() && o.opinion) {
            join(path, opinion_segment);
        }
        if (!o.version.empty()) {
            join(path, o.version);
        }
    }
    return finish(Platform::Windows, std::move(path));
}

Result<std::string> append(Platform p, std::string_view base, std::string_view component)
{
    std::string path = normalize_base(p, base);
    join(path, component);
    return finish(p, std::move(path));
}

std::vector<std::string> split_path_list(Platform p, std::string_view list)
{
    std::vector<std::string> result;
    const char sep = list_separator(p);
    while (!list.empty()) {
        const std::size_t cut = list.find(sep);
        const std::string_view piece = validation::trim_whitespace(list.substr(0, cut));
        if (validation::is_usable_base(p, piece)) {
            result.push_back(normalize_base(p, piece));
        }
        if (cut == std::string_view::npos) {
            break;
        }
        list.remove_prefix(cut + 1);
    }
    return result;
}

std::string render(Platform p, std::string_view generic)
{
    std::string out(generic);
    if (p == Platform::Windows) {
        std::ranges::replace(out, '/', '\\');
    }
    return out;
}

} // namespace platformdirs::compose
