# This file is part of the cpp-platformdirs Project.
# License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
#

cmake_minimum_required(VERSION 3.24)
project(cpp-platformdirs
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Platform directory resolution for C++ (platformdirs API)"
)

# Release metadata next to the version: a release bumps these three lines together.
set(CPP_PLATFORMDIRS_PART_NUMBER_BASE "SJ-PKG-0100")
set(CPP_PLATFORMDIRS_RELEASE_DATE "20260926")
set(CPP_PLATFORMDIRS_PART_NUMBER "${CPP_PLATFORMDIRS_PART_NUMBER_BASE}-${CPP_PLATFORMDIRS_RELEASE_DATE}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Before the target: the exported include directory uses CMAKE_INSTALL_INCLUDEDIR.
include(GNUInstallDirs)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

option(CPP_PLATFORMDIRS_BUILD_TESTS "Build unit tests" OFF)
option(CPP_PLATFORMDIRS_BUILD_EXAMPLES "Build example programs" OFF)

configure_file(cmake/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/platformdirs/version.h @ONLY)

# Sanitizer build types, as the house C++ style requires.
set(CMAKE_CXX_FLAGS_ASAN "-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer"
    CACHE STRING "Flags for ASan+UBSan builds")
set(CMAKE_EXE_LINKER_FLAGS_ASAN "-fsanitize=address,undefined"
    CACHE STRING "Linker flags for ASan+UBSan builds")
set(CMAKE_SHARED_LINKER_FLAGS_ASAN "-fsanitize=address,undefined"
    CACHE STRING "Shared linker flags for ASan+UBSan builds")
set(CMAKE_CXX_FLAGS_TSAN "-O1 -g -fsanitize=thread"
    CACHE STRING "Flags for TSan builds")
set(CMAKE_EXE_LINKER_FLAGS_TSAN "-fsanitize=thread"
    CACHE STRING "Linker flags for TSan builds")
set(CMAKE_SHARED_LINKER_FLAGS_TSAN "-fsanitize=thread"
    CACHE STRING "Shared linker flags for TSan builds")

set(PLATFORMDIRS_SOURCES
    src/platform.cpp
    src/utf8.cpp
    src/validation.cpp
    src/compose.cpp
    src/fixed_sources.cpp
    src/context.cpp
    src/user_dirs.cpp
    src/rules/common.cpp
    src/rules/unix.cpp
    src/rules/macos.cpp
    src/rules/windows.cpp
    src/dirs.cpp
)
# Native sources are the only platform-selected files in the tree.
if(WIN32)
    list(APPEND PLATFORMDIRS_SOURCES src/native/windows.cpp)
else()
    list(APPEND PLATFORMDIRS_SOURCES src/native/posix.cpp)
endif()

add_library(platformdirs ${PLATFORMDIRS_SOURCES})
add_library(platformdirs::platformdirs ALIAS platformdirs)

target_include_directories(platformdirs
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
    target_compile_options(platformdirs PRIVATE /W4 /permissive- /utf-8)
    target_compile_definitions(platformdirs PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    target_compile_options(platformdirs PRIVATE -Wall -Wextra -Wpedantic -Werror=return-type)
endif()
if(WIN32)
    target_link_libraries(platformdirs PRIVATE shell32 ole32 uuid)
endif()
set_target_properties(platformdirs PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

install(TARGETS platformdirs
    EXPORT cpp-platformdirsTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/platformdirs DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/platformdirs/version.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/platformdirs)
install(EXPORT cpp-platformdirsTargets
    NAMESPACE platformdirs::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpp-platformdirs
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpp-platformdirsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cpp-platformdirsConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpp-platformdirs
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/cpp-platformdirsConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/cpp-platformdirsConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cpp-platformdirsConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpp-platformdirs
)
# The .pc file finds the prefix relative to itself (relocatable, multiarch-safe);
# an absolute libdir or includedir, as Nix passes, is written as is.
file(RELATIVE_PATH PLATFORMDIRS_PC_TO_PREFIX
    "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig" "${CMAKE_INSTALL_PREFIX}")
if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
    set(PLATFORMDIRS_PC_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
else()
    set(PLATFORMDIRS_PC_LIBDIR "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
endif()
if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
    set(PLATFORMDIRS_PC_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
else()
    set(PLATFORMDIRS_PC_INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
endif()
configure_file(cmake/cpp-platformdirs.pc.in ${CMAKE_CURRENT_BINARY_DIR}/cpp-platformdirs.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cpp-platformdirs.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

if(CPP_PLATFORMDIRS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
if(CPP_PLATFORMDIRS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
