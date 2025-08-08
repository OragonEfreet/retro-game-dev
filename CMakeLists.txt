cmake_minimum_required(VERSION 3.21)
project(retro_game_dev C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

option(BJ_FEATURE_WIN32       "banjo: Add Win32 support"             OFF)
option(BJ_FEATURE_X11         "banjo: Add X11 support"               OFF)
option(BJ_FEATURE_MME         "banjo: Add Windows MME audio"         OFF)
option(BJ_FEATURE_ALSA        "banjo: Add ALSA audio"                OFF)
option(BJ_FEATURE_EMSCRIPTEN  "banjo: Add Emscripten support"        OFF)
option(BJ_CONFIG_ALL          "banjo: Enable all CONFIG options"     OFF)
option(BJ_CONFIG_CHECKS_ABORT "banjo: Failed check aborts execution" OFF)
option(BJ_CONFIG_CHECKS_LOG   "banjo: Log failed checks"             OFF)
option(BJ_CONFIG_LOG_COLOR    "banjo: Colored log output"            OFF)
option(BJ_CONFIG_PEDANTIC     "banjo: Extra runtime checks"          OFF)

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BANJO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(
  banjo
  GIT_REPOSITORY https://github.com/OragonEfreet/banjo.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(banjo)

add_executable(pong pong.c)
target_link_libraries(pong PRIVATE banjo)

if (NOT MSVC)
  target_link_libraries(pong PRIVATE m)
endif()

if (EMSCRIPTEN)
  set_target_properties(pong PROPERTIES SUFFIX ".html")
endif()

