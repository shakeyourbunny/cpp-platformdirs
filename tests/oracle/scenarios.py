# This file is part of the cpp-platformdirs Project.
# License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
#

"""The scenario matrix for the Python platformdirs oracle.

Each scenario is one process environment plus one set of constructor
options. generate_golden.py runs Python platformdirs over it and the
C++ test replays it through the in-memory sources. Keep every value
absolute: relative values are the documented divergence and would only
produce rows for the skip list.
"""

WIN_LOCAL = "C:\\Users\\t\\AppData\\Local"
WIN_ROAMING = "C:\\Users\\t\\AppData\\Roaming"
WIN_PROGRAMDATA = "C:\\ProgramData"
WIN_PROFILE = "C:\\Users\\t"

WINDOWS_FOLDERS = {
    "LOCAL_APPDATA": WIN_LOCAL,
    "APPDATA": WIN_ROAMING,
    "COMMON_APPDATA": WIN_PROGRAMDATA,
    "PERSONAL": WIN_PROFILE + "\\Documents",
    "DOWNLOADS": WIN_PROFILE + "\\Downloads",
    "MYPICTURES": WIN_PROFILE + "\\Pictures",
    "MYVIDEO": WIN_PROFILE + "\\Videos",
    "MYMUSIC": WIN_PROFILE + "\\Music",
    "DESKTOPDIRECTORY": WIN_PROFILE + "\\Desktop",
    "PROGRAMS": WIN_ROAMING + "\\Microsoft\\Windows\\Start Menu\\Programs",
    "COMMON_PROGRAMS": WIN_PROGRAMDATA + "\\Microsoft\\Windows\\Start Menu\\Programs",
}

WINDOWS_ENV = {"USERPROFILE": WIN_PROFILE, "PUBLIC": "C:\\Users\\Public"}

XDG_ALL = {
    "XDG_DATA_HOME": "/d",
    "XDG_CONFIG_HOME": "/c",
    "XDG_CACHE_HOME": "/ca",
    "XDG_STATE_HOME": "/s",
    "XDG_RUNTIME_DIR": "/rt",
    "XDG_DATA_DIRS": "/x:/y:/z",
    "XDG_CONFIG_DIRS": "/c1:/c2",
    "XDG_DOCUMENTS_DIR": "/media/docs",
    "XDG_DOWNLOAD_DIR": "/media/dl",
    "XDG_PICTURES_DIR": "/media/pics",
    "XDG_VIDEOS_DIR": "/media/vids",
    "XDG_MUSIC_DIR": "/media/music",
    "XDG_DESKTOP_DIR": "/media/desk",
    "XDG_PROJECTS_DIR": "/media/proj",
    "XDG_PUBLICSHARE_DIR": "/media/pub",
    "XDG_TEMPLATES_DIR": "/media/tpl",
}

USER_DIRS_FILE = {
    "XDG_DOCUMENTS_DIR": "$HOME/Docs",
    "XDG_MUSIC_DIR": "/mnt/music",
    "XDG_DESKTOP_DIR": "$HOME",
}


def _s(id_, platform, home, env=None, folders=None, userdirs=None, **options):
    return {
        "id": id_,
        "platform": platform,
        "home": home,
        "env": env or {},
        "folders": folders or {},
        "userdirs": userdirs or {},
        "options": options,
    }


SCENARIOS = [
    # Linux
    _s("linux-default", "linux", "/home/t", appname="myapp"),
    _s("linux-version", "linux", "/home/t", appname="myapp", version="1.0"),
    _s("linux-noname", "linux", "/home/t"),
    _s("linux-opinion-off", "linux", "/home/t", appname="myapp", opinion=False),
    _s("linux-xdg", "linux", "/home/t", env=XDG_ALL, appname="myapp", version="2"),
    _s("linux-xdg-noname", "linux", "/home/t", env=XDG_ALL),
    _s("linux-userdirs", "linux", "/home/t", env={"XDG_CONFIG_HOME": "@CONFIG@"},
       userdirs=USER_DIRS_FILE, appname="myapp"),
    _s("linux-runtime-env", "linux", "/home/t", env={"XDG_RUNTIME_DIR": "/rt"}, appname="myapp"),
    # macOS
    _s("macos-default", "macos", "/Users/t", appname="myapp"),
    _s("macos-version", "macos", "/Users/t", appname="myapp", version="1.0"),
    _s("macos-noname", "macos", "/Users/t"),
    _s("macos-opinion-off", "macos", "/Users/t", appname="myapp", opinion=False),
    _s("macos-xdg", "macos", "/Users/t", env=XDG_ALL, appname="myapp", version="2"),
    # Windows
    _s("windows-default", "windows", WIN_PROFILE, env=WINDOWS_ENV, folders=WINDOWS_FOLDERS,
       appname="myapp"),
    _s("windows-version", "windows", WIN_PROFILE, env=WINDOWS_ENV, folders=WINDOWS_FOLDERS,
       appname="myapp", version="1.0"),
    _s("windows-author", "windows", WIN_PROFILE, env=WINDOWS_ENV, folders=WINDOWS_FOLDERS,
       appname="myapp", appauthor="Acme", version="1.0"),
    _s("windows-noauthor", "windows", WIN_PROFILE, env=WINDOWS_ENV, folders=WINDOWS_FOLDERS,
       appname="myapp", appauthor=False),
    _s("windows-roaming", "windows", WIN_PROFILE, env=WINDOWS_ENV, folders=WINDOWS_FOLDERS,
       appname="myapp", roaming=True, version="3"),
    _s("windows-opinion-off", "windows", WIN_PROFILE, env=WINDOWS_ENV, folders=WINDOWS_FOLDERS,
       appname="myapp", opinion=False, version="1.0"),
    _s("windows-noname", "windows", WIN_PROFILE, env=WINDOWS_ENV, folders=WINDOWS_FOLDERS),
]
