#!/usr/bin/env python3
# This file is part of the cpp-platformdirs Project.
# License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
#

"""Generate the oracle golden file from Python platformdirs.

Runs every scenario in scenarios.py through the Python library and
writes two tab-separated files next to the C++ tests:

    tests/data/scenarios.tsv     id, key, value  (the environment to replay)
    tests/data/corpus-golden.tsv id, property, expected value

Python's Windows and macOS classes run on Linux: the Windows one through
its WIN_PD_OVERRIDE_* hook, the macOS one because it only expands "~".
Values are written in generic form (every backslash replaced by a
slash), because Python on Linux joins Windows overrides with "/".

Run with the kista venv:
    ~/Projects/libs/kista/.venv/bin/python3 tests/oracle/generate_golden.py
"""

from __future__ import annotations

import os
import sys
import tempfile
from pathlib import Path

import platformdirs
from platformdirs.macos import MacOS
from platformdirs.unix import Unix
from platformdirs.windows import Windows

sys.path.insert(0, str(Path(__file__).parent))
from scenarios import SCENARIOS  # noqa: E402

EXPECTED_VERSION = "4.11.0"

SINGLE_PROPERTIES = [
    "user_data_dir", "user_config_dir", "user_cache_dir", "user_state_dir", "user_log_dir",
    "user_runtime_dir", "user_preference_dir", "user_bin_dir", "user_applications_dir",
    "user_fonts_dir", "user_documents_dir", "user_downloads_dir", "user_pictures_dir",
    "user_videos_dir", "user_music_dir", "user_desktop_dir", "user_projects_dir",
    "user_publicshare_dir", "user_templates_dir", "site_cache_dir", "site_state_dir",
    "site_log_dir", "site_runtime_dir", "site_bin_dir",
]

# Private lists on Unix and macOS; single strings on Windows.
LIST_PROPERTIES = {
    "site_data_dirs": ("_site_data_dirs", "site_data_dir"),
    "site_config_dirs": ("_site_config_dirs", "site_config_dir"),
    "site_applications_dirs": ("_site_applications_dirs", "site_applications_dir"),
}

# Documented divergences, never edited out of the golden file by hand.
SKIP = {
    # Python checks the real /run/user/<uid> for writability when the
    # variable is unset; the C++ side answers through its system source.
    ("linux", "user_runtime_dir", "no XDG_RUNTIME_DIR"),
}

ENV_KEYS = set(os.environ)


def generic(value: str) -> str:
    return value.replace("\\", "/")


def clear_env() -> None:
    for key in list(os.environ):
        if key.startswith(("XDG_", "WIN_PD_OVERRIDE_")) or key in {
            "HOME", "USERPROFILE", "PUBLIC", "LOCALAPPDATA", "APPDATA", "ALLUSERSPROFILE",
            "PROGRAMDATA", "TMPDIR",
        }:
            del os.environ[key]


def main() -> int:
    if platformdirs.__version__ != EXPECTED_VERSION:
        print(f"platformdirs {EXPECTED_VERSION} expected, found {platformdirs.__version__}",
              file=sys.stderr)
        return 1
    if "/opt/python" in sys.prefix:
        print("Homebrew prefix detected; macOS site dirs would differ", file=sys.stderr)
        return 1

    root = Path(__file__).resolve().parents[1] / "data"
    root.mkdir(parents=True, exist_ok=True)
    scenario_rows: list[tuple[str, str, str]] = []
    golden_rows: list[tuple[str, str, str]] = []
    saved_env = dict(os.environ)

    with tempfile.TemporaryDirectory() as tmp:
        for s in SCENARIOS:
            os.environ.clear()
            os.environ.update(saved_env)
            clear_env()

            sid, platform = s["id"], s["platform"]
            home = s["home"]
            os.environ["HOME"] = home
            scenario_rows.append((sid, "platform", platform))
            scenario_rows.append((sid, "home", generic(home)))

            config_dir = None
            for key, value in s["env"].items():
                if value == "@CONFIG@":
                    config_dir = Path(tmp) / sid
                    config_dir.mkdir(exist_ok=True)
                    value = str(config_dir)
                    # The C++ side uses a fixed path for the same file.
                    scenario_rows.append((sid, "env:" + key, "/cfg"))
                else:
                    scenario_rows.append((sid, "env:" + key, generic(value)))
                os.environ[key] = value
            for key, value in s["folders"].items():
                os.environ["WIN_PD_OVERRIDE_" + key] = value
                scenario_rows.append((sid, "folder:" + key, generic(value)))
            if s["userdirs"]:
                assert config_dir is not None, "userdirs needs env XDG_CONFIG_HOME=@CONFIG@"
                lines = [f'{k}="{v}"' for k, v in s["userdirs"].items()]
                (config_dir / "user-dirs.dirs").write_text("\n".join(lines) + "\n",
                                                           encoding="utf-8")
                for k, v in s["userdirs"].items():
                    scenario_rows.append((sid, "userdirs:" + k, v))
            for key, value in s["options"].items():
                scenario_rows.append((sid, "opt:" + key, str(value)))

            cls = {"linux": Unix, "macos": MacOS, "windows": Windows}[platform]
            d = cls(**s["options"])

            # Python saw the real temporary config dir; the C++ replay
            # sees the fixed name the scenario file records.
            def expected(value: str) -> str:
                if config_dir is not None:
                    value = value.replace(str(config_dir), "/cfg")
                return generic(value)

            for prop in SINGLE_PROPERTIES:
                if (platform == "linux" and prop == "user_runtime_dir"
                        and "XDG_RUNTIME_DIR" not in s["env"]):
                    continue
                golden_rows.append((sid, prop, expected(getattr(d, prop))))
            for name, (private, single) in LIST_PROPERTIES.items():
                if platform == "windows":
                    values = [getattr(d, single)]
                else:
                    values = list(getattr(d, private))
                golden_rows.append((sid, name, "|".join(expected(v) for v in values)))

    os.environ.clear()
    os.environ.update(saved_env)

    with open(root / "scenarios.tsv", "w", encoding="utf-8") as f:
        f.write("# generated by tests/oracle/generate_golden.py, do not edit\n")
        for row in scenario_rows:
            f.write("\t".join(row) + "\n")
    with open(root / "corpus-golden.tsv", "w", encoding="utf-8") as f:
        f.write(f"# generated by tests/oracle/generate_golden.py from platformdirs "
                f"{platformdirs.__version__}, do not edit\n")
        for row in golden_rows:
            f.write("\t".join(row) + "\n")
    print(f"{len(SCENARIOS)} scenarios, {len(golden_rows)} golden rows")
    return 0


if __name__ == "__main__":
    sys.exit(main())
