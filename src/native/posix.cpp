// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

// POSIX implementation, shared by Linux, the BSDs and macOS.

#include <platformdirs/sources.h>
#include <platformdirs/validation.h>

#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <vector>

namespace platformdirs {

namespace {

constexpr std::size_t passwd_buffer_start = 16384;
constexpr std::size_t passwd_buffer_limit = std::size_t{1} << 20;

class PosixEnvironment final : public EnvironmentSource {
public:
    std::optional<std::string> get(std::string_view name) const override
    {
        // getenv wants a terminated string; a string_view is not one.
        const std::string key(name);
        const char* value = std::getenv(key.c_str());
        if (value == nullptr) {
            return std::nullopt;
        }
        return std::string(value);
    }
};

// Closes the descriptor on every path out of read_text_file.
class Descriptor {
public:
    explicit Descriptor(int fd) : fd_(fd) {}
    ~Descriptor()
    {
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }
    Descriptor(const Descriptor&) = delete;
    Descriptor& operator=(const Descriptor&) = delete;
    int get() const { return fd_; }

private:
    int fd_;
};

class PosixSystem final : public SystemSource {
public:
    std::optional<std::string> home_directory() const override
    {
        // HOME counts only when it would be usable as a base; anything
        // else falls through to the passwd entry, as Python does.
        if (const char* home = std::getenv("HOME");
            home != nullptr && validation::is_usable_base(host_platform(), home)) {
            return std::string(home);
        }
        long hint = sysconf(_SC_GETPW_R_SIZE_MAX);
        std::size_t size = hint > 0 ? static_cast<std::size_t>(hint) : passwd_buffer_start;
        // Directory-backed passwd entries can exceed the hint; grow on ERANGE.
        while (size <= passwd_buffer_limit) {
            std::vector<char> buffer(size);
            passwd entry{};
            passwd* found = nullptr;
            const int rc = getpwuid_r(getuid(), &entry, buffer.data(), buffer.size(), &found);
            if (rc == ERANGE) {
                size *= 2;
                continue;
            }
            if (rc != 0 || found == nullptr || found->pw_dir == nullptr ||
                !validation::is_usable_base(host_platform(), found->pw_dir)) {
                return std::nullopt;
            }
            return std::string(found->pw_dir);
        }
        return std::nullopt;
    }

    std::optional<unsigned long> user_id() const override { return getuid(); }

    std::optional<std::string> known_folder(KnownFolder) const override { return std::nullopt; }

    bool is_writable_directory(std::string_view path) const override
    {
        const std::string p(path);
        struct stat st{};
        if (stat(p.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
            return false;
        }
        return access(p.c_str(), W_OK) == 0;
    }

    std::optional<std::string> read_text_file(std::string_view path,
                                              std::size_t max_bytes) const override
    {
        const std::string p(path);
        // Open before inspecting, non-blocking, so a FIFO put in place
        // between a check and the open cannot park the thread.
        const Descriptor fd(::open(p.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC));
        if (fd.get() < 0) {
            return std::nullopt;
        }
        struct stat st{};
        if (fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode)) {
            return std::nullopt;
        }
        std::string content;
        std::vector<char> chunk(65536);
        while (content.size() < max_bytes) {
            const std::size_t want = std::min(chunk.size(), max_bytes - content.size());
            const ssize_t n = ::read(fd.get(), chunk.data(), want);
            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }
                return std::nullopt;
            }
            if (n == 0) {
                break;
            }
            content.append(chunk.data(), static_cast<std::size_t>(n));
        }
        return content;
    }
};

} // namespace

const EnvironmentSource& native_environment()
{
    static const PosixEnvironment instance;
    return instance;
}

const SystemSource& native_system()
{
    static const PosixSystem instance;
    return instance;
}

} // namespace platformdirs
