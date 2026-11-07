# viscan fork of PROJ: minimum Apple OS versions.
#
# Included from the top-level CMakeLists.txt BEFORE project(), because CMake
# reads CMAKE_OSX_DEPLOYMENT_TARGET when the compiler is set up.
#
# - macOS: 15.0 (Sequoia), unless CMAKE_OSX_DEPLOYMENT_TARGET is given.
# - iOS:   18.0, set by the iOS toolchain file iOS.cmake (IOS_DEPLOYMENT_TARGET).
#
# Both are new enough for std::to_chars(double) (macOS 13.3 / iOS 16.3), so
# the fast number formatting in viscan_printf.cpp is used.
#
# Note: "macOS 16" does not exist. Apple renamed it to macOS 26, and clang
# silently turns a 16.0 deployment target into 26.0.

set(VISCAN_MACOS_DEPLOYMENT_TARGET "15.0")
set(VISCAN_IOS_DEPLOYMENT_TARGET "18.0") # default in iOS.cmake, keep in sync

# An explicit -DCMAKE_OSX_DEPLOYMENT_TARGET or MACOSX_DEPLOYMENT_TARGET in the
# environment wins. An empty value (left in the cache of older build
# directories) counts as unset.
if(CMAKE_HOST_APPLE AND NOT CMAKE_TOOLCHAIN_FILE
   AND NOT CMAKE_OSX_DEPLOYMENT_TARGET
   AND NOT DEFINED ENV{MACOSX_DEPLOYMENT_TARGET})
  set(CMAKE_OSX_DEPLOYMENT_TARGET "${VISCAN_MACOS_DEPLOYMENT_TARGET}"
      CACHE STRING "Minimum macOS version (viscan default)" FORCE)
endif()
