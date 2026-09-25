// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

// Replays the scenarios generate_golden.py ran through Python platformdirs. The golden
// file is never edited: a mismatch is a bug here or a divergence for the generator's skip list.

#include <platformdirs/platformdirs.h>
#include <platformdirs/testing.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>

using namespace platformdirs;

namespace {

struct Row {
    std::string id;
    std::string key;
    std::string value;
};

std::vector<Row> read_tsv(const std::string& name)
{
    std::ifstream in(std::string(PLATFORMDIRS_TEST_DATA_DIR) + "/" + name);
    std::vector<Row> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line.front() == '#') {
            continue;
        }
        const std::size_t a = line.find('\t');
        const std::size_t b = line.find('\t', a + 1);
        if (a == std::string::npos || b == std::string::npos) {
            continue;
        }
        rows.push_back({line.substr(0, a), line.substr(a + 1, b - a - 1), line.substr(b + 1)});
    }
    return rows;
}

std::string generic(const Path& p)
{
    std::string s = p.string();
    std::ranges::replace(s, '\\', '/');
    return s;
}

std::string generic_list(const PathList& list)
{
    std::string out;
    for (const Path& p : list) {
        if (!out.empty()) {
            out += '|';
        }
        out += generic(p);
    }
    return out;
}

const std::map<std::string, KnownFolder> folder_names = {
    {"LOCAL_APPDATA", KnownFolder::LocalAppData},
    {"APPDATA", KnownFolder::RoamingAppData},
    {"COMMON_APPDATA", KnownFolder::ProgramData},
    {"PERSONAL", KnownFolder::Documents},
    {"DOWNLOADS", KnownFolder::Downloads},
    {"MYPICTURES", KnownFolder::Pictures},
    {"MYVIDEO", KnownFolder::Videos},
    {"MYMUSIC", KnownFolder::Music},
    {"DESKTOPDIRECTORY", KnownFolder::Desktop},
    {"PROGRAMS", KnownFolder::Programs},
    {"COMMON_PROGRAMS", KnownFolder::CommonPrograms},
};

using Single = std::function<Result<Path>(const Dirs&)>;
using Many = std::function<Result<PathList>(const Dirs&)>;

const std::map<std::string, Single> singles = {
    {"user_data_dir", [](const Dirs& d) { return d.user_data_dir(); }},
    {"user_config_dir", [](const Dirs& d) { return d.user_config_dir(); }},
    {"user_cache_dir", [](const Dirs& d) { return d.user_cache_dir(); }},
    {"user_state_dir", [](const Dirs& d) { return d.user_state_dir(); }},
    {"user_log_dir", [](const Dirs& d) { return d.user_log_dir(); }},
    {"user_runtime_dir", [](const Dirs& d) { return d.user_runtime_dir(); }},
    {"user_preference_dir", [](const Dirs& d) { return d.user_preference_dir(); }},
    {"user_bin_dir", [](const Dirs& d) { return d.user_bin_dir(); }},
    {"user_applications_dir", [](const Dirs& d) { return d.user_applications_dir(); }},
    {"user_fonts_dir", [](const Dirs& d) { return d.user_fonts_dir(); }},
    {"user_documents_dir", [](const Dirs& d) { return d.user_documents_dir(); }},
    {"user_downloads_dir", [](const Dirs& d) { return d.user_downloads_dir(); }},
    {"user_pictures_dir", [](const Dirs& d) { return d.user_pictures_dir(); }},
    {"user_videos_dir", [](const Dirs& d) { return d.user_videos_dir(); }},
    {"user_music_dir", [](const Dirs& d) { return d.user_music_dir(); }},
    {"user_desktop_dir", [](const Dirs& d) { return d.user_desktop_dir(); }},
    {"user_projects_dir", [](const Dirs& d) { return d.user_projects_dir(); }},
    {"user_publicshare_dir", [](const Dirs& d) { return d.user_publicshare_dir(); }},
    {"user_templates_dir", [](const Dirs& d) { return d.user_templates_dir(); }},
    {"site_cache_dir", [](const Dirs& d) { return d.site_cache_dir(); }},
    {"site_state_dir", [](const Dirs& d) { return d.site_state_dir(); }},
    {"site_log_dir", [](const Dirs& d) { return d.site_log_dir(); }},
    {"site_runtime_dir", [](const Dirs& d) { return d.site_runtime_dir(); }},
    {"site_bin_dir", [](const Dirs& d) { return d.site_bin_dir(); }},
};

const std::map<std::string, Many> lists = {
    {"site_data_dirs", [](const Dirs& d) { return d.site_data_dirs(); }},
    {"site_config_dirs", [](const Dirs& d) { return d.site_config_dirs(); }},
    {"site_applications_dirs", [](const Dirs& d) { return d.site_applications_dirs(); }},
};

struct Scenario {
    Platform platform = Platform::Linux;
    FixedEnvironment env;
    FixedSystem sys;
    Options options;
    std::string config_home;
    std::vector<std::pair<std::string, std::string>> userdirs;
};

Platform platform_from(const std::string& name)
{
    if (name == "linux") {
        return Platform::Linux;
    }
    if (name == "macos") {
        return Platform::MacOS;
    }
    return Platform::Windows;
}

std::map<std::string, Scenario> load_scenarios()
{
    std::map<std::string, Scenario> out;
    for (const Row& r : read_tsv("scenarios.tsv")) {
        Scenario& s = out[r.id];
        if (r.key == "platform") {
            s.platform = platform_from(r.value);
            s.options.platform = s.platform;
            s.sys.set_user_id(s.platform == Platform::Windows ? std::nullopt
                                                              : std::optional<unsigned long>(1000));
        } else if (r.key == "home") {
            s.sys.set_home(r.value);
        } else if (r.key.starts_with("env:")) {
            s.env.set(r.key.substr(4), r.value);
            if (r.key == "env:XDG_CONFIG_HOME") {
                s.config_home = r.value;
            }
        } else if (r.key.starts_with("folder:")) {
            s.sys.set_folder(folder_names.at(r.key.substr(7)), r.value);
        } else if (r.key.starts_with("userdirs:")) {
            s.userdirs.emplace_back(r.key.substr(9), r.value);
        } else if (r.key == "opt:appname") {
            s.options.app_name = r.value;
        } else if (r.key == "opt:version") {
            s.options.version = r.value;
        } else if (r.key == "opt:appauthor") {
            s.options.author = r.value == "False" ? Author::none() : Author::named(r.value);
        } else if (r.key == "opt:roaming") {
            s.options.roaming = r.value == "True";
        } else if (r.key == "opt:opinion") {
            s.options.opinion = r.value == "True";
        }
    }
    for (auto& [id, s] : out) {
        if (!s.userdirs.empty()) {
            std::string content;
            for (const auto& [k, v] : s.userdirs) {
                content += k + "=\"" + v + "\"\n";
            }
            s.sys.add_file(s.config_home + "/user-dirs.dirs", content);
        }
    }
    return out;
}

} // namespace

TEST(Oracle, every_golden_row_matches_python_platformdirs)
{
    const auto scenarios = load_scenarios();
    ASSERT_FALSE(scenarios.empty()) << "scenarios.tsv missing or empty";
    const auto golden = read_tsv("corpus-golden.tsv");
    ASSERT_GT(golden.size(), 400u) << "corpus-golden.tsv missing or truncated";

    std::map<std::string, Dirs> dirs;
    for (const auto& [id, s] : scenarios) {
        const auto d = Dirs::create(s.options, s.env, s.sys);
        ASSERT_TRUE(d.has_value()) << id << ": " << d.error().message;
        dirs.emplace(id, *d);
    }

    for (const Row& row : golden) {
        const auto it = dirs.find(row.id);
        ASSERT_NE(it, dirs.end()) << "golden row for unknown scenario " << row.id;
        const Dirs& d = it->second;
        if (const auto single = singles.find(row.key); single != singles.end()) {
            const auto r = single->second(d);
            ASSERT_TRUE(r.has_value()) << row.id << " " << row.key << ": " << r.error().message;
            EXPECT_EQ(generic(*r), row.value) << row.id << " " << row.key;
        } else if (const auto list = lists.find(row.key); list != lists.end()) {
            const auto r = list->second(d);
            ASSERT_TRUE(r.has_value()) << row.id << " " << row.key << ": " << r.error().message;
            EXPECT_EQ(generic_list(*r), row.value) << row.id << " " << row.key;
        } else {
            FAIL() << "no accessor mapped for golden property " << row.key;
        }
    }
}

TEST(Oracle, every_public_accessor_is_covered_by_a_golden_row)
{
    const auto golden = read_tsv("corpus-golden.tsv");
    for (const auto& [name, fn] : singles) {
        const bool covered =
            std::ranges::any_of(golden, [&](const Row& r) { return r.key == name; });
        EXPECT_TRUE(covered) << name;
    }
    for (const auto& [name, fn] : lists) {
        const bool covered =
            std::ranges::any_of(golden, [&](const Row& r) { return r.key == name; });
        EXPECT_TRUE(covered) << name;
    }
}
