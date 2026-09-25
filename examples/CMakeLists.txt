# This file is part of the cpp-platformdirs Project.
# License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
#

add_executable(show_dirs show_dirs.cpp)
target_link_libraries(show_dirs PRIVATE platformdirs::platformdirs)
if(NOT MSVC)
    target_compile_options(show_dirs PRIVATE -Wall -Wextra -Wpedantic -Werror=return-type)
endif()
