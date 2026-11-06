/*
 * viscan fork of PROJ: replaces src/networkfilemanager.cpp in the math-only
 * build.
 *
 * Network access (CDN grids, remote files) and the SQLite based grid chunk
 * cache are not available. The public C API keeps working and reports that
 * the network is disabled. Nothing throws through the C API.
 *
 * proj_context_is_network_enabled() always returns FALSE, so PROJ_NETWORK=ON
 * and "network = on" in proj.ini have no effect. All network code paths in
 * PROJ check that function first.
 */

#define FROM_PROJ_CPP

#include "filemanager.hpp"
#include "proj.h"
#include "proj_internal.h"

#include <memory>
#include <string>

namespace {
const char *const kNoNetwork =
    "network access is not available in this build (viscan math-only)";
} // namespace

NS_PROJ_START

std::unique_ptr<File> pj_network_file_open(PJ_CONTEXT *ctx,
                                           const char *filename) {
    pj_log(ctx, PJ_LOG_ERROR, "%s: %s", filename, kNoNetwork);
    return nullptr;
}

void FileManager::fillDefaultNetworkInterface(PJ_CONTEXT *) {}

void FileManager::clearMemoryCache() {}

NS_PROJ_END

std::string pj_context_get_grid_cache_filename(PJ_CONTEXT *) {
    return std::string();
}

int proj_context_set_network_callbacks(
    PJ_CONTEXT *ctx, proj_network_open_cbk_type,
    proj_network_close_cbk_type,
    proj_network_get_header_value_cbk_type,
    proj_network_read_range_type, void *) {
    pj_log(ctx, PJ_LOG_ERROR, "%s", kNoNetwork);
    return FALSE;
}

int proj_context_set_enable_network(PJ_CONTEXT *ctx, int enable) {
    if (enable) {
        pj_log(ctx, PJ_LOG_ERROR, "%s", kNoNetwork);
    }
    return FALSE;
}

int proj_context_is_network_enabled(PJ_CONTEXT *) { return FALSE; }

void proj_context_set_url_endpoint(PJ_CONTEXT *, const char *) {}

void proj_grid_cache_set_enable(PJ_CONTEXT *, int) {}

void proj_grid_cache_set_filename(PJ_CONTEXT *, const char *) {}

void proj_grid_cache_set_max_size(PJ_CONTEXT *, int) {}

void proj_grid_cache_set_ttl(PJ_CONTEXT *, int) {}

void proj_grid_cache_clear(PJ_CONTEXT *) {}

int proj_is_download_needed(PJ_CONTEXT *, const char *, int) { return FALSE; }

int proj_download_file(PJ_CONTEXT *ctx, const char *url_or_filename, int,
                       int (*)(PJ_CONTEXT *, double, void *), void *) {
    pj_log(ctx, PJ_LOG_ERROR, "%s: %s", url_or_filename, kNoNetwork);
    return FALSE;
}
