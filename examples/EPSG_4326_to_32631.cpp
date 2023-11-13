/*******************************************************************************
    Example code that illustrates how to convert between two CRS

    Note: This file is in-lined in the documentation. Any changes must be
    reflected in docs/source/development/quickstart.rst

*******************************************************************************/

#include <cassert>
#include <cmath>   // for HUGE_VAL
#include <iomanip> // for std::setprecision()

#include <iostream>

#include "proj/coordinateoperation.hpp"
#include "proj/crs.hpp"
#include "proj/io.hpp"
#include "proj/util.hpp"// for nn_dynamic_pointer_cast

using namespace NS_PROJ::crs;
using namespace NS_PROJ::io;
using namespace NS_PROJ::operation;
using namespace NS_PROJ::util;

int main(void) {
////    auto dbContext = DatabaseContext::create("/home/pi/.vs/PROJ/data/proj.db");
////
////    // Instantiate a generic authority factory, that is not tied to a particular
////    // authority, to be able to get transformations registered by different
////    // authorities. This can only be used for CoordinateOperationContext.
////    auto authFactory = AuthorityFactory::create(dbContext, std::string());
////
////    // Create a coordinate operation context, that can be customized to ammend
////    // the way coordinate operations are computed. Here we ask for default
////    // settings.
////    auto coord_op_ctxt =
////        CoordinateOperationContext::create(authFactory, nullptr, 0.0);
////
////    // Instantiate a authority factory for EPSG related objects.
////    auto authFactoryEPSG = AuthorityFactory::create(dbContext, "EPSG");
////
////      //auto sourceCRS = NN_CHECK_THROW(nn_dynamic_pointer_cast<CRS>(
////      //  createFromUserInput("+proj=longlat +ellps=GRS80 "
////      //                      "+towgs84=-0.0471,-0.0574,0.0010,0.00160,-0.00227,-"
////      //                      "0.01372,0.02258 +no_defs +type=crs",
////      //                      dbContext)));
////
////
////
////    auto sourceCRS = authFactoryEPSG->createCoordinateReferenceSystem("9754"); 
////    //auto sourceCRS = authFactoryEPSG->createCoordinateReferenceSystem("31467");
////    //auto sourceCRS = NN_CHECK_THROW(nn_dynamic_pointer_cast<CRS>(createFromUserInput(
////    //        "WGS 84 (G1762)", dbContext)));
//////auto sourceCRS = NN_CHECK_THROW(nn_dynamic_pointer_cast<CRS>(createFromUserInput(
//////            " +proj=tmerc +lat_0=0 +lon_0=9 +k=1 +x_0=3500000 +y_0=0 "
//////            "+ellps=bessel +nadgrids=BWTA2017.gsb +units=m +no_defs +type=crs",
//////            dbContext)));
////    //auto sourceCRS = NN_CHECK_THROW(nn_dynamic_pointer_cast<CRS>(createFromUserInput("WGS 84 (G1762)", dbContext)));
////    //auto sourceCRS = NN_CHECK_THROW(nn_dynamic_pointer_cast<CRS>(
////    //    createFromUserInput("+proj=longlat +ellps=GRS80 +no_defs +type=crs",
////    //                        dbContext)));
////   
////
////    // Instantiate target CRS from PROJ.4 string (commented out, the equivalent
////    // from the EPSG code)
////    
////    //auto targetCRS = authFactoryEPSG->createCoordinateReferenceSystem("32632"); //25832
////
////    //auto targetCRS = NN_CHECK_THROW(nn_dynamic_pointer_cast<CRS>(
////    //   createFromUserInput("+proj=utm +zone=32 +ellps=GRS80 "
////    //                       "+towgs84=-0.0471,-0.0574,0.0010,0.00160,-0.00227,-"
////    //                       "0.01372,0.02258 +no_defs +type=crs",
////    //                       dbContext)));
////
////    auto targetCRS = NN_CHECK_THROW(nn_dynamic_pointer_cast<CRS>(
////        createFromUserInput("ETRS89/DREF91/2016 + DHHN2016",
////                            dbContext)));
////
////
////    // List operations available to transform from EPSG:4326
////    // (WGS 84 latitude/longitude) to EPSG:32631 (WGS 84 / UTM zone 31N).
////    auto list = CoordinateOperationFactory::create()->createOperations(
////        sourceCRS, targetCRS, coord_op_ctxt);
////
////    // Check that we got a non-empty list of operations
////    // The list is sorted from the most relevant to the less relevant one.
////    // Cf
////    // https://proj.org/operations/operations_computation.html#filtering-and-sorting-of-coordinate-operations
////    // for more details on the sorting of those operations.
////    // For a transformation between a projected CRS and its base CRS, like
////    // we do here, there will be only one operation.
////    assert(!list.empty());
//// 
////
////    auto a = JSONFormatter::create(dbContext);
////    std::string rules = list[0]->exportToJSON(a.get());
////    std::cout<<rules<<std::endl;
////
////    auto formatter = PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_5, nullptr);
////    auto projString = list[0]->exportToPROJString(formatter.get());
////
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
        "+step +proj=utm +zone=32 +ellps=WGS84 "; //Project to UTM32N
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
  

 
    //+proj=pipeline 
    //+step +inv +proj=tmerc +lat_0=0 +lon_0=9 +k=1 +x_0=3500000 +y_0=0 +ellps=bessel
    //+step +proj=hgridshift +grids=BWTA2017.gsb 
    //+step +proj=push +v_3 
    //+step +proj=cart +ellps=WGS84
    //+step +inv +proj=helmert +x=-0.0471 +y=-0.0574 +z=0.001 +rx=0.0016 +ry=-0.00227 +rz=-0.01372 +s=0.02258 +convention=position_vector 
    //+step +inv +proj=cart +ellps=GRS80 
    //+step +proj=pop +v_3 
    //+step +proj=utm +zone=32 +ellps=GRS80



    std::cout << projString << std::endl;

    //__________ NO DB CONTEXT REQUIRED ANYMORE __________

    PJ_CONTEXT *myctx = proj_context_create();
    PJ *T1 = proj_create(myctx, projString.c_str());

    //const auto &alt = T1->alternativeCoordinateOperations[0];
    //auto coordOperation =
    //    dynamic_cast<NS_PROJ::operation::CoordinateOperation *>(
    //        alt.pj->iso_obj.get());
    
   
    //auto T2 = pj_obj_create(myctx, coordOperation.);
    //auto transformer2 = coordOperation->coordinateTransformer(myctx);

    // Create an execution context (must only be used by one thread at a time)
    //PJ_CONTEXT *ctx = proj_context_create();


    // Create a coordinate transformer from the first operation of the list
    //auto transformer = list[0]->coordinateTransformer(ctx);
    /*
    PJ_COORD c = {{
        49, // latitude in degree
        2,  // longitude in degree
        0.0,       // z ordinate. unused
        HUGE_VAL   // time ordinate. unused
    }};
    */
   //  Perform the coordinate transformation.
    //PJ_COORD c = {{
    //      latitude in degree
    //    9.478435237935595836,  // longitude in degree
    //    48.893069368382505502,
    //    0,  // z ordinate. unused
    //    HUGE_VAL // time ordinate. unused
    //}};
    //

    //48.893069368382505502 
    //9.478435237935594060
    //48.893069368382505502 
    //9.478435237935595836


    //PJ_COORD c = {{
    //    535068.757, // latitude in degree
    //    5415679.245, // longitude in degree
    //    0,         // z ordinate. unused
    //    HUGE_VAL   // time ordinate. unused
    //}};

   PJ_COORD c = {{
         // latitude in degree
        3556873.023590570,          // longitude in degree
        5442147.78848171, 
        0, // z ordinate. unused
        HUGE_VAL     // time ordinate. unused
    }};
    

    //auto cOut = transformer->transform(c);
   std::cout << "TRANSFORMING" << std::endl;
   auto cOut = proj_trans(T1, PJ_DIRECTION::PJ_FWD, c);
   for (int i = 0; i < 1; ++i)
       {
       PJ_COORD c = {{
           // latitude in degree
           3556873.023590570+i, // longitude in degree
           5442147.78848171+i,
           0,       // z ordinate. unused
           HUGE_VAL // time ordinate. unused
       }};
    cOut = proj_trans(T1, PJ_DIRECTION::PJ_FWD, c);
   }
    //auto c2new = transformer2->transform(c2);

    // Display result
    std::cout << std::fixed << std::setprecision(18);
    std::cout << "Easting: " << cOut.v[0] << std::endl;  // should be 426857.988
    std::cout << "Northing: " << cOut.v[1]
              << std::endl;                           // should be 5427937.523
    std::cout << "Altitude: " << cOut.v[2]
              << std::endl; // should be 5427937.523


    // Destroy execution context
    proj_context_destroy(myctx);

    return 0;
}
