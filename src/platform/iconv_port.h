#ifndef PLATFORM_ICONV_PORT_H
#define PLATFORM_ICONV_PORT_H

/* iconv for the client's conversions between its code page and Unicode. Android's C library
   converts only between Unicode forms, so there (or with RAGE_OWN_ICONV) the client converts with
   its own tables of the Windows code pages (code_page_iconv.c); elsewhere it is the system's. */
#if defined(__ANDROID__) && !defined(RAGE_OWN_ICONV)
#define RAGE_OWN_ICONV 1
#endif

#ifdef RAGE_OWN_ICONV
#include <stddef.h>

typedef struct CodeConverter *rage_iconv_t;

/* Charsets: UTF-8, UTF-16LE, UTF-16BE and CP<n> for the tabled Windows code pages. Unknown names
   fail with EINVAL. */
rage_iconv_t rage_iconv_open(const char *to, const char *from);
size_t rage_iconv(rage_iconv_t converter, char **in, size_t *in_left, char **out, size_t *out_left);
int rage_iconv_close(rage_iconv_t converter);

#define iconv_t rage_iconv_t
#define iconv_open rage_iconv_open
#define iconv rage_iconv
#define iconv_close rage_iconv_close
#elif !defined(_WIN32)
#include <iconv.h>
#endif

#endif
