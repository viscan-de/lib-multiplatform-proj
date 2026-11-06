# viscan fork of PROJ

## Purpose

This fork is used only as the math engine of viscan's own coordinate
transformation library. That library builds PROJ strings and
`+proj=pipeline` definitions itself and has PROJ evaluate them.

The fork is built as a **static library** in a **math-only** configuration,
which guarantees:

- **No CRS or pipeline definitions from the database.** There is no `proj.db`.
  EPSG, ESRI, IGNF, … codes and name lookups are rejected, because the EPSG
  data is incomplete or inaccurate for our purposes.
- **No definitions from files.** `+init=<file>:<key>` is rejected, whatever
  the file and whatever `PROJ_USE_PROJ4_INIT_RULES` is set to.
- **No network access.** Grids are never downloaded; `PROJ_NETWORK=ON` and
  `network = on` in `proj.ini` have no effect.
- **No SQLite.** The library neither contains nor needs SQLite.

PROJ still reads grid files (`.gsb`, `.tif`, `.gtx`, …), TIN and deformation
JSON files, and `proj.ini` from the search paths set by our library.

`viscan/test/viscan_math_only_test.cpp` checks all of this. See "Tests" below.

| | |
|---|---|
| Upstream | https://github.com/OSGeo/PROJ |
| Current upstream release | **9.9.0** |
| Switch | CMake option `VISCAN_MATH_ONLY` (default `ON`) |

## How the fork is organised

The fork keeps upstream files untouched as far as possible, so that new PROJ
releases merge with few or no conflicts. Everything viscan-specific lives in
`viscan/`:

| Path | What |
|---|---|
| `viscan/cmake/ViscanMathOnly.cmake` | Included early from `CMakeLists.txt`: options, static library defaults, MSVC runtime, PIC. |
| `viscan/cmake/ViscanMathOnlyFinalize.cmake` | Runs at the end of the configure step (`cmake_language(DEFER)`): swaps sources of target `proj` for the stubs, removes the SQLite link, prepares test data and applies the test exclusions. |
| `viscan/src/viscan_database_stubs.cpp` | Replaces `src/iso19111/factory.cpp`. `DatabaseContext::create()` throws `FactoryException`, so no database or authority factory can exist. The other methods are unreachable and only make the library link. |
| `viscan/src/viscan_network_stubs.cpp` | Replaces `src/networkfilemanager.cpp`. The network C API reports "disabled" and never throws. |
| `viscan/src/viscan_tinshift_gpkg_stubs.cpp` | Replaces `src/transformations/tinshift_gpkg.cpp` and `src/sqlite3_utils.cpp`. GeoPackage TIN files are rejected. |
| `viscan/src/sqlite3_stub/sqlite3.h` | Stand-in for `<sqlite3.h>` so that upstream sources compile without SQLite. It maps `sqlite3_snprintf` to `viscan_printf.cpp`. |
| `viscan/src/viscan_printf.cpp` | Locale-independent `snprintf` (always `.` as decimal separator). |
| `viscan/test/` | viscan test, list of excluded upstream tests. |
| `viscan/tools/` | `merge_upstream.sh`, `update_test_exclusions.py`. |
| `viscan/examples/gk3_to_utm32.cpp` | Example: GK3 → BWTA2017 → WGS84 → UTM32 pipeline. |

### Changes to upstream files

All of them are small additions:

| File | Change |
|---|---|
| `CMakeLists.txt` | `include(viscan/cmake/ViscanMathOnly.cmake)`; SQLite check and `add_subdirectory(data)` (builds `proj.db`, installs init files) wrapped in `if(NOT VISCAN_MATH_ONLY)`. |
| `src/init.cpp` | `get_init()` returns an error under `#ifdef VISCAN_MATH_ONLY`, which disables `+init=`. |
| `src/proj.h` | `PROJ_STATICALLY_LINKED` defines an empty `PROJ_DLL`. Users of the static library define it (needed with MSVC). |

Fork-only files outside `viscan/`: `build-posix.sh`, `build-windows.sh`,
`CMakePresets.json`, `iOS.cmake`, `ios.toolchain.cmake`, `LICENSE.MD`,
`.github/workflows/{build-multiplatform,build-ubuntu,release-multiplatform}.yml`.
Upstream's own CI workflows are removed.

## Building

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -t proj
```

`VISCAN_MATH_ONLY=ON` makes these the defaults:

| Setting | Value |
|---|---|
| `BUILD_SHARED_LIBS` | `OFF` (static library) |
| `BUILD_APPS` | `OFF` |
| `BUILD_TESTING` | `OFF` |
| `CMAKE_POSITION_INDEPENDENT_CODE` | `ON` |
| `CMAKE_MSVC_RUNTIME_LIBRARY` | `MultiThreaded` (`/MT`, also for Debug) |
| `ENABLE_CURL`, `EMBED_RESOURCE_FILES`, `USE_ONLY_EMBEDDED_RESOURCE_FILES` | forced `OFF`; turning one on is a configure error |

libtiff stays enabled (`ENABLE_TIFF`), for GeoTIFF grids such as `GCG2016.tif`.

The static runtime is selected through `CMAKE_MSVC_RUNTIME_LIBRARY`. The old
fork appended `/MT` to `CMAKE_C(XX)_FLAGS`; CMake's default `/MD` came later on
the command line and won, which caused warning D9025.

## Using the library

- Define `PROJ_STATICALLY_LINKED` when compiling against the headers (MSVC).
- Link libtiff (and its dependencies); SQLite and curl are not needed.
- Always call `proj_context_set_search_paths()` with the directory holding your
  grids. Otherwise PROJ also searches the user-writable directory,
  `PROJ_DATA`/`PROJ_LIB`, `../share/proj`, the compiled-in install directory and
  the current directory.
- Write `+algo=poder_engsager` on `tmerc`/`utm` steps. Without `+algo`, the
  algorithm comes from `tmerc_default_algo` in a `proj.ini` found on the search
  path (default `poder_engsager`).
- Use one `PJ_CONTEXT` per thread and reuse `PJ` objects; creating them per
  coordinate reloads grids every time.

### What does not work (by design)

| Feature | Why |
|---|---|
| `EPSG:xxxx`, `urn:ogc:…`, names like `"WGS 84"` | Need `proj.db` |
| `proj_create_crs_to_crs()` with authority codes; operation search, `proj_identify()`, name normalisation in WKT | Need `proj.db` |
| `+init=…` | Definitions from files |
| `+proj=defmodel` | The model JSON names its CRS by EPSG code, which needs `proj.db` |
| `+proj=tinshift +file=*.gpkg` | Needs SQLite (JSON TIN files work) |
| Grids via CDN (`https://…`, `PROJ_NETWORK`) | Network |

These calls fail cleanly through the C API (NULL / error code). In the C++ API
they throw `osgeo::proj::io::FactoryException`.

## Tests

```sh
cmake -S . -B build-tests -DBUILD_TESTING=ON   # needs GTest (or network) and SQLite3 dev files
cmake --build build-tests -j
ctest --test-dir build-tests
```

- `viscan_math_only` (viscan test) checks:
  - EPSG codes, `+init=` (also with `PROJ_USE_PROJ4_INIT_RULES`), the network
    and GeoPackage TIN files are rejected
  - `proj_cleanup()` works
  - WKT/PROJJSON use `.` under a German locale
  - UTM and the GK3→UTM32 pipeline compute correctly, and local grids work
  - The test links the whole static library, so a missing stub fails the
    build.
- All upstream tests are built **unchanged**. Tests that need the removed
  functionality are listed in `viscan/test/excluded_tests.cmake`:
  - whole ctest tests are marked `DISABLED`; ctest shows them as "Not Run"
  - individual gtest cases are excluded through `GTEST_FILTER`
- Upstream unit tests create SQLite databases themselves, so the *test* build
  needs SQLite3; the library does not.
- Applying the exclusions needs CMake ≥ 3.28.

Status with PROJ 9.9.0:

| Category | Result |
|---|---|
| Running tests (viscan test included) | 32 pass |
| Disabled whole tests | 25: GIGS suites (`+init=epsg:`), `defmodel`, `tinshift_gpkg`, `epsg_no_grid`, `Builtins2`, `4D-API-cs2cs-style` |
| Excluded gtest cases | 580 cases (C++ API: 553 of 1416), all of them traced to database, init-file or network use |

Still running: `Builtins`, `Ellipsoid`, `Axisswap`, `Unitconvert`, `GDA`,
`DHDN_ETRS89`, `Deformation`, `geotiff_grids`, `gridshift`, `tinshift`
(JSON), the geodesic tests, and 863 C++ API cases.

## Merging a new PROJ release

```sh
git switch -c viscan/proj-<version>
viscan/tools/merge_upstream.sh <version>
```

The script merges the tag and drops upstream's CI workflows. Expected
conflicts are limited to the hooks in `CMakeLists.txt`, `src/init.cpp` and
`src/proj.h`. Then:

1. Build with tests, **ignoring** the exclusions:
   ```sh
   cmake -S . -B build-tests -DBUILD_TESTING=ON -DVISCAN_IGNORE_TEST_EXCLUSIONS=ON
   cmake --build build-tests -j
   ```
   - **Undefined symbols** (linking `viscan_math_only_test` or a unit test):
     upstream added or changed a database method. Add the stub to
     `viscan/src/viscan_database_stubs.cpp`, copying the signature from
     `src/iso19111/factory.cpp`.
   - **Configure error "upstream source … no longer part of target proj":**
     a replaced source file was renamed. Update
     `ViscanMathOnlyFinalize.cmake`.
   - **Compile error in `sqlite3_stub/sqlite3.h`:** upstream uses more of the
     SQLite API in a header. Add the declaration. Never add a definition: the
     library must not call SQLite.
2. `ctest --test-dir build-tests -R viscan_math_only --output-on-failure`
3. `python3 viscan/tools/update_test_exclusions.py build-tests`, then
   **review** `git diff viscan/test/excluded_tests.cmake`. Every new exclusion
   must be explained by database, init-file or network use. Broken projection
   math would show up here too.
4. Build the release configuration (`cmake --build … -t proj`) and commit.

## Known limitations

- `cmake --install` produces a `proj-config.cmake` that still declares a
  dependency on SQLite3 (upstream template). The release workflow copies
  headers and libraries directly and does not use it.
- The `GTEST_FILTER` for the C++ API tests is about 28,000 characters long.
  That is fine on Linux and macOS, but close to the Windows limit of 32,767
  characters per environment variable.

## History

Before 2026-11 the fork was based on upstream `e9c0e8c8` (2023-11, between
PROJ 9.3 and 9.4). The database and network code was disabled with
`#ifdef SQLLITE_ENABLED` (plus `#undef`) around most of `factory.cpp` and
`networkfilemanager.cpp`, and the tests were commented out. That version had
these problems, all fixed now:

- `proj_cleanup()` aborted the process (its stub threw `std::runtime_error`).
- `proj_context_set_enable_network()`, `proj_download_file()` and others threw
  C++ exceptions through the C API.
- `+init=<file>:<key>` still read definitions from init files.
- SQLite was still needed (`sqlite3_snprintf`) on non-Windows platforms.
- `/MT` had no effect on Windows (warning D9025).
- PROJJSON numbers followed the process locale (`,` with a German locale).
- Large `#ifdef` hunks in upstream files made merging new releases
  impractical; tests were disabled entirely.
