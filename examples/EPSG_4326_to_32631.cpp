/*******************************************************************************
    Example code that illustrates how to convert between two CRS

    Note: This file is in-lined in the documentation. Any changes must be
    reflected in docs/source/development/quickstart.rst

*******************************************************************************/

#include <cassert>
#include <cmath>   // for HUGE_VAL
#include <iomanip> // for std::setprecision()
#include <iostream>
#include <string>

#include "proj/coordinateoperation.hpp"
using namespace NS_PROJ::operation;

PJ_COORD transformFwd(const std::string &pipelineDefinition,
                      const PJ_COORD &inputCoord) {

    const std::string searchPath = "/home/pi/";
    const auto searchPaths = searchPath.c_str();
    PJ_CONTEXT *myctx = proj_context_create();
    proj_context_is_network_enabled(myctx);
    proj_context_set_search_paths(myctx, 1, &searchPaths);
    PJ *T1 = proj_create(myctx, pipelineDefinition.c_str());

    auto result = proj_trans(T1, PJ_DIRECTION::PJ_FWD, inputCoord);

    // Destroy execution context
    proj_context_destroy(myctx);
    return result;
}

PJ_COORD transformRev(const std::string &pipelineDefinition,
                      const PJ_COORD &inputCoord) {

    const std::string searchPath = "/home/pi/";
    const auto searchPaths = searchPath.c_str();
    PJ_CONTEXT *myctx = proj_context_create();
    proj_context_is_network_enabled(myctx);
    proj_context_set_search_paths(myctx, 1, &searchPaths);
    PJ *T1 = proj_create(myctx, pipelineDefinition.c_str());

    auto result = proj_trans(T1, PJ_DIRECTION::PJ_INV, inputCoord);

    // Destroy execution context
    proj_context_destroy(myctx);
    return result;
}

int main(void) {
    std::string projString =
    
        "+proj=pipeline "
        "+step +inv +proj=tmerc +lat_0=0 +lon_0=9 +k=1 +x_0=3500000 +y_0=0 +ellps=bessel " //GK Z3 --> Ellipsoidal System
        "+step +proj=hgridshift +grids=BWTA2017.gsb " //BWTA2017 corrections --> ETRS89 horizontal
        "+step +proj=vgridshift +grids=GCG2016.tif +multiplier=1 " //DHHN2016 --> ellipsoidal heights (ETRS89 vertical)
        "+step +proj=push +v_3 " //Save Z
        "+step +proj=cart +ellps=GRS80 " //Go to cartesic system assuming GRS80 Ellipsoidal
        "+step +proj=helmert +x=-0.0471 +y=-0.0574 +z=0.001 +rx=0.0016 +ry=-0.00227 +rz=-0.01372 +s=0.02258 +convention=position_vector " //applying ETRS89-->WGS84 Shift
        "+step +inv +proj=cart +ellps=WGS84 " //Go back to ellipsoidal system of WGS84
        "+step +proj=pop +v_3 " //Restoring Z (ellipsoidal height)
        "+step +proj=utm +zone=32 +ellps=WGS84"; //Project to UTM32N
        //"+step +inv +proj=utm +zone=32 +ellps=WGS84 "     // Project to UTM32N
        //"+step +inv +proj=pop +v_3 " // Restoring Z (ellipsoidal height)
        //"+step +proj=cart +ellps=WGS84 " // Go back to ellipsoidal system of
        //"+step +inv +proj=helmert +x=-0.0471 +y=-0.0574 +z=0.001 +rx=0.0016 +ry=-0.00227 +rz=-0.01372 +s=0.02258 +convention=position_vector " //applying ETRS89-->WGS84 Shift
        //"+step +inv +proj=cart +ellps=GRS80 " // Go to cartesic system assuming GRS80
        //"+step +inv +proj=push +v_3 "         // Save Z
        //"+step +inv +proj=vgridshift +grids=GCG2016.tif +multiplier=1 " //DHHN2016 --> ellipsoidal heights (ETRS89 vertical)
        //"+step +inv +proj=hgridshift +grids=BWTA2017.gsb " //BWTA2017 corrections --> ETRS89 horizontal
        //"+step +proj=tmerc +lat_0=0 +lon_0=9 +k=1 +x_0=3500000 +y_0=0 +ellps=bessel"; //GK Z3 --> Ellipsoidal System
                                     //  WGS84

    
    //        "+step +proj=axisswap +order=2,1 "
    //        "+step +proj=unitconvert +xy_in=deg +xy_out=rad "
    //        "+step +proj=unitconvert +xy_in=rad +xy_out=deg "
    //        "+step +proj=axisswap +order=2,1 "
  

 /*
    "+proj=pipeline "
    "+step +inv +proj=tmerc +lat_0=0 +lon_0=9 +k=1 +x_0=3500000 +y_0=0 +ellps=bessel "
    "+step +proj=hgridshift +grids=BWTA2017.gsb "
    "+step +proj=push +v_3 "
    "+step +proj=cart +ellps=WGS84 "
    "+step +inv +proj=helmert +x=-0.0471 +y=-0.0574 +z=0.001 +rx=0.0016 +ry=-0.00227 +rz=-0.01372 +s=0.02258 +convention=position_vector "
    "+step +inv +proj=cart +ellps=GRS80 "
    "+step +proj=pop +v_3 "
    "+step +proj=utm +zone=32 +ellps=GRS80 ";
       */

    /*
    PJ_COORD c = {{
        49, // latitude in degree
        2,  // longitude in degree
        0.0,       // z ordinate. unused
        HUGE_VAL   // time ordinate. unused
    }};
    */
    //  Perform the coordinate transformation.
    // PJ_COORD c = {{
    //      latitude in degree
    //    9.478435237935595836,  // longitude in degree
    //    48.893069368382505502,
    //    0,  // z ordinate. unused
    //    HUGE_VAL // time ordinate. unused
    //}};
    //

    // 48.893069368382505502
    // 9.478435237935594060
    // 48.893069368382505502
    // 9.478435237935595836

    // PJ_COORD c = {{
    //     535068.757, // latitude in degree
    //     5415679.245, // longitude in degree
    //     0,         // z ordinate. unused
    //     HUGE_VAL   // time ordinate. unused
    // }};

    PJ_COORD c = {{
        // latitude in degree
        3556873.023590570, // longitude in degree
        5442147.78848171,
        100,       // z ordinate. unused
        HUGE_VAL // time ordinate. unused
    }};

    std::cout << projString << std::endl;


    auto cOut = transformFwd(projString, c);

    auto cOut2 = transformRev(projString, cOut);
 
    // Display result
    std::cout << std::fixed << std::setprecision(18);

    std::cout << "Easting: " << c.v[0] << std::endl;
    std::cout << "Northing: " << c.v[1] << std::endl;
    std::cout << "Altitude: " << c.v[2] << std::endl;


    std::cout << "Easting: " << cOut.v[0] << std::endl;  // should be 426857.988
    std::cout << "Northing: " << cOut.v[1]
              << std::endl;                           // should be 5427937.523
    std::cout << "Altitude: " << cOut.v[2]
              << std::endl; // should be 5427937.523

    std::cout << "Easting: " << cOut2.v[0] << std::endl;
    std::cout << "Northing: " << cOut2.v[1]
              << std::endl;
    std::cout << "Altitude: " << cOut2.v[2]
              << std::endl;




    return 0;
}



