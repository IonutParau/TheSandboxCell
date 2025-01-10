cmake_minimum_required(VERSION 3.31)

include(CheckIPOSupported)

project(TheSandboxCell C)

find_package(raylib 5.0 REQUIRED)

check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

if(NOT DEFINED TSC_MODE)
    message(STATUS "Defaulting to DEBUG mode")
    set(TSC_MODE "DEBUG")
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE) # LTO!!!!

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "-fPIC")
set(CMAKE_C_FLAGS_DEBUG "-g3")
set(CMAKE_C_FLAGS_RELEASE "-O3")

if(TSC_MODE EQUAL "DEBUG")
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(TSC_MODE EQUAL "RELEASE")
    set(CMAKE_BUILD_TYPE Release)
endif()

if(TSC_MODE EQUAL "TURBO")
    set(CMAKE_BUILD_TYPE Release)
    set(CMAKE_C_FLAGS_RELEASE "-Ofast")
    add_compile_definitions(TSC_TURBO)
endif()

if(DEFINED TSC_SINGLE_THREAD)
    add_compile_definitions(TSC_SINGLE_THREAD)
endif()

add_library(
    tsc SHARED
    src/threads/workers.c
    src/utils.c
    src/cells/cell.c
    src/cells/grid.c
    src/cells/ticking.c
    src/graphics/resources.c
    src/graphics/rendering.c
    src/cells/subticks.c
    src/saving/saving.c
    src/saving/saving_buffer.c
    src/graphics/ui.c
    src/api/api.c
    src/api/modloader.c
    src/api/value.c
    src/api/tscjson.c
    src/threads/tinycthread.c
)

target_link_libraries(tsc raylib)
    
target_link_libraries(tsc "m")

# MacOS Raylib schenanigans
if (APPLE)
    target_link_libraries(tsc "-framework IOKit")
    target_link_libraries(tsc "-framework Cocoa")
    target_link_libraries(tsc "-framework OpenGL")
endif()

add_executable(thesandboxcell src/main.c)

target_link_libraries(thesandboxcell tsc)

add_executable(tests src/testing.c src/saving/test_saving.c)

target_link_libraries(tests tsc)

if(ipo_supported)
    message(STATUS "IPO / LTO enabled")
else()
    message(STATUS "IPO / LTO not supported: ${ipo_error}")
endif()

install(TARGETS thesandboxcell DESTINATION bin)
