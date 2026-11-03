#ifndef PLATFORM_CODE_PAGE_TABLES_H
#define PLATFORM_CODE_PAGE_TABLES_H

#include <stddef.h>
#include <stdint.h>

/* A Windows code page as tables (code_page_tables.c, written by tools/code_pages.py). Bytes below
   0x80 are ASCII. single[b - 0x80] is the character of byte b alone, 0 when b is not one; a byte
   that is not one is a lead byte in a double-byte page, and double_byte[(lead - 0x81) * 191 +
   trail - 0x40] the character of the pair, 0 when the pair is not one. decode_only lists the codes
   that decode to a character which another code encodes. */
#define CODE_PAGE_LEAD_FIRST 0x81
#define CODE_PAGE_LEAD_LAST 0xfe
#define CODE_PAGE_TRAIL_FIRST 0x40
#define CODE_PAGE_TRAIL_LAST 0xfe
#define CODE_PAGE_TRAILS (CODE_PAGE_TRAIL_LAST - CODE_PAGE_TRAIL_FIRST + 1)

struct CodePageTable {
    unsigned code_page;
    const uint16_t *single;
    const uint16_t *double_byte;
    const uint16_t *decode_only;
    size_t decode_only_count;
};

extern const struct CodePageTable code_page_tables[];
extern const size_t code_page_table_count;

#endif
