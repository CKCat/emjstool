cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0135 NEW)
project(jstool LANGUAGES C CXX RC)

# Compiler specific options for MSVC
if(MSVC)
    # Enable /utf8 for MSVC to prevent warning C4819 (data loss on code page 936)
    add_compile_options(/utf-8)
endif()


# EmEditor Plugin requires building a DLL
add_library(jstool SHARED
    src/jstool.cpp
    src/jstool.h
    src/mimetools.cpp
    src/mimetools.h
    src/json_utils.cpp
    src/json_utils.h
    src/hash_utils.cpp
    src/hash_utils.h
    src/time_utils.cpp
    src/time_utils.h
    src/tinflate.c
    src/include/etlframe.h
    src/nlohmann/json.hpp
    src/include/plugin.h
    src/res/resource.h
    src/res/Resource.rc
    src/jstool.def
)

target_include_directories(jstool PRIVATE src src/include src/res src/nlohmann)

# --- Testing ---
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# For Windows: prevent gtest from overriding the compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(mimetools_test 
    tests/mimetools_test.cpp 
    src/mimetools.cpp 
    src/tinflate.c
    src/hash_utils.cpp
    src/time_utils.cpp
)
target_include_directories(mimetools_test PRIVATE src)
# Define dummy HWND/BOOL etc for mimetools.cpp in non-windows test environment if needed, 
# but here we are on Windows so we just link user32
target_link_libraries(mimetools_test GTest::gtest_main user32 bcrypt)

add_executable(htmltools_test 
    tests/htmltools_test.cpp 
    src/mimetools.cpp 
    src/tinflate.c
    src/hash_utils.cpp
    src/time_utils.cpp
)
target_include_directories(htmltools_test PRIVATE src)
target_link_libraries(htmltools_test GTest::gtest_main user32 bcrypt)

add_executable(json_robustness_test tests/json_robustness_test.cpp src/json_utils.cpp)
target_include_directories(json_robustness_test PRIVATE src)
target_link_libraries(json_robustness_test GTest::gtest_main)

add_executable(hashtools_test tests/hashtools_test.cpp src/hash_utils.cpp)
target_include_directories(hashtools_test PRIVATE src)
target_link_libraries(hashtools_test GTest::gtest_main bcrypt)

add_executable(timetools_test tests/timetools_test.cpp src/time_utils.cpp)
target_include_directories(timetools_test PRIVATE src)
target_link_libraries(timetools_test GTest::gtest_main)

add_test(NAME MimeToolsTest COMMAND mimetools_test)
add_test(NAME HTMLToolsTest COMMAND htmltools_test)
add_test(NAME JSONRobustnessTest COMMAND json_robustness_test)
add_test(NAME HashToolsTest COMMAND hashtools_test)
add_test(NAME TimeToolsTest COMMAND timetools_test)

# Set Unicode and platform definitions
target_compile_definitions(jstool PRIVATE
    JSTOOL_EXPORTS
    _WINDOWS
    _USRDLL
    _WINDLL
    UNICODE
    _UNICODE
    WIN32
)

# Windows specific standard libraries
target_link_libraries(jstool PRIVATE
    shlwapi
    kernel32
    user32
    gdi32
    winspool
    comdlg32
    advapi32
    shell32
    ole32
    oleaut32
    uuid
    odbc32
    odbccp32
    uxtheme
    comctl32
    bcrypt
)

# Compiler specific options for MSVC
if(MSVC)
    # Use static CRT (/MT for release, /MTd for debug) to match vcxproj
    set_property(TARGET jstool PROPERTY 
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

