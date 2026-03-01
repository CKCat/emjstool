cmake_minimum_required(VERSION 3.15)
project(jstool LANGUAGES C CXX RC)

# EmEditor Plugin requires building a DLL
add_library(jstool SHARED
    src/jstool.cpp
    src/jstool.h
    src/etlframe.h
    src/json.hpp
    src/plugin.h
    src/resource.h
    src/Resource.rc
    src/jstool.def
)

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
)

# Compiler specific options for MSVC
if(MSVC)
    # Enable /utf8 for MSVC to prevent warning C4819 (data loss on code page 936)
    target_compile_options(jstool PRIVATE /utf8)

    # Use static CRT (/MT for release, /MTd for debug) to match vcxproj
    set_property(TARGET jstool PROPERTY 
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

