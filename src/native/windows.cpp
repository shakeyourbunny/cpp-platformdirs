// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

// Windows implementation: Known Folder API for the bases, the wide environment API so
// values outside the ANSI code page survive, everything handed out as UTF-8.

#include <platformdirs/sources.h>
#include <platformdirs/validation.h>

// Order matters: windows.h defines the macros the others need, and initguid.h makes
// knownfolders.h define the FOLDERID constants here instead of declaring them.
// clang-format off
#include <windows.h>
#include <initguid.h>
#include <knownfolders.h>
#include <shlobj.h>
// clang-format on

#include <algorithm>
#include <cwchar>
#include <fstream>
#include <string>

namespace platformdirs {

namespace {

std::optional<std::string> to_utf8(const wchar_t* wide, int wide_length)
{
    if (wide_length == 0) {
        return std::string();
    }
    const int needed = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, wide_length,
                                           nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return std::nullopt; // unpaired surrogate in the input
    }
    std::string out(static_cast<std::size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, wide_length, out.data(), needed,
                        nullptr, nullptr);
    return out;
}

std::optional<std::wstring> to_wide(std::string_view utf8)
{
    if (utf8.empty()) {
        return std::wstring();
    }
    const int needed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(),
                                           static_cast<int>(utf8.size()), nullptr, 0);
    if (needed <= 0) {
        return std::nullopt;
    }
    std::wstring out(static_cast<std::size_t>(needed), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()),
                        out.data(), needed);
    return out;
}

class WindowsEnvironment final : public EnvironmentSource {
public:
    std::optional<std::string> get(std::string_view name) const override
    {
        const auto wide_name = to_wide(name);
        if (!wide_name) {
            return std::nullopt;
        }
        // The variable can change between the size query and the read,
        // so the read is retried until the value fits the buffer.
        DWORD size = 256;
        std::wstring value(size, L'\0');
        for (int attempt = 0; attempt < 8; ++attempt) {
            SetLastError(0);
            const DWORD written = GetEnvironmentVariableW(wide_name->c_str(), value.data(), size);
            if (written == 0) {
                if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
                    return std::nullopt;
                }
                return std::string();
            }
            if (written < size) {
                return to_utf8(value.c_str(), static_cast<int>(written));
            }
            size = written + 1;
            value.assign(size, L'\0');
        }
        return std::nullopt;
    }
};

const KNOWNFOLDERID& folder_id(KnownFolder f)
{
    switch (f) {
    case KnownFolder::LocalAppData: return FOLDERID_LocalAppData;
    case KnownFolder::RoamingAppData: return FOLDERID_RoamingAppData;
    case KnownFolder::ProgramData: return FOLDERID_ProgramData;
    case KnownFolder::Profile: return FOLDERID_Profile;
    case KnownFolder::Documents: return FOLDERID_Documents;
    case KnownFolder::Downloads: return FOLDERID_Downloads;
    case KnownFolder::Pictures: return FOLDERID_Pictures;
    case KnownFolder::Videos: return FOLDERID_Videos;
    case KnownFolder::Music: return FOLDERID_Music;
    case KnownFolder::Desktop: return FOLDERID_Desktop;
    case KnownFolder::Programs: return FOLDERID_Programs;
    case KnownFolder::CommonPrograms: return FOLDERID_CommonPrograms;
    case KnownFolder::Fonts: return FOLDERID_Fonts;
    case KnownFolder::Public: return FOLDERID_Public;
    }
    return FOLDERID_LocalAppData;
}

bool usable(const std::optional<std::string>& value)
{
    return value && validation::is_usable_base(Platform::Windows, *value);
}

class WindowsSystem final : public SystemSource {
public:
    std::optional<std::string> home_directory() const override
    {
        // Each step counts only when usable as a base; a bad answer
        // falls through to the next one.
        if (auto profile = known_folder(KnownFolder::Profile); usable(profile)) {
            return profile;
        }
        const WindowsEnvironment env;
        if (auto p = env.get("USERPROFILE"); usable(p)) {
            return p;
        }
        const auto drive = env.get("HOMEDRIVE");
        const auto rest = env.get("HOMEPATH");
        if (drive && rest) {
            if (auto joined = std::optional<std::string>(*drive + *rest); usable(joined)) {
                return joined;
            }
        }
        return std::nullopt;
    }

    std::optional<unsigned long> user_id() const override { return std::nullopt; }

    // KF_FLAG_DONT_VERIFY as Python passes it: the path comes back even when the folder
    // does not exist yet. Nothing is downgraded to an 8.3 short name.
    std::optional<std::string> known_folder(KnownFolder f) const override
    {
        PWSTR buffer = nullptr;
        const HRESULT hr =
            SHGetKnownFolderPath(folder_id(f), KF_FLAG_DONT_VERIFY, nullptr, &buffer);
        if (FAILED(hr) || buffer == nullptr) {
            if (buffer != nullptr) {
                CoTaskMemFree(buffer);
            }
            return std::nullopt;
        }
        const auto result = to_utf8(buffer, static_cast<int>(std::wcslen(buffer)));
        CoTaskMemFree(buffer);
        return result;
    }

    bool is_writable_directory(std::string_view path) const override
    {
        const auto wide = to_wide(path);
        if (!wide) {
            return false;
        }
        const DWORD attributes = GetFileAttributesW(wide->c_str());
        return attributes != INVALID_FILE_ATTRIBUTES &&
               (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
               (attributes & FILE_ATTRIBUTE_READONLY) == 0;
    }

    std::optional<std::string> read_text_file(std::string_view path,
                                              std::size_t max_bytes) const override
    {
        const auto wide = to_wide(path);
        if (!wide) {
            return std::nullopt;
        }
        const DWORD attributes = GetFileAttributesW(wide->c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            return std::nullopt;
        }
        std::ifstream in(wide->c_str(), std::ios::binary);
        if (!in) {
            return std::nullopt;
        }
        std::string content;
        std::string chunk(65536, '\0');
        while (content.size() < max_bytes) {
            const std::size_t want = std::min(chunk.size(), max_bytes - content.size());
            in.read(chunk.data(), static_cast<std::streamsize>(want));
            const auto got = static_cast<std::size_t>(in.gcount());
            if (got == 0) {
                break;
            }
            content.append(chunk.data(), got);
        }
        return content;
    }
};

} // namespace

const EnvironmentSource& native_environment()
{
    static const WindowsEnvironment instance;
    return instance;
}

const SystemSource& native_system()
{
    static const WindowsSystem instance;
    return instance;
}

} // namespace platformdirs
