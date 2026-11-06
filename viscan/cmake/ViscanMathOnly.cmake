# viscan fork of PROJ: "math only" build.
#
# Included from the top-level CMakeLists.txt (right after include(policies)).
# Produces a static PROJ library that evaluates PROJ strings and pipelines
# only. It has no database (proj.db / EPSG), no network access, and it never
# reads CRS or pipeline definitions from files (+init=). Grid and helper
# files are still read from the search paths.
#
# Upstream files are left untouched as far as possible. Sources are swapped
# and test exclusions applied at the end of the configure step, see
# ViscanMathOnlyFinalize.cmake. See also FORK_CHANGES.md.

option(VISCAN_MATH_ONLY
  "viscan: build PROJ without database, network and definition files" ON)
if(NOT VISCAN_MATH_ONLY)
  return()
endif()

set(VISCAN_DIR "${PROJ_SOURCE_DIR}/viscan")
message(STATUS "viscan: math-only build (no database, network, init files)")

# Options that would bring removed functionality back are rejected.
foreach(_viscan_opt ENABLE_CURL EMBED_RESOURCE_FILES
                    USE_ONLY_EMBEDDED_RESOURCE_FILES)
  if(${_viscan_opt})
    message(FATAL_ERROR
      "${_viscan_opt}=ON cannot be combined with VISCAN_MATH_ONLY=ON")
  endif()
  set(${_viscan_opt} OFF CACHE BOOL "Disabled by VISCAN_MATH_ONLY" FORCE)
endforeach()

# Defaults (can still be overridden on the command line).
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build PROJ library shared.")
set(BUILD_APPS OFF CACHE BOOL
  "Build PROJ applications (default value for BUILD_CCT, BUILD_CS2CS, etc.)")
set(BUILD_TESTING OFF CACHE BOOL "Build the testing tree.")
option(VISCAN_IGNORE_TEST_EXCLUSIONS
  "viscan: run all upstream tests, ignoring viscan/test/excluded_tests.cmake"
  OFF)

# The static library is linked into shared objects by its users.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# MSVC: static runtime (/MT), also for Debug builds. Uses the CMake runtime
# selection (policy CMP0091) instead of appending /MT to CMAKE_<LANG>_FLAGS,
# which CMake's default /MD overrode (warning D9025).
if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

# The library itself does not use SQLite. Some upstream unit tests create
# SQLite databases themselves, so the test build needs it.
if(BUILD_TESTING)
  find_package(SQLite3 REQUIRED)
  if(NOT TARGET SQLite3::SQLite3) # CMake < 4.3
    add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
  endif()
endif()

# Swap sources, drop the SQLite link and set up tests once all targets exist.
cmake_language(DEFER CALL include
  "${VISCAN_DIR}/cmake/ViscanMathOnlyFinalize.cmake")
