/*
 * viscan fork of PROJ: this is NOT SQLite.
 *
 * In the math-only build this directory comes first in the include path of
 * the proj target, so that upstream sources that include <sqlite3.h> compile
 * without SQLite:
 *
 * - sqlite3_snprintf()/sqlite3_vsnprintf(), used for locale independent
 *   number formatting, are mapped to viscan_proj_*printf() (viscan_printf.cpp).
 *   They use their own names, so they never clash with a real SQLite that the
 *   application links.
 * - The types and the other functions below are only declared so that the
 *   inline helpers in sqlite3_utils.hpp compile. Nothing defines them. Any
 *   real use fails at link time, which the viscan tests check.
 */
#ifndef VISCAN_SQLITE3_STUB_H
#define VISCAN_SQLITE3_STUB_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef long long sqlite3_int64;
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
typedef struct sqlite3_vfs {
    int iVersion; /* never used; a complete type is needed by sqlite3_utils */
} sqlite3_vfs;

int sqlite3_finalize(sqlite3_stmt *);
int sqlite3_step(sqlite3_stmt *);
int sqlite3_reset(sqlite3_stmt *);
int sqlite3_bind_null(sqlite3_stmt *, int);
int sqlite3_bind_text(sqlite3_stmt *, int, const char *, int, void (*)(void *));
int sqlite3_bind_int64(sqlite3_stmt *, int, sqlite3_int64);
int sqlite3_bind_blob(sqlite3_stmt *, int, const void *, int,
                      void (*)(void *));
const unsigned char *sqlite3_column_text(sqlite3_stmt *, int);
sqlite3_int64 sqlite3_column_int64(sqlite3_stmt *, int);
int sqlite3_column_bytes(sqlite3_stmt *, int);
const void *sqlite3_column_blob(sqlite3_stmt *, int);

#define sqlite3_snprintf viscan_proj_snprintf
#define sqlite3_vsnprintf viscan_proj_vsnprintf

/* Same contract as sqlite3_snprintf(): always uses '.' as decimal separator
 * whatever the current locale, always zero-terminates, returns buf. */
char *viscan_proj_snprintf(int n, char *buf, const char *fmt, ...);
char *viscan_proj_vsnprintf(int n, char *buf, const char *fmt, va_list ap);

#ifdef __cplusplus
}
#endif

#endif /* VISCAN_SQLITE3_STUB_H */
