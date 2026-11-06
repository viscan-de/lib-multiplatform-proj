/*
 * viscan fork of PROJ: locale independent snprintf, replacing
 * sqlite3_snprintf()/sqlite3_vsnprintf() in the math-only build
 * (see sqlite3_stub/sqlite3.h).
 *
 * PROJ uses them to write numbers into WKT, PROJJSON and PROJ strings. They
 * must always write '.' as decimal separator, also when the application has
 * set e.g. a German locale (LC_NUMERIC=de_DE).
 */

#include "sqlite3.h" // the viscan stand-in

#include <locale.h>
#include <stdio.h>

#if defined(_WIN32)
#define VISCAN_PRINTF_MSVCRT
#elif defined(__APPLE__)
#include <xlocale.h>
#endif

namespace {

#ifdef VISCAN_PRINTF_MSVCRT
_locale_t cLocale() {
    static const _locale_t loc = _create_locale(LC_ALL, "C");
    return loc;
}
#else
locale_t cLocale() {
    static const locale_t loc = newlocale(LC_ALL_MASK, "C", nullptr);
    return loc;
}
#endif

} // namespace

extern "C" char *viscan_proj_vsnprintf(int n, char *buf, const char *fmt,
                                       va_list ap) {
    if (n <= 0 || buf == nullptr) {
        return buf;
    }
    const size_t size = static_cast<size_t>(n);
#ifdef VISCAN_PRINTF_MSVCRT
    // _vsnprintf_l does not zero-terminate on truncation.
    const int ret = _vsnprintf_l(buf, size, fmt, cLocale(), ap);
    if (ret < 0 || static_cast<size_t>(ret) >= size) {
        buf[size - 1] = '\0';
    }
#else
    // uselocale() only changes the locale of the calling thread.
    const locale_t old = uselocale(cLocale());
    vsnprintf(buf, size, fmt, ap);
    uselocale(old);
#endif
    return buf;
}

extern "C" char *viscan_proj_snprintf(int n, char *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    viscan_proj_vsnprintf(n, buf, fmt, ap);
    va_end(ap);
    return buf;
}
