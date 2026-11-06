/*
 * viscan fork of PROJ: replaces src/iso19111/factory.cpp in the math-only
 * build.
 *
 * There is no database (proj.db). DatabaseContext::create() throws
 * FactoryException, so no DatabaseContext and hence no AuthorityFactory can
 * exist. All callers inside PROJ handle that (EPSG:xxxx etc. are rejected,
 * PROJ strings and pipelines work). The other methods are unreachable and
 * only exist so that the library links.
 *
 * Only the methods referenced by the rest of the library or by the upstream
 * unit tests are defined. After merging a new upstream release, missing ones
 * show up as undefined symbols when linking viscan_math_only_test (it links
 * the whole archive) or the unit tests. Copy their signatures from
 * src/iso19111/factory.cpp.
 */

#define FROM_PROJ_CPP

#include "proj/common.hpp"
#include "proj/coordinateoperation.hpp"
#include "proj/coordinates.hpp"
#include "proj/crs.hpp"
#include "proj/datum.hpp"
#include "proj/io.hpp"
#include "proj/metadata.hpp"
#include "proj/util.hpp"

#include "proj_internal.h"

#include <memory>
#include <string>

using namespace NS_PROJ::common;

// Called by proj_cleanup().
void pj_clear_sqlite_cache() {}

NS_PROJ_START
namespace io {

namespace {
[[noreturn]] void throwNoDatabase() {
    throw FactoryException(
        "PROJ is built without database support (viscan math-only build)");
}
} // namespace

// ---------------------------------------------------------------------------
// Exceptions: real implementations (same as upstream).
// ---------------------------------------------------------------------------

FactoryException::FactoryException(const char *message) : Exception(message) {}
FactoryException::FactoryException(const std::string &message)
    : Exception(message) {}
FactoryException::~FactoryException() = default;
FactoryException::FactoryException(const FactoryException &) = default;

struct NoSuchAuthorityCodeException::Private {
    std::string authority_;
    std::string code_;

    Private(const std::string &authority, const std::string &code)
        : authority_(authority), code_(code) {}
};

NoSuchAuthorityCodeException::NoSuchAuthorityCodeException(
    const std::string &message, const std::string &authority,
    const std::string &code)
    : FactoryException(message), d(std::make_unique<Private>(authority, code)) {
}
NoSuchAuthorityCodeException::~NoSuchAuthorityCodeException() = default;
NoSuchAuthorityCodeException::NoSuchAuthorityCodeException(
    const NoSuchAuthorityCodeException &other)
    : FactoryException(other), d(std::make_unique<Private>(*(other.d))) {}
const std::string &NoSuchAuthorityCodeException::getAuthority() const {
    return d->authority_;
}
const std::string &NoSuchAuthorityCodeException::getAuthorityCode() const {
    return d->code_;
}

// ---------------------------------------------------------------------------
// DatabaseContext
// ---------------------------------------------------------------------------

DatabaseContextNNPtr
DatabaseContext::create(const std::string &, const std::vector<std::string> &,
                        PJ_CONTEXT *) {
    throwNoDatabase();
}

DatabaseContextNNPtr DatabaseContext::create(void *) { throwNoDatabase(); }

unsigned int DatabaseContext::getQueryCounter() const { throwNoDatabase(); }

std::string DatabaseContext::getAliasFromOfficialName(const std::string &,
                                                      const std::string &,
                                                      const std::string &) const {
    throwNoDatabase();
}

std::list<std::string>
DatabaseContext::getAliases(const std::string &, const std::string &,
                            const std::string &, const std::string &,
                            const std::string &) const {
    throwNoDatabase();
}

std::vector<std::string>
DatabaseContext::getAllowedAuthorities(const std::string &,
                                       const std::string &) const {
    throwNoDatabase();
}

std::set<std::string> DatabaseContext::getAuthorities() const {
    throwNoDatabase();
}

bool DatabaseContext::getAuthorityAndVersion(const std::string &,
                                             std::string &, std::string &) {
    throwNoDatabase();
}

std::vector<std::string> DatabaseContext::getDatabaseStructure() const {
    throwNoDatabase();
}

std::vector<std::string> DatabaseContext::getInsertStatementsFor(
    const common::IdentifiedObjectNNPtr &, const std::string &,
    const std::string &, bool, const std::vector<std::string> &) {
    throwNoDatabase();
}

const char *DatabaseContext::getMetadata(const char *) const {
    throwNoDatabase();
}

std::string DatabaseContext::getName(const std::string &, const std::string &,
                                     const std::string &) const {
    throwNoDatabase();
}

std::list<std::pair<std::string, std::string>>
DatabaseContext::getNonDeprecated(const std::string &, const std::string &,
                                  const std::string &) const {
    throwNoDatabase();
}

std::string DatabaseContext::getOldProjGridName(const std::string &) {
    throwNoDatabase();
}

const std::string &DatabaseContext::getPath() const { throwNoDatabase(); }

std::string DatabaseContext::getProjGridName(const std::string &) {
    throwNoDatabase();
}

std::string DatabaseContext::getTextDefinition(const std::string &,
                                               const std::string &,
                                               const std::string &) const {
    throwNoDatabase();
}

std::vector<operation::CoordinateOperationNNPtr>
DatabaseContext::getTransformationsForGridName(const DatabaseContextNNPtr &,
                                               const std::string &) {
    throwNoDatabase();
}

std::vector<std::string>
DatabaseContext::getVersionedAuthoritiesFromName(const std::string &) {
    throwNoDatabase();
}

bool DatabaseContext::getVersionedAuthority(const std::string &,
                                            const std::string &,
                                            std::string &) {
    throwNoDatabase();
}

bool DatabaseContext::isKnownName(const std::string &,
                                  const std::string &) const {
    throwNoDatabase();
}

bool DatabaseContext::lookForGridAlternative(const std::string &,
                                             std::string &, std::string &,
                                             bool &) const {
    throwNoDatabase();
}

bool DatabaseContext::lookForGridInfo(const std::string &, bool, std::string &,
                                      std::string &, std::string &, bool &,
                                      bool &, bool &) const {
    throwNoDatabase();
}

void DatabaseContext::startInsertStatementsSession() { throwNoDatabase(); }

void DatabaseContext::stopInsertStatementsSession() { throwNoDatabase(); }

std::string DatabaseContext::suggestsCodeFor(
    const common::IdentifiedObjectNNPtr &, const std::string &, bool) {
    throwNoDatabase();
}

bool DatabaseContext::toWGS84AutocorrectWrongValues(double &, double &,
                                                    double &, double &,
                                                    double &, double &,
                                                    double &) const {
    throwNoDatabase();
}

// ---------------------------------------------------------------------------
// AuthorityFactory
// ---------------------------------------------------------------------------

AuthorityFactoryNNPtr AuthorityFactory::create(const DatabaseContextNNPtr &,
                                               const std::string &) {
    throwNoDatabase();
}

const DatabaseContextNNPtr &AuthorityFactory::databaseContext() const {
    throwNoDatabase();
}

// Declared noexcept: must not throw. Unreachable anyway.
const std::string &AuthorityFactory::getAuthority() PROJ_PURE_DEFN {
    static const std::string empty;
    return empty;
}

std::vector<operation::CoordinateOperationNNPtr>
AuthorityFactory::createBetweenGeodeticCRSWithDatumBasedIntermediates(
    const crs::CRSNNPtr &, const std::string &, const std::string &,
    const crs::CRSNNPtr &, const std::string &, const std::string &, bool,
    bool, bool, bool, const std::vector<std::string> &,
    const metadata::ExtentPtr &, const metadata::ExtentPtr &, bool) const {
    throwNoDatabase();
}

util::BaseObjectNNPtr
AuthorityFactory::createObject(const std::string &) const {
    throwNoDatabase();
}

std::string AuthorityFactory::getDescriptionText(const std::string &) const {
    throwNoDatabase();
}

operation::ConversionNNPtr
AuthorityFactory::createConversion(const std::string &) const {
    throwNoDatabase();
}

crs::DerivedProjectedCRSNNPtr
AuthorityFactory::createDerivedProjectedCRS(const std::string &) const {
    throwNoDatabase();
}

datum::EngineeringDatumNNPtr
AuthorityFactory::createEngineeringDatum(const std::string &) const {
    throwNoDatabase();
}

crs::EngineeringCRSNNPtr
AuthorityFactory::createEngineeringCRS(const std::string &) const {
    throwNoDatabase();
}

std::vector<operation::CoordinateOperationNNPtr>
AuthorityFactory::createFromCoordinateReferenceSystemCodes(
    const std::string &, const std::string &) const {
    throwNoDatabase();
}

crs::CompoundCRSNNPtr
AuthorityFactory::createCompoundCRS(const std::string &) const {
    throwNoDatabase();
}

std::list<crs::CompoundCRSNNPtr>
AuthorityFactory::createCompoundCRSFromExisting(
    const crs::CompoundCRSNNPtr &) const {
    throwNoDatabase();
}

coordinates::CoordinateMetadataNNPtr
AuthorityFactory::createCoordinateMetadata(const std::string &) const {
    throwNoDatabase();
}

operation::CoordinateOperationNNPtr
AuthorityFactory::createCoordinateOperation(const std::string &, bool) const {
    throwNoDatabase();
}

crs::CRSNNPtr
AuthorityFactory::createCoordinateReferenceSystem(const std::string &) const {
    throwNoDatabase();
}

crs::CRSNNPtr
AuthorityFactory::createCoordinateReferenceSystem(const std::string &,
                                                  bool) const {
    throwNoDatabase();
}

cs::CoordinateSystemNNPtr
AuthorityFactory::createCoordinateSystem(const std::string &) const {
    throwNoDatabase();
}

datum::DatumNNPtr AuthorityFactory::createDatum(const std::string &) const {
    throwNoDatabase();
}

datum::DatumEnsembleNNPtr
AuthorityFactory::createDatumEnsemble(const std::string &,
                                      const std::string &) const {
    throwNoDatabase();
}

datum::EllipsoidNNPtr
AuthorityFactory::createEllipsoid(const std::string &) const {
    throwNoDatabase();
}

std::list<datum::EllipsoidNNPtr>
AuthorityFactory::createEllipsoidFromExisting(
    const datum::EllipsoidNNPtr &) const {
    throwNoDatabase();
}

metadata::ExtentNNPtr
AuthorityFactory::createExtent(const std::string &) const {
    throwNoDatabase();
}

std::vector<operation::CoordinateOperationNNPtr>
AuthorityFactory::createFromCRSCodesWithIntermediates(
    const std::string &, const std::string &, const std::string &,
    const std::string &, bool, bool, bool, bool,
    const std::vector<std::pair<std::string, std::string>> &, ObjectType,
    const std::vector<std::string> &, const metadata::ExtentPtr &,
    const metadata::ExtentPtr &, bool) const {
    throwNoDatabase();
}

std::vector<operation::CoordinateOperationNNPtr>
AuthorityFactory::createFromCoordinateReferenceSystemCodes(
    const std::string &, const std::string &, const std::string &,
    const std::string &, bool, bool, bool, bool, bool, bool,
    const metadata::ExtentPtr &, const metadata::ExtentPtr &) const {
    throwNoDatabase();
}

crs::GeodeticCRSNNPtr
AuthorityFactory::createGeodeticCRS(const std::string &) const {
    throwNoDatabase();
}

std::list<crs::GeodeticCRSNNPtr>
AuthorityFactory::createGeodeticCRSFromDatum(const std::string &,
                                             const std::string &,
                                             const std::string &) const {
    throwNoDatabase();
}

std::list<crs::GeodeticCRSNNPtr> AuthorityFactory::createGeodeticCRSFromDatum(
    const datum::GeodeticReferenceFrameNNPtr &, const std::string &,
    const std::string &) const {
    throwNoDatabase();
}

std::list<crs::GeodeticCRSNNPtr>
AuthorityFactory::createGeodeticCRSFromEllipsoid(const std::string &,
                                                 const std::string &,
                                                 const std::string &) const {
    throwNoDatabase();
}

datum::GeodeticReferenceFrameNNPtr
AuthorityFactory::createGeodeticDatum(const std::string &) const {
    throwNoDatabase();
}

crs::GeographicCRSNNPtr
AuthorityFactory::createGeographicCRS(const std::string &) const {
    throwNoDatabase();
}

std::list<common::IdentifiedObjectNNPtr>
AuthorityFactory::createObjectsFromName(const std::string &,
                                        const std::vector<ObjectType> &, bool,
                                        size_t) const {
    throwNoDatabase();
}

std::list<AuthorityFactory::PairObjectName>
AuthorityFactory::createObjectsFromNameEx(const std::string &,
                                          const std::vector<ObjectType> &,
                                          bool, size_t, bool) const {
    throwNoDatabase();
}

datum::PrimeMeridianNNPtr
AuthorityFactory::createPrimeMeridian(const std::string &) const {
    throwNoDatabase();
}

crs::ProjectedCRSNNPtr
AuthorityFactory::createProjectedCRS(const std::string &) const {
    throwNoDatabase();
}

std::list<crs::ProjectedCRSNNPtr>
AuthorityFactory::createProjectedCRSFromExisting(
    const crs::ProjectedCRSNNPtr &) const {
    throwNoDatabase();
}

UnitOfMeasureNNPtr
AuthorityFactory::createUnitOfMeasure(const std::string &) const {
    throwNoDatabase();
}

crs::VerticalCRSNNPtr
AuthorityFactory::createVerticalCRS(const std::string &) const {
    throwNoDatabase();
}

std::list<crs::VerticalCRSNNPtr>
AuthorityFactory::createVerticalCRSFromDatum(const std::string &,
                                             const std::string &) const {
    throwNoDatabase();
}

datum::VerticalReferenceFrameNNPtr
AuthorityFactory::createVerticalDatum(const std::string &) const {
    throwNoDatabase();
}

std::set<std::string>
AuthorityFactory::getAuthorityCodes(const ObjectType &, bool) const {
    throwNoDatabase();
}

std::list<AuthorityFactory::CRSInfo> AuthorityFactory::getCRSInfoList() const {
    throwNoDatabase();
}

std::list<AuthorityFactory::CelestialBodyInfo>
AuthorityFactory::getCelestialBodyList() const {
    throwNoDatabase();
}

std::list<std::string>
AuthorityFactory::getGeoidModels(const std::string &) const {
    throwNoDatabase();
}

std::string AuthorityFactory::getOfficialNameFromAlias(
    const std::string &, const std::string &, const std::string &, bool,
    std::string &, std::string &, std::string &) const {
    throwNoDatabase();
}

std::vector<operation::PointMotionOperationNNPtr>
AuthorityFactory::getPointMotionOperationsFor(const crs::GeodeticCRSNNPtr &,
                                              bool) const {
    throwNoDatabase();
}

std::vector<operation::CoordinateOperationNNPtr>
AuthorityFactory::getTransformationsForGeoid(const std::string &,
                                             bool) const {
    throwNoDatabase();
}

std::list<AuthorityFactory::UnitInfo> AuthorityFactory::getUnitList() const {
    throwNoDatabase();
}

std::string AuthorityFactory::identifyBodyFromSemiMajorAxis(double,
                                                            double) const {
    throwNoDatabase();
}

std::list<std::pair<std::string, std::string>>
AuthorityFactory::listAreaOfUseFromName(const std::string &, bool) const {
    throwNoDatabase();
}

} // namespace io
NS_PROJ_END
