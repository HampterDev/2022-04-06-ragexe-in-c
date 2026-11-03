#ifndef CORE_UTF8_H
#define CORE_UTF8_H

#include <stddef.h>
#include <wchar.h>

/* UTF-8 is the client's only in-memory text encoding.

   A character is a well-formed sequence of 1-4 bytes. Every other byte (a stray
   continuation byte, an overlong form, a surrogate, a value above U+10FFFF or a
   truncated sequence) is a one-byte character of its own. Stepping therefore never
   splits a valid character, never swallows the character after an invalid byte, and
   never reads past the end: bounded functions stop at text + length, the unbounded
   ones at the NUL terminator. */

#define UTF8_CHAR_MAX 4
#define UTF8_INVALID (-1L)
#define UTF8_REPLACEMENT 0xfffdUL

/* Bytes of the character at text (1-4), or 0 when length is 0. */
size_t utf8_char_size(const char *text, size_t length);

/* Code point of the character at text, or UTF8_INVALID for an invalid byte. *size, if
   given, receives utf8_char_size(text, length). */
long utf8_decode(const char *text, size_t length, size_t *size);

/* The character after text in a NUL-terminated string; text itself at the terminator. */
const char *utf8_next(const char *text);

/* The character after text, never past end; end when text >= end. */
const char *utf8_step(const char *text, const char *end);

/* The start of the character that ends at text, which should be a character boundary:
   the lead byte of a well-formed sequence ending there, otherwise text - 1. Never
   before start; start when text <= start. */
const char *utf8_prev(const char *start, const char *text);

/* Nonzero if every byte of text belongs to a well-formed sequence. */
int utf8_valid(const char *text, size_t length);

/* The longest prefix of text, at most max_bytes long, that ends on a character
   boundary. */
size_t utf8_prefix(const char *text, size_t length, size_t max_bytes);

/* Copies the longest prefix of text that ends on a character boundary and fits in
   size - 1 bytes, and fills the rest of out with NULs, as a fixed packet field wants:
   a whole-character strncpy that always terminates. Returns the bytes copied; writes
   nothing when size is 0. */
size_t utf8_copy_prefix(char *out, size_t size, const char *text);

/* Display columns: 2 for East Asian wide and fullwidth characters, 1 for every other
   character and invalid byte. */
size_t utf8_columns(const char *text, size_t length);

/* Writes code_point (U+0000-U+10FFFF, not a surrogate) to out and returns its length
   1-4; returns 0 and writes nothing for any other value. */
size_t utf8_encode(unsigned long code_point, char out[UTF8_CHAR_MAX]);

/* Converts count UTF-16 units to NUL-terminated UTF-8, joining surrogate pairs and
   writing U+FFFD for unpaired surrogates. Stops before a character that would not fit
   in capacity - 1 bytes. Returns the bytes written, without the terminator; writes
   nothing when capacity is 0. */
size_t utf8_from_utf16(char *out, size_t capacity, const wchar_t *text, size_t count);

/* Feeds one UTF-16 unit, as WM_CHAR delivers them, and writes the completed code
   points to out (0-2 of them). A high surrogate waits in *pending for its low
   surrogate; an unpaired surrogate becomes U+FFFD. */
size_t utf16_feed(wchar_t *pending, wchar_t unit, unsigned long out[2]);

#endif
