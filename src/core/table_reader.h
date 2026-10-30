#ifndef CORE_TABLE_READER_H
#define CORE_TABLE_READER_H

#include <stddef.h>

enum TableReadMode {
    TABLE_FIELDS,
    TABLE_FIELDS_AND_LINES,
    TABLE_RECORDS
};

struct TableBlock;

struct TableReader {
    char **fields;
    size_t count;
    size_t capacity;
    struct TableBlock *blocks;
};

unsigned char table_reader_read(struct TableReader *reader, const char *name,
                                enum TableReadMode mode);

void table_reader_release(struct TableReader *reader);

#endif
