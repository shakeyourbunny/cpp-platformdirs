# cpp-platformdirs

Where should your program put its config file? Its cache? Its logs?
The answer is different on every operating system, and getting it
wrong means littering someone's home directory. cpp-platformdirs works
it out for you, the same way Python's
[platformdirs](https://github.com/tox-dev/platformdirs) does, as a
C++23 library with no dependencies beyond the standard library and
the operating system.

```cpp
#include <platformdirs/platformdirs.h>

int main()
{
    platformdirs::Options options;
    options.app_name = "myapp";
    options.version = "1.0";

    const auto dirs = platformdirs::Dirs::create(options);
    if (!dirs) {
        std::cerr << dirs.error().message << '\n';
        return 1;
    }
    const auto config = dirs->user_config_dir();
    if (config) {
        // Linux:   /home/you/.config/myapp/1.0
        // macOS:   /Users/you/Library/Application Support/myapp/1.0
        // Windows: C:\Users\you\AppData\Local\myapp\myapp\1.0
        std::cout << config->string() << '\n';
    }
}
```

If you would rather not hold an object, every common directory is a
free function too:

```cpp
const auto cache = platformdirs::user_cache_dir("myapp", "1.0");
```

## What you get

Data, config, cache, state, log and runtime directories for the user,
the same set again for the whole machine, the user's bin and
applications directories, the preference directory, and the media
directories: documents, downloads, pictures, videos, music, desktop,
projects, public share, templates. Every directory comes back as a
`std::filesystem::path` inside a `std::expected`, so a missing home
directory or an unusable name is a value you handle, not an exception
you catch.

On Linux and the BSDs the library follows the XDG Base Directory
Specification, including the search paths and the localised folder
names from `user-dirs.dirs`. On macOS it follows Apple's layout under
`~/Library`, with the XDG variables taking precedence when they are
set. On Windows it asks the Known Folder API and falls back to the
classic environment variables when that fails.

One Windows note: read the text of a returned path with `native()` or
`u8string()`. `std::filesystem::path::string()` converts to the ANSI
code page there and throws for a character it cannot represent, which
a user name in the wrong script will trigger.

Search paths are vectors. `site_data_dirs()` returns every entry and
`site_data_dir()` the first one, which is what a caller writing a file
wants. There is no colon-joined string anywhere in the API, because a
joined string is not a path.

Fonts are a small extension over the Python API: `user_fonts_dir()`,
`site_fonts_dirs()` and `iter_fonts_dirs()` give the directories a
font loader should search, in order, on each platform.

| | Linux | macOS | Windows |
|---|---|---|---|
| `user_config_dir()` | `~/.config/myapp` | `~/Library/Application Support/myapp` | `%LOCALAPPDATA%\myapp\myapp` |
| `user_cache_dir()` | `~/.cache/myapp` | `~/Library/Caches/myapp` | `%LOCALAPPDATA%\myapp\myapp\Cache` |
| `user_log_dir()` | `~/.local/state/myapp/log` | `~/Library/Logs/myapp` | `%LOCALAPPDATA%\myapp\myapp\Logs` |
| `user_runtime_dir()` | `$XDG_RUNTIME_DIR/myapp` | `~/Library/Caches/TemporaryItems/myapp` | `%LOCALAPPDATA%\Temp\myapp\myapp` |
| `site_config_dir()` | `/etc/xdg/myapp` | `/Library/Application Support/myapp` | `%PROGRAMDATA%\myapp\myapp` |
| `user_fonts_dir()` | `~/.local/share/fonts` | `~/Library/Fonts` | `%LOCALAPPDATA%\Microsoft\Windows\Fonts` |

The `examples/show_dirs` program prints the whole set for any
application name, and with `--platform` it shows what another
platform's rules would answer.

## Being careful with names

The application name, author and version become directory names, so
they are checked before anything is built. A name holding a separator,
a `..`, a NUL or control character, a character Windows reserves, a
Windows device name such as `NUL` or `COM1` (with or without an
extension), a leading or trailing space, a trailing dot, malformed
UTF-8, an invisible or direction-changing code point, or more than 255
bytes is refused with an error naming the problem. Nothing is quietly
cleaned up: the name was written in your source code, and that is
where it should be fixed. The rules apply on every platform, so a name
that works on Linux cannot turn into an escape when the same program
runs on Windows.

Directory values that arrive from the environment get the same care.
The XDG specification says a relative path in one of its variables is
invalid and must be ignored; this library does that, and applies the
same rule to `HOME`, to every entry of a search path, and to every
value read from `user-dirs.dirs`. A blank value, a value with a
control character in it, or a value longer than the platform allows is
treated as unset. A resolved path that would exceed the platform's
length limit is an error, never a truncated string.

## What it does not do

It never creates a directory and never sets a permission. Creating
the directory with the right mode, and doing it without following a
link someone planted first, is your program's job, next to the code
that knows what it is about to store. The one file the library reads,
`user-dirs.dirs`, is opened like any other file, links included.
The same goes for the window between resolving a path and using it: a
library that returns a path cannot close it, and this one does not
pretend to.

There is no Android support, no sandbox awareness (Flatpak, Snap), and
no Homebrew relocation on macOS.

## Differences from Python platformdirs

The names, the option semantics and the rules follow platformdirs
4.11, so programs in both languages agree on where files go. The
deliberate differences:

- Names and environment values are validated as described above.
  Python passes them through.
- Relative XDG values are ignored per the specification. Python
  honours them, which makes the result depend on the working
  directory.
- `~` is not expanded in environment values. An XDG value must be
  absolute.
- `multipath` does not exist; the plural accessors return vectors.
- `ensure_exists` does not exist; nothing is created.
- `user-dirs.dirs` is read the way the shell reads it: the last
  assignment to a key wins, keys are case-sensitive, a backslash
  escapes the next character, an unquoted value ends at a blank and a
  quoted one at the closing quote. Python feeds the file to an INI
  parser instead.
- Whitespace around a single environment value is trimmed, as Python
  does; entries of a search path are trimmed too.
- The Windows path is UTF-8 throughout; nothing is shortened to an 8.3
  name.

## Building

CMake 3.24 or newer and a C++23 compiler (GCC 14, Clang 19, MSVC 2022
or Apple Clang from Xcode 15).

```
cmake --preset release
cmake --build --preset release
cmake --install build/release --prefix /usr/local
```

Then in your project:

```cmake
find_package(cpp-platformdirs CONFIG REQUIRED)
target_link_libraries(yourapp PRIVATE platformdirs::platformdirs)
```

A `pkg-config` file is installed as well. The library can also be
added with `add_subdirectory` or `FetchContent`.

## Testing

```
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

The suite runs the rules for all five platforms on whatever machine
you are on, because the rules read the environment through an
interface that the tests replace with an in-memory stand-in (shipped
in `<platformdirs/testing.h>`, so you can do the same for your own
code). No test touches your real configuration. A second layer replays
twenty scenarios through Python platformdirs and compares every
answer, so the two libraries cannot drift apart unnoticed. Presets
`asan` and `tsan` build the suite under the sanitizers.

The Windows and macOS rules are covered by those tests on every host;
the native sources for Windows have been built and run on Windows, the
macOS ones have not yet run on a Mac.

## License

GPL-3.0-or-later. See LICENSE for the full text and
THIRD_PARTY_LICENSES.txt for the test dependency and the references
the rules follow.

---
cpp-platformdirs has been written with the help of tooled assistance, but has been
designed, reviewed and thoroughly tested and signed off by its developer.
