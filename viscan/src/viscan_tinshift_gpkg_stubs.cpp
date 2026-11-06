/*
 * viscan fork of PROJ: replaces src/transformations/tinshift_gpkg.cpp and
 * src/sqlite3_utils.cpp in the math-only build.
 *
 * TIN models stored in GeoPackage files (+proj=tinshift +file=*.gpkg) need
 * SQLite and are not supported. TIN models in JSON files are not affected.
 */

#define FROM_PROJ_CPP

#include "transformations/tinshift_gpkg.hpp"

const char *TINShiftGeopackageException::what() const noexcept {
    return msg_.c_str();
}

std::unique_ptr<TINShiftGeopackageFile>
TINShiftGeopackageFile::open(PJ_CONTEXT *, const std::string &) {
    throw TINShiftGeopackageException(
        "GeoPackage TIN files are not supported in this build (no SQLite)");
}

TINShiftGeopackageFile::~TINShiftGeopackageFile() = default;

TINShiftGeopackageEvaluator::TINShiftGeopackageEvaluator(
    std::unique_ptr<TINShiftGeopackageFile> fileIn)
    : file_(std::move(fileIn)) {}

bool TINShiftGeopackageEvaluator::forward(double, double, double, double &,
                                          double &, double &) {
    return false;
}

bool TINShiftGeopackageEvaluator::inverse(double, double, double, double &,
                                          double &, double &) {
    return false;
}

NS_PROJ_START
// Needed by the TINShiftGeopackageFile destructor. Never actually created.
SQLite3VFS::~SQLite3VFS() = default;
NS_PROJ_END
