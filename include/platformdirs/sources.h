// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

#ifndef PLATFORMDIRS_SOURCES_H
#define PLATFORMDIRS_SOURCES_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// The two seams between the rules and the outside world; the rules ask these instead
// of touching the environment or a file, and tests swap in the doubles from testing.h.
namespace platformdirs {

class EnvironmentSource {
public:
    virtual ~EnvironmentSource() = default;
    // nullopt when the variable is unset. A variable that is set but
    // empty comes back as an empty string. Never throws.
    virtual std::optional<std::string> get(std::string_view name) const = 0;
};

// The Windows folders this library asks for. Names follow the
// KNOWNFOLDERID constants, not the legacy CSIDL spelling.
enum class KnownFolder : std::uint8_t {
    LocalAppData,
    RoamingAppData,
    ProgramData,
    Profile,
    Documents,
    Downloads,
    Pictures,
    Videos,
    Music,
    Desktop,
    Programs,
    CommonPrograms,
    Fonts,
    Public,
};

class SystemSource {
public:
    virtual ~SystemSource() = default;

    // nullopt when no home can be determined. POSIX: a usable HOME, else the passwd entry.
    // Windows: the Profile folder, USERPROFILE, then HOMEDRIVE with HOMEPATH.
    virtual std::optional<std::string> home_directory() const = 0;

    // Numeric real user id (getuid, as Python's os.getuid); nullopt on Windows.
    virtual std::optional<unsigned long> user_id() const = 0;

    // nullopt when the platform has no such concept or the lookup
    // failed. The rules fall back to environment variables.
    virtual std::optional<std::string> known_folder(KnownFolder f) const = 0;

    // For the runtime directory fallback: true when path is an existing
    // directory the process may write to.
    virtual bool is_writable_directory(std::string_view path) const = 0;

    // At most max_bytes of a regular file; nullopt when it does not
    // exist, is not a regular file, or cannot be read.
    virtual std::optional<std::string> read_text_file(std::string_view path,
                                                      std::size_t max_bytes) const = 0;
};

constexpr std::size_t max_config_file_bytes = std::size_t{1024} * 1024;

// The real thing for the platform this library was built on.
const EnvironmentSource& native_environment();
const SystemSource& native_system();

} // namespace platformdirs

#endif // PLATFORMDIRS_SOURCES_H
