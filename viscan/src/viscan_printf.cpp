/*
 * viscan fork of PROJ: locale independent snprintf, replacing
 * sqlite3_snprintf()/sqlite3_vsnprintf() in the math-only build
 * (see sqlite3_stub/sqlite3.h).
 *
 * PROJ uses them to write numbers into WKT, PROJJSON and PROJ strings, e.g.
 * when proj_create() normalises a pipeline. They must always write '.' as
 * decimal separator, also when the application has set e.g. a German locale.
 *
 * Fast path: the formats PROJ uses ("%d", "%.15g", "%.*g", "%lld", ...) are
 * formatted with std::to_chars, which is locale independent by definition
 * and produces exactly the printf output in the "C" locale. Floating point
 * std::to_chars needs VISCAN_HAVE_TO_CHARS_DOUBLE, set by CMake when the
 * toolchain/deployment target supports it (Apple: macOS 13.3, iOS 16.3).
 * Everything else uses the C library's printf with the "C" locale.
 */

#include "sqlite3.h" // the viscan stand-in

#include <charconv>
#include <cmath>
#include <cstring>
#include <locale.h>
#include <stdio.h>
#include <system_error>

#if defined(_WIN32)
#define VISCAN_PRINTF_MSVCRT
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <xlocale.h>
#define VISCAN_PRINTF_VSNPRINTF_L
#endif

namespace {

// ---------------------------------------------------------------------------
// Slow path: C library printf in the "C" locale.
// ---------------------------------------------------------------------------

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

void formatWithCLocale(size_t size, char *buf, const char *fmt, va_list ap) {
#if defined(VISCAN_PRINTF_MSVCRT)
    // _vsnprintf_l does not zero-terminate on truncation.
    const int ret = _vsnprintf_l(buf, size, fmt, cLocale(), ap);
    if (ret < 0 || static_cast<size_t>(ret) >= size) {
        buf[size - 1] = '\0';
    }
#elif defined(VISCAN_PRINTF_VSNPRINTF_L)
    vsnprintf_l(buf, size, cLocale(), fmt, ap);
#else
    // glibc, musl, bionic: no vsnprintf_l. uselocale() only affects the
    // calling thread.
    const locale_t old = uselocale(cLocale());
    vsnprintf(buf, size, fmt, ap);
    uselocale(old);
#endif
}

// ---------------------------------------------------------------------------
// Fast path
// ---------------------------------------------------------------------------

// Output buffer that truncates like snprintf.
struct Out {
    char *buf;
    size_t size; // including the terminating zero
    size_t len = 0;

    void put(const char *s, size_t n) {
        if (len + 1 < size) {
            const size_t room = size - 1 - len;
            std::memcpy(buf + len, s, n < room ? n : room);
        }
        len += n;
    }
    void finish() { buf[len < size ? len : size - 1] = '\0'; }
};

template <class T> bool putInteger(Out &out, T value) {
    char tmp[32];
    const auto res = std::to_chars(tmp, tmp + sizeof(tmp), value);
    if (res.ec != std::errc())
        return false;
    out.put(tmp, static_cast<size_t>(res.ptr - tmp));
    return true;
}

#ifdef VISCAN_HAVE_TO_CHARS_DOUBLE
bool putDouble(Out &out, double value, char conv, int precision) {
    // inf/nan: spelling differs between to_chars ("nan(snan)") and the
    // platform's printf; leave them to printf.
    if (!std::isfinite(value))
        return false;
    char tmp[400]; // enough for "%.17f" of DBL_MAX
    const std::chars_format fmt = conv == 'f'   ? std::chars_format::fixed
                                  : conv == 'e' ? std::chars_format::scientific
                                                : std::chars_format::general;
    const auto res = std::to_chars(tmp, tmp + sizeof(tmp), value, fmt,
                                   precision < 0 ? 6 : precision);
    if (res.ec != std::errc())
        return false;
    out.put(tmp, static_cast<size_t>(res.ptr - tmp));
    return true;
}
#endif

// Formats fmt if it only uses the plain conversions PROJ needs. Returns
// false (without having written anything usable) otherwise; the caller then
// uses the slow path with an untouched va_list.
bool formatFast(size_t size, char *buf, const char *fmt, va_list ap) {
    Out out{buf, size};
    const char *p = fmt;
    while (*p) {
        const char *pct = std::strchr(p, '%');
        if (pct == nullptr) {
            out.put(p, std::strlen(p));
            break;
        }
        out.put(p, static_cast<size_t>(pct - p));
        p = pct + 1;
        if (*p == '%') {
            out.put("%", 1);
            ++p;
            continue;
        }
        // No flags or width: anything like "%04X" goes to the slow path.
        int precision = -1;
        if (*p == '.') {
            ++p;
            if (*p == '*') {
                precision = va_arg(ap, int);
                ++p;
            } else {
                precision = 0;
                while (*p >= '0' && *p <= '9')
                    precision = precision * 10 + (*p++ - '0');
            }
        }
        int longs = 0;
        while (*p == 'l') {
            ++longs;
            ++p;
        }
        const char conv = *p++;
        bool ok = false;
        switch (conv) {
        case 'd':
        case 'i':
            if (precision >= 0)
                return false;
            ok = longs == 0   ? putInteger(out, va_arg(ap, int))
                 : longs == 1 ? putInteger(out, va_arg(ap, long))
                              : putInteger(out, va_arg(ap, long long));
            break;
        case 'u':
            if (precision >= 0)
                return false;
            ok = longs == 0   ? putInteger(out, va_arg(ap, unsigned))
                 : longs == 1 ? putInteger(out, va_arg(ap, unsigned long))
                              : putInteger(out, va_arg(ap, unsigned long long));
            break;
        case 's':
            if (precision >= 0 || longs)
                return false;
            {
                const char *s = va_arg(ap, const char *);
                if (s == nullptr)
                    return false;
                out.put(s, std::strlen(s));
            }
            ok = true;
            break;
#ifdef VISCAN_HAVE_TO_CHARS_DOUBLE
        case 'g':
        case 'e':
        case 'f':
            if (longs > 1)
                return false;
            ok = putDouble(out, va_arg(ap, double), conv, precision);
            break;
#endif
        default:
            return false;
        }
        if (!ok)
            return false;
    }
    out.finish();
    return true;
}

} // namespace

extern "C" char *viscan_proj_vsnprintf(int n, char *buf, const char *fmt,
                                       va_list ap) {
    if (n <= 0 || buf == nullptr) {
        return buf;
    }
    const size_t size = static_cast<size_t>(n);
    va_list apFast;
    va_copy(apFast, ap);
    const bool done = formatFast(size, buf, fmt, apFast);
    va_end(apFast);
    if (!done) {
        formatWithCLocale(size, buf, fmt, ap);
    }
    return buf;
}

extern "C" char *viscan_proj_snprintf(int n, char *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    viscan_proj_vsnprintf(n, buf, fmt, ap);
    va_end(ap);
    return buf;
}
