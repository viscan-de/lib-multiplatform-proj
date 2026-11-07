/*
 * viscan fork of PROJ: example of using the math-only library.
 *
 * Gauss-Krueger zone 3 (DHDN, Bessel) + DHHN2016 heights -> ETRS89 via the
 * BWTA2017 grid -> WGS84 (Helmert) -> UTM 32N.
 *
 *   gk3_to_utm32 <directory with BWTA2017.gsb and GCG2016.tif>
 *
 * Moved here from the former examples/EPSG_4326_to_32631.cpp. Fixed:
 * one context and one PJ for all coordinates (not one per coordinate),
 * error checks, PJ destroyed before its context.
 */

#include "proj.h"

#include <cmath>
#include <cstdio>

int main(int argc, char **argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s <grid directory>\n", argv[0]);
        return 2;
    }

    // One context per thread. Grid files are only searched in this directory.
    PJ_CONTEXT *ctx = proj_context_create();
    const char *searchPaths[] = {argv[1]};
    proj_context_set_search_paths(ctx, 1, searchPaths);

    // tmerc/utm always use Poder/Engsager in this build; +algo is optional.
    const char *pipeline =
        "+proj=pipeline "
        // GK zone 3 -> geographic (DHDN, Bessel)
        "+step +inv +proj=tmerc +lat_0=0 +lon_0=9 +k=1 +x_0=3500000 +y_0=0 "
        "+ellps=bessel +algo=poder_engsager "
        // BWTA2017 -> ETRS89 horizontal
        "+step +proj=hgridshift +grids=BWTA2017.gsb "
        // DHHN2016 -> ellipsoidal height (ETRS89)
        "+step +proj=vgridshift +grids=GCG2016.tif +multiplier=1 "
        "+step +proj=push +v_3 "
        // ETRS89 -> WGS84
        "+step +proj=cart +ellps=GRS80 "
        "+step +proj=helmert +x=-0.0471 +y=-0.0574 +z=0.001 +rx=0.0016 "
        "+ry=-0.00227 +rz=-0.01372 +s=0.02258 +convention=position_vector "
        "+step +inv +proj=cart +ellps=WGS84 "
        "+step +proj=pop +v_3 "
        // -> UTM 32N
        "+step +proj=utm +zone=32 +ellps=WGS84 +algo=poder_engsager";

    PJ *P = proj_create(ctx, pipeline);
    if (P == nullptr) {
        const int err = proj_context_errno(ctx);
        std::fprintf(stderr, "proj_create failed: %s\n",
                     proj_context_errno_string(ctx, err));
        proj_context_destroy(ctx);
        return 1;
    }

    const PJ_COORD gk = proj_coord(3556873.023590570, 5442147.78848171, 100.0,
                                   HUGE_VAL);
    const PJ_COORD utm = proj_trans(P, PJ_FWD, gk);
    const PJ_COORD back = proj_trans(P, PJ_INV, utm);
    if (proj_errno(P) != 0) {
        std::fprintf(stderr, "transformation failed: %s\n",
                     proj_errno_string(proj_errno(P)));
    }

    std::printf("GK3:   %.4f %.4f %.4f\n", gk.xyz.x, gk.xyz.y, gk.xyz.z);
    std::printf("UTM32: %.4f %.4f %.4f\n", utm.xyz.x, utm.xyz.y, utm.xyz.z);
    std::printf("back:  %.4f %.4f %.4f\n", back.xyz.x, back.xyz.y, back.xyz.z);

    proj_destroy(P);
    proj_context_destroy(ctx);
    return 0;
}
