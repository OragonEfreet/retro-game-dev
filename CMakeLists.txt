cmake_minimum_required(VERSION 3.21)
project(my_games C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build banjo as static
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BANJO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Put all produced JS/WASM (emscripten) and native executables NEXT TO index.html
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
foreach(cfg IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
  string(TOUPPER "${cfg}" CFGU)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFGU} "${CMAKE_CURRENT_BINARY_DIR}")
endforeach()

# ---- banjo feature/config toggles ----
option(BJ_FEATURE_WIN32       "banjo: Add Win32 support"                                              OFF)
option(BJ_FEATURE_X11         "banjo: Add X11 support"                                                OFF)
option(BJ_FEATURE_MME         "banjo: Add Windows MME audio"                                          OFF)
option(BJ_FEATURE_ALSA        "banjo: Add ALSA audio"                                                 OFF)
option(BJ_FEATURE_EMSCRIPTEN  "banjo: Add Emscripten support"                                         OFF)
option(BJ_CONFIG_ALL          "banjo: Enable all CONFIG options"                                      OFF)
option(BJ_CONFIG_CHECKS_ABORT "banjo: Failed check aborts execution"                                  OFF)
option(BJ_CONFIG_CHECKS_LOG   "banjo: Log failed checks"                                              OFF)
option(BJ_CONFIG_LOG_COLOR    "banjo: Colored log output"                                             OFF)
option(BJ_CONFIG_PEDANTIC     "banjo: Extra runtime checks"                                           OFF)

include(FetchContent)

# Fetch banjo
FetchContent_Declare(
  banjo
  GIT_REPOSITORY https://github.com/OragonEfreet/banjo.git
  GIT_TAG        drawing
)
FetchContent_MakeAvailable(banjo)

# Track discovered game targets globally
set_property(GLOBAL PROPERTY BANJO_GAMES "")

function(_configure_game_target tgt)
  if (MSVC)
    target_compile_options(${tgt} PRIVATE /W4 /D_CRT_SECURE_NO_WARNINGS)
  else()
    target_compile_options(${tgt} PRIVATE -Wall -Wextra)
  endif()

  target_link_libraries(${tgt} PRIVATE banjo)
  if (NOT MSVC)
    target_link_libraries(${tgt} PRIVATE m)
  endif()

  if (EMSCRIPTEN)
    target_link_options(${tgt} PRIVATE
      "-sALLOW_MEMORY_GROWTH"
      "-sENVIRONMENT=web"
      "-sMODULARIZE=0"
      "-sEXPORT_ES6=0"
      "-sNO_EXIT_RUNTIME=1"
      "--pre-js" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ems_pre.js"
    )
  endif()

  get_property(_games GLOBAL PROPERTY BANJO_GAMES)
  list(APPEND _games ${tgt})
  set_property(GLOBAL PROPERTY BANJO_GAMES "${_games}")
endfunction()

function(add_banjo_games SRC_DIR)
  if (NOT IS_DIRECTORY "${SRC_DIR}")
    message(FATAL_ERROR "add_banjo_games: '${SRC_DIR}' is not a directory")
  endif()

  # 1) Top-level .c files
  file(GLOB _top_c RELATIVE "${SRC_DIR}" "${SRC_DIR}/*.c")
  foreach(_c ${_top_c})
    get_filename_component(_name "${_c}" NAME_WE)
    option(GAME_${_name} "Build game ${_name}" ON)
    if (GAME_${_name})
      add_executable(${_name} "${SRC_DIR}/${_c}")
      target_include_directories(${_name} PRIVATE "${SRC_DIR}")
      _configure_game_target(${_name})
    endif()
  endforeach()

  # 2) Immediate subdirectories
  file(GLOB _children RELATIVE "${SRC_DIR}" "${SRC_DIR}/*")
  foreach(_child ${_children})
    if (IS_DIRECTORY "${SRC_DIR}/${_child}")
      file(GLOB _c_in "${SRC_DIR}/${_child}/*.c")
      file(GLOB _h_in "${SRC_DIR}/${_child}/*.h")
      if (_c_in)
        option(GAME_${_child} "Build game ${_child}" ON)
        if (GAME_${_child})
          add_executable(${_child} ${_c_in} ${_h_in})
          target_include_directories(${_child} PRIVATE "${SRC_DIR}/${_child}")
          _configure_game_target(${_child})
        endif()
      else()
        message(WARNING "Folder '${_child}' has no .c files; skipping executable.")
      endif()
    endif()
  endforeach()
endfunction()

# -------- discover games under src/ --------
add_banjo_games("${CMAKE_CURRENT_SOURCE_DIR}/src")

# -------- generate index.html --------
get_property(ALL_GAMES GLOBAL PROPERTY BANJO_GAMES)
if (NOT ALL_GAMES)
  message(WARNING "No games found under src/. index.html will still be generated.")
endif()
list(SORT ALL_GAMES)

set(GAME_OPTIONS_HTML "")
foreach(G ${ALL_GAMES})
  set(GAME_OPTIONS_HTML "${GAME_OPTIONS_HTML}<option value=\"${G}\">${G}</option>\n")
endforeach()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/index.html.in"
  "${CMAKE_CURRENT_BINARY_DIR}/index.html"
  @ONLY
)

add_custom_target(index ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/index.html" ${ALL_GAMES})

message(STATUS "Games: ${ALL_GAMES}")
