# viscan fork of PROJ: tests of the math-only build.
# Included (not add_subdirectory) from ViscanMathOnlyFinalize.cmake, because
# subdirectories cannot be added during deferred execution.

# Init files with valid definitions; the library must not use them.
set(_defs "${PROJ_BINARY_DIR}/viscan/test/defs")
file(WRITE "${_defs}/epsg"
  "<4326> +proj=longlat +datum=WGS84 +no_defs <>\n")
file(WRITE "${_defs}/mydefs"
  "<utm32> +proj=utm +zone=32 +ellps=GRS80 <>\n")
configure_file(${PROJ_SOURCE_DIR}/data/ITRF2014 ${_defs}/ITRF2014 COPYONLY)
# Must be ignored: tmerc/utm always use Poder/Engsager.
file(WRITE "${_defs}/proj.ini" "tmerc_default_algo = evenden_snyder\n")

add_executable(viscan_math_only_test
  ${CMAKE_CURRENT_LIST_DIR}/viscan_math_only_test.cpp)
target_include_directories(viscan_math_only_test PRIVATE ${PROJ_SOURCE_DIR}/src)
# Link every object of the static library: a missing stub for a function
# removed together with factory.cpp & co fails the build here.
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
  target_link_libraries(viscan_math_only_test PRIVATE
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,proj>")
else()
  target_link_libraries(viscan_math_only_test PRIVATE proj)
endif()

add_test(NAME viscan_math_only
  COMMAND viscan_math_only_test "${_defs}" "${PROJ_SOURCE_DIR}/data/tests")
set_property(TEST viscan_math_only PROPERTY ENVIRONMENT
  "PROJ_SKIP_READ_USER_WRITABLE_DIRECTORY=YES")

# Keep the example compiling. It is not run: it needs the BWTA2017 and
# GCG2016 grids.
add_executable(viscan_example_gk3_to_utm32
  ${VISCAN_DIR}/examples/gk3_to_utm32.cpp)
target_link_libraries(viscan_example_gk3_to_utm32 PRIVATE proj)
