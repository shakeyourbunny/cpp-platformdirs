// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#include <platformdirs/testing.h>

#include <utility>

namespace platformdirs {

void FixedEnvironment::set(std::string name, std::string value)
{
    vars_[std::move(name)] = std::move(value);
}

void FixedEnvironment::unset(std::string_view name)
{
    if (const auto it = vars_.find(name); it != vars_.end()) {
        vars_.erase(it);
    }
}

void FixedEnvironment::clear()
{
    vars_.clear();
}

std::optional<std::string> FixedEnvironment::get(std::string_view name) const
{
    const auto it = vars_.find(name);
    if (it == vars_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void FixedSystem::clear()
{
    home_.reset();
    uid_ = 1000;
    folders_.clear();
    files_.clear();
    writable_dirs_.clear();
}

void FixedSystem::set_home(std::optional<std::string> home)
{
    home_ = std::move(home);
}

void FixedSystem::set_user_id(std::optional<unsigned long> uid)
{
    uid_ = uid;
}

void FixedSystem::set_folder(KnownFolder f, std::optional<std::string> path)
{
    if (path) {
        folders_[f] = std::move(*path);
    } else {
        folders_.erase(f);
    }
}

void FixedSystem::add_file(std::string path, std::string content)
{
    files_[std::move(path)] = std::move(content);
}

void FixedSystem::add_writable_directory(std::string path)
{
    writable_dirs_.insert(std::move(path));
}

std::optional<std::string> FixedSystem::home_directory() const
{
    return home_;
}

std::optional<unsigned long> FixedSystem::user_id() const
{
    return uid_;
}

std::optional<std::string> FixedSystem::known_folder(KnownFolder f) const
{
    const auto it = folders_.find(f);
    if (it == folders_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool FixedSystem::is_writable_directory(std::string_view path) const
{
    return writable_dirs_.contains(path);
}

std::optional<std::string> FixedSystem::read_text_file(std::string_view path,
                                                       std::size_t max_bytes) const
{
    const auto it = files_.find(path);
    if (it == files_.end()) {
        return std::nullopt;
    }
    return it->second.substr(0, max_bytes);
}

} // namespace platformdirs
