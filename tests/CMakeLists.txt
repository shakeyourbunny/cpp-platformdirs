# This file is part of the cpp-platformdirs Project.
# License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
#

find_package(GTest 1.14 CONFIG QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.16.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(platformdirs_tests
    test_platform.cpp
    test_utf8.cpp
    test_validation.cpp
    test_compose.cpp
    test_sources.cpp
    test_native.cpp
    test_user_dirs.cpp
    test_rules_unix.cpp
    test_rules_macos.cpp
    test_rules_windows.cpp
    test_api.cpp
    test_security.cpp
    test_threads.cpp
    test_oracle.cpp
)
target_link_libraries(platformdirs_tests PRIVATE platformdirs::platformdirs GTest::gtest_main)
# Internal headers (utf8.h) are tested directly and are not installed.
target_include_directories(platformdirs_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)
if(MSVC)
    # The host tests call getenv; the CRT's deprecation note is noise here.
    target_compile_definitions(platformdirs_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(platformdirs_tests PRIVATE -Wall -Wextra -Wpedantic -Werror=return-type)
endif()
target_compile_definitions(platformdirs_tests PRIVATE
    PLATFORMDIRS_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

include(GoogleTest)
gtest_discover_tests(platformdirs_tests)
