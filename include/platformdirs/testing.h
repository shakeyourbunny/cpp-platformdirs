// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#ifndef PLATFORMDIRS_TESTING_H
#define PLATFORMDIRS_TESTING_H

#include <platformdirs/sources.h>

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>

// In-memory stand-ins for the environment and the system, so every platform's rules can
// be tested on one host without touching real configuration. Shipped for callers' tests.
namespace platformdirs {

class FixedEnvironment final : public EnvironmentSource {
public:
    void set(std::string name, std::string value);
    void unset(std::string_view name);
    void clear();

    std::optional<std::string> get(std::string_view name) const override;

private:
    std::map<std::string, std::string, std::less<>> vars_;
};

class FixedSystem final : public SystemSource {
public:
    // Fresh state: no home, uid 1000, no folders, no files.
    void clear();
    void set_home(std::optional<std::string> home);
    void set_user_id(std::optional<unsigned long> uid);
    void set_folder(KnownFolder f, std::optional<std::string> path);
    void add_file(std::string path, std::string content);
    void add_writable_directory(std::string path);

    std::optional<std::string> home_directory() const override;
    std::optional<unsigned long> user_id() const override;
    std::optional<std::string> known_folder(KnownFolder f) const override;
    bool is_writable_directory(std::string_view path) const override;
    std::optional<std::string> read_text_file(std::string_view path,
                                              std::size_t max_bytes) const override;

private:
    std::optional<std::string> home_;
    std::optional<unsigned long> uid_ = 1000;
    std::map<KnownFolder, std::string> folders_;
    std::map<std::string, std::string, std::less<>> files_;
    std::set<std::string, std::less<>> writable_dirs_;
};

} // namespace platformdirs

#endif // PLATFORMDIRS_TESTING_H
