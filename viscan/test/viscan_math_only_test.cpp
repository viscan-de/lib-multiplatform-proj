/*
 * viscan fork of PROJ: checks the guarantees of the math-only build.
 *
 *   viscan_math_only_test <test data dir> <PROJ data/tests dir>
 *
 * - CRS and pipeline definitions never come from the database or from files
 * - no network access, nothing throws through the C API
 * - proj_cleanup() works
 * - numbers are written with '.' whatever the locale
 * - PROJ strings and pipelines compute correctly
 *
 * The executable links the whole static library, so it also fails to build
 * when a stub for a removed upstream function is missing.
 */

#include "proj.h"

#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

int gFailures = 0;

void check(bool ok, const std::string &what) {
    std::printf("%s %s\n", ok ? "[  OK  ]" : "[ FAIL ]", what.c_str());
    if (!ok)
        ++gFailures;
}

void setEnv(const char *name, const char *value) {
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

void quietLogger(void *, int, const char *) {}

PJ_CONTEXT *newContext(const std::string &searchPath) {
    PJ_CONTEXT *ctx = proj_context_create();
    proj_log_func(ctx, nullptr, quietLogger);
    const char *paths[] = {searchPath.c_str()};
    proj_context_set_search_paths(ctx, 1, paths);
    return ctx;
}

void checkRejected(PJ_CONTEXT *ctx, const std::string &definition) {
    PJ *P = proj_create(ctx, definition.c_str());
    check(P == nullptr, "rejected: " + definition);
    proj_destroy(P);
}

void testNoDefinitionsFromDatabaseOrFiles(const std::string &dataDir) {
    PJ_CONTEXT *ctx = newContext(dataDir);

    checkRejected(ctx, "EPSG:4326");
    checkRejected(ctx, "EPSG:25832");
    checkRejected(ctx, "urn:ogc:def:crs:EPSG::4326");
    checkRejected(ctx, "ESRI:102100");
    checkRejected(ctx, "WGS 84"); // lookup by name

    // dataDir contains init files "epsg" and "mydefs" with valid definitions.
    checkRejected(ctx, "+init=epsg:4326");
    checkRejected(ctx, "+init=mydefs:utm32");
    checkRejected(ctx, "+proj=pipeline +step +init=mydefs:utm32");
    checkRejected(ctx, "+init=ITRF2014:ITRF2008");
    proj_context_use_proj4_init_rules(ctx, 1);
    checkRejected(ctx, "+init=epsg:4326");
    checkRejected(ctx, "+init=mydefs:utm32");
    proj_context_use_proj4_init_rules(ctx, 0);

    PJ *P = proj_create_crs_to_crs(ctx, "EPSG:4326", "EPSG:25832", nullptr);
    check(P == nullptr, "proj_create_crs_to_crs(EPSG:4326, EPSG:25832) fails");
    proj_destroy(P);

    proj_context_destroy(ctx);
}

void testNoNetwork(const std::string &dataDir) {
    setEnv("PROJ_NETWORK", "ON");
    PJ_CONTEXT *ctx = newContext(dataDir);
    check(!proj_context_is_network_enabled(ctx),
          "network disabled despite PROJ_NETWORK=ON");
    check(!proj_context_set_enable_network(ctx, 1),
          "proj_context_set_enable_network(TRUE) returns FALSE");
    check(!proj_context_is_network_enabled(ctx), "network still disabled");
    check(!proj_download_file(ctx, "de_adv_BETA2007.tif", 0, nullptr,
                              nullptr),
          "proj_download_file() returns FALSE");
    checkRejected(ctx, "+proj=hgridshift "
                       "+grids=https://cdn.proj.org/de_adv_BETA2007.tif");
    proj_context_destroy(ctx);
    setEnv("PROJ_NETWORK", "OFF");
}

void testGeoPackageTinRejected(const std::string &projTestsDir) {
    PJ_CONTEXT *ctx = newContext(projTestsDir);
    checkRejected(ctx,
                  "+proj=tinshift +file=tinshift_simplified_kkj_etrs.gpkg");
    PJ *P =
        proj_create(ctx, "+proj=tinshift +file=tinshift_simplified_kkj_etrs.json");
    check(P != nullptr, "tinshift with a JSON file works");
    proj_destroy(P);
    proj_context_destroy(ctx);
}

double transformRoundTrip(PJ *P, PJ_COORD c, PJ_COORD &projected) {
    projected = proj_trans(P, PJ_FWD, c);
    const PJ_COORD back = proj_trans(P, PJ_INV, projected);
    return std::fabs(back.lpz.lam - c.lpz.lam) +
           std::fabs(back.lpz.phi - c.lpz.phi);
}

void testMath(const std::string &dataDir) {
    PJ_CONTEXT *ctx = newContext(dataDir);

    // UTM 32N on its central meridian (9 deg E): easting is exactly 500 km.
    PJ *P = proj_create(ctx, "+proj=utm +zone=32 +ellps=GRS80");
    check(P != nullptr, "+proj=utm +zone=32 +ellps=GRS80 works");
    if (P) {
        PJ_COORD c = proj_coord(proj_torad(9.0), proj_torad(48.0), 0, 0);
        PJ_COORD xy;
        const double err = transformRoundTrip(P, c, xy);
        check(std::fabs(xy.enu.e - 500000.0) < 1e-6,
              "UTM easting on central meridian = 500000 m");
        check(err < 1e-12, "UTM round trip");
    }
    proj_destroy(P);

    // Gauss-Krueger zone 3 (Bessel) -> UTM 32N, without grids.
    P = proj_create(ctx, "+proj=pipeline "
                         "+step +inv +proj=tmerc +lat_0=0 +lon_0=9 +k=1 "
                         "+x_0=3500000 +y_0=0 +ellps=bessel "
                         "+step +proj=utm +zone=32 +ellps=GRS80");
    check(P != nullptr, "GK3 -> UTM32 pipeline works");
    if (P) {
        PJ_COORD gk = proj_coord(3500000.0, 5320000.0, 0, 0);
        const PJ_COORD utm = proj_trans(P, PJ_FWD, gk);
        const PJ_COORD back = proj_trans(P, PJ_INV, utm);
        check(std::fabs(utm.enu.e - 500000.0) < 1e-6,
              "GK3 -> UTM32 keeps the central meridian");
        check(std::fabs(back.enu.e - gk.enu.e) < 1e-6 &&
                  std::fabs(back.enu.n - gk.enu.n) < 1e-6,
              "GK3 -> UTM32 round trip");
    }
    proj_destroy(P);

    // Grid files are still read from the search path.
    P = proj_create(ctx, "+proj=hgridshift +grids=BETA2007.gsb");
    check(P != nullptr, "hgridshift with a local grid file works");
    proj_destroy(P);

    proj_context_destroy(ctx);
}

void testLocaleIndependentNumbers(const std::string &dataDir) {
    const char *locales[] = {"de_DE.UTF-8", "de_DE.utf8", "de_DE", "German"};
    const char *found = nullptr;
    for (const char *loc : locales) {
        if (std::setlocale(LC_ALL, loc)) {
            found = loc;
            break;
        }
    }
    if (!found) {
        std::printf("[ SKIP ] no German locale installed\n");
        return;
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f", 0.5);
    std::printf("         locale %s formats 0.5 as \"%s\"\n", found, buf);

    PJ_CONTEXT *ctx = newContext(dataDir);
    PJ *P = proj_create(ctx, "+proj=utm +zone=32 +ellps=GRS80 +type=crs");
    if (P) {
        const char *wkt = proj_as_wkt(ctx, P, PJ_WKT2_2019, nullptr);
        check(wkt && std::strstr(wkt, "0.9996") && !std::strstr(wkt, "0,9996"),
              "WKT uses '.' under a German locale");
        const char *json = proj_as_projjson(ctx, P, nullptr);
        check(json && std::strstr(json, "0.9996") &&
                  !std::strstr(json, "0,9996"),
              "PROJJSON uses '.' under a German locale");
    } else {
        check(false, "create +type=crs for locale test");
    }
    proj_destroy(P);
    proj_context_destroy(ctx);
    std::setlocale(LC_ALL, "C");
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s <test data dir> <PROJ data/tests dir>\n",
                     argv[0]);
        return 2;
    }
    const std::string dataDir = argv[1];
    const std::string projTestsDir = argv[2];

    testNoDefinitionsFromDatabaseOrFiles(dataDir);
    testNoNetwork(dataDir);
    testGeoPackageTinRejected(projTestsDir);
    testMath(projTestsDir);
    testLocaleIndependentNumbers(dataDir);

    proj_cleanup(); // used to abort the process in the old fork
    check(true, "proj_cleanup() returns");

    std::printf("%d failure(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
