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
- **Transverse Mercator always uses Poder/Engsager.** This applies to `tmerc`,
  `utm` and `tmerczoned`. `+approx`, `+algo=evenden_snyder` and `+algo=auto`
  are rejected, and `tmerc_default_algo` in `proj.ini` is ignored. Results
  cannot change because of a setting outside the definition. On a sphere
  (no flattening) PROJ uses the exact spherical formulas, as upstream does.

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
| `viscan/src/viscan_printf.cpp` | Locale-independent `snprintf` (always `.` as decimal separator). PROJ's formats are handled with `std::to_chars` (about 50 ns per number); other formats and unsupported platforms use the C library (see "Number formatting speed"). |
| `viscan/test/` | viscan test, list of excluded upstream tests. |
| `viscan/tools/` | `merge_upstream.sh`, `update_test_exclusions.py`, `gie_filter.py`. |
| `viscan/examples/gk3_to_utm32.cpp` | Example: GK3 → BWTA2017 → WGS84 → UTM32 pipeline. |

### Changes to upstream files

All of them are small additions:

| File | Change |
|---|---|
| `CMakeLists.txt` | `include(viscan/cmake/ViscanPlatform.cmake)` before `project()` (Apple minimum versions); `include(viscan/cmake/ViscanMathOnly.cmake)`; SQLite check and `add_subdirectory(data)` (builds `proj.db`, installs init files) wrapped in `if(NOT VISCAN_MATH_ONLY)`. |
| `src/init.cpp` | `get_init()` returns an error under `#ifdef VISCAN_MATH_ONLY`, which disables `+init=`. |
| `src/projections/tmerc.cpp` | `getAlgoFromParams()` always returns Poder/Engsager under `#ifdef VISCAN_MATH_ONLY` and rejects other algorithms. |
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
| `CMAKE_MSVC_RUNTIME_LIBRARY` | not set: CMake's default, the DLL runtime (`/MD`, `/MDd` for Debug) |
| `CMAKE_OSX_DEPLOYMENT_TARGET` (macOS) | `15.0`, from `viscan/cmake/ViscanPlatform.cmake`; an explicit value or `MACOSX_DEPLOYMENT_TARGET` wins |
| `IOS_DEPLOYMENT_TARGET` (iOS, `iOS.cmake`) | `18.0` |
| `ENABLE_CURL`, `EMBED_RESOURCE_FILES`, `USE_ONLY_EMBEDDED_RESOURCE_FILES` | forced `OFF`; turning one on is a configure error |

libtiff stays enabled (`ENABLE_TIFF`), for GeoTIFF grids such as `GCG2016.tif`.

**MSVC runtime: `/MD`.** It's a static library, but it uses the DLL runtime,
because its users (`lib-multiplatform-coordinate-transformation`, the Python
binding) build with `/MD`. The linker refuses `/MT` objects there (LNK2038
"mismatch detected for 'RuntimeLibrary'").
- **History:** the old fork appended `/MT` to `CMAKE_C(XX)_FLAGS`. CMake's
  default `/MD` came later on the command line and won (warning D9025), so
  releases up to v0.2.x were effectively `/MD`. v0.3.0 made `/MT` effective,
  which broke the Windows build of the coordinate transformation library;
  v0.3.1 returns to `/MD`.
- **Bundled libraries:** the release workflow installs libtiff and its
  dependencies with the vcpkg triplet `x64-windows-static-md` (static
  libraries, DLL runtime), so the whole package uses one runtime.

## Using the library

- Define `PROJ_STATICALLY_LINKED` when compiling against the headers (MSVC).
- Link libtiff (and its dependencies); SQLite and curl are not needed.
- Always call `proj_context_set_search_paths()` with the directory holding your
  grids. Otherwise PROJ also searches the user-writable directory,
  `PROJ_DATA`/`PROJ_LIB`, `../share/proj`, the compiled-in install directory and
  the current directory.
- `tmerc`/`utm` need no `+algo`: Poder/Engsager is always used. Explicitly
  writing `+algo=poder_engsager` is still accepted.
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
| `tmerc`/`utm` with `+approx`, `+algo=evenden_snyder`, `+algo=auto` | Always Poder/Engsager |
| Grids via CDN (`https://…`, `PROJ_NETWORK`) | Network |

These calls fail cleanly through the C API (NULL / error code). In the C++ API
they throw `osgeo::proj::io::FactoryException`.

### Number formatting speed

SQLite used to format numbers as text for PROJ, in WKT, PROJJSON and PROJ
strings. `proj_create()` uses this too, about 16 numbers for a typical
pipeline. `proj_trans()` and all projection math never format numbers.

| Path | Per number | Where |
|---|---|---|
| `std::to_chars` | ~50 ns | Linux (GCC ≥ 11), Windows (MSVC), Android, Apple with deployment target ≥ macOS 13.3 / iOS 16.3. This includes the pinned macOS 15 / iOS 18. |
| C library printf, "C" locale | ~280 ns on macOS | Only toolchains without `std::to_chars(double)` |

CMake detects which one is available with the actual compiler and deployment
target (`VISCAN_HAVE_TO_CHARS_DOUBLE`). With the slow path, a typical
`proj_create()` takes about 4 µs longer, roughly 5 %. `proj_trans()` is not
affected.

## Compatibility with the previous fork (`main`, PROJ ~9.3)

Checked by reading the code of both versions (2026-11). Every operation and
parameter that the previous fork accepted is still supported, with these
exceptions:

| Not supported anymore | Why |
|---|---|
| `tmerc`/`utm` with `+approx`, `+algo=evenden_snyder`, `+algo=auto` | Deliberate: always Poder/Engsager (`main` used Poder/Engsager by default too) |
| `+init=<file>:<key>` | Deliberate: no definitions from files (`main` read init files from the search path) |
| `+proj=isea` with `+mode`, `+aperture`, `+resolution`, `+rescale` | Removed upstream (isea rewritten, 9.8/9.9). These parameters are now silently ignored, so `+mode=di/dd/hex` gives plane coordinates. |
| `+step +inv +proj=affine` with a non-invertible matrix | `proj_create()` now fails. On `main` the pipeline was created but failed on every coordinate. |

Details:

- Operations: `main` had 183 and the branch has 192. None was removed. New:
  `airocean`, `dsea`, `hourglass`, `interrupted`, `ivea`, `mod_krovak`,
  `spilhaus`, `tmerczoned`, `tsea`.
- Pipeline syntax: `+proj=pipeline`, `+step`, `+inv`, `+omit_fwd`,
  `+omit_inv`, `push`/`pop` with `+v_1`…`+v_4`: unchanged.
  `+omit_fwd` steps no longer need a forward function.
- Parameters: every parameter read on `main` is still read, apart from the
  `isea` ones above. `+t_final` (including `now`) moved to common code.
- Parameter values: all compared keywords are unchanged (e.g.
  `+convention=`, `+order=`), apart from the `isea` modes. The predefined
  `+ellps=` (23), `+datum=` (15), `+units=` (11) and `+pm=` tables are
  identical.
- Stricter checks (new errors) only concern new features (`+coord_type` and
  offset grids in `gridshift`, new projections). `axisswap` errors are only
  reworded.
- Not covered here: numerical changes. Upstream made some projections more
  exact: `aea`, `cea`, `laea`, `eqearth`, `healpix` and `rhealpix` (9.7),
  spherical `tmerc` (9.8) and inverse `cass` (9.6).

## Tests

```sh
cmake -S . -B build-tests -DBUILD_TESTING=ON   # needs GTest (or network) and SQLite3 dev files
cmake --build build-tests -j
ctest --test-dir build-tests
```

- `viscan_math_only` (viscan test) checks:
  - EPSG codes, `+init=` (also with `PROJ_USE_PROJ4_INIT_RULES`), the network
    and GeoPackage TIN files are rejected
  - `tmerc`/`utm` use Poder/Engsager even when `proj.ini` asks for another
    algorithm, and other algorithms are rejected
  - `proj_cleanup()` works
  - number formatting equals printf in the C locale (200,000 random values),
    and WKT/PROJJSON use `.` under a German locale
  - UTM and the GK3→UTM32 pipeline compute correctly, and local grids work
  - The test links the whole static library, so a missing stub fails the
    build.
- All upstream tests are built **unchanged**. Tests that need the removed
  functionality are listed in `viscan/test/excluded_tests.cmake`:
  - individual gtest cases are excluded through `GTEST_FILTER`
  - individual gie operations are excluded: the original test is disabled
    and `<name>.filtered` runs the gie file without them
    (`viscan/tools/gie_filter.py`). An entry that no longer matches an
    operation fails the test.
  - tests where everything fails are marked `DISABLED`; ctest shows them as
    "Not Run"
- Python 3 is needed for the test build (gie filtering).
- Upstream unit tests create SQLite databases themselves, so the *test* build
  needs SQLite3; the library does not.
- Applying the exclusions needs CMake ≥ 3.28.

Status with PROJ 9.9.0:

| Category | Result |
|---|---|
| Running tests (viscan test included) | 36 pass |
| gie files run with operations excluded | `Builtins` 375 of 380 (the non-Poder/Engsager tmerc cases), `Builtins2` 73 of 75, `4D-API-cs2cs-style` 52 of 54 (`+init=`), `defmodel` 6 of 16, `tinshift_gpkg` 2 of 7 |
| Disabled whole tests | 20 GIGS suites (every definition is `+init=epsg:`), `epsg_no_grid` |
| Excluded gtest cases | 581 cases (C++ API: 554 of 1416). All are traced to database, init-file or network use, or to the Poder/Engsager rule (`io.projparse_tmerc_approx`). |

Also running: `Ellipsoid`, `Axisswap`, `Unitconvert`, `GDA`, `DHDN_ETRS89`,
`Deformation`, `geotiff_grids`, `gridshift`, `tinshift` (JSON), the geodesic
tests, and 862 C++ API cases.

## Merging a new PROJ release

```sh
git switch -c viscan/proj-<version>
viscan/tools/merge_upstream.sh <version>
```

The script merges the tag and drops upstream's CI workflows. Expected
conflicts are limited to the hooks in `CMakeLists.txt`, `src/init.cpp`,
`src/projections/tmerc.cpp` and `src/proj.h`. Then:

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
- Apple's `printf` writes `%g` exact ties with a trailing zero
  (`3.84160e+06` instead of `3.8416e+06`), which only matters on the slow
  path. The value is the same. The viscan test compares digits and values,
  not that zero.

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
- `/MT` had no effect on Windows (warning D9025). The runtime is now `/MD`
  without that warning, see "Building".
- PROJJSON numbers followed the process locale (`,` with a German locale).
- The Transverse Mercator algorithm could be switched by `+approx`/`+algo` or
  by a `proj.ini`.
- Large `#ifdef` hunks in upstream files made merging new releases
  impractical; tests were disabled entirely.
