#include "core/table_reader.h"
#include "core/client.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct TableBlock {
    struct TableBlock *next;
    char text[];
};

static void table_allocation_failed(void)
{
    fputs("Unable to allocate resource table memory.\n", stderr);
    abort();
}

static void append_field(struct TableReader *reader, char *field)
{
    if (reader->count == reader->capacity) {
        size_t limit = SIZE_MAX / sizeof *reader->fields;
        size_t capacity = reader->capacity ? reader->capacity : 64;
        char **fields;
        if (reader->count == limit)
            table_allocation_failed();
        capacity = capacity > limit / 2 ? limit : capacity * 2;
        fields = realloc(reader->fields, capacity * sizeof *fields);
        if (!fields)
            table_allocation_failed();
        reader->fields = fields;
        reader->capacity = capacity;
    }
    reader->fields[reader->count++] = field;
}

void table_reader_release(struct TableReader *reader)
{
    struct TableBlock *block = reader->blocks;
    while (block) {
        struct TableBlock *next = block->next;
        free(block);
        block = next;
    }
    free(reader->fields);
    *reader = (struct TableReader){0};
}

unsigned char table_reader_read(struct TableReader *reader, const char *name,
                                enum TableReadMode mode)
{
    struct File file;
    struct TableBlock *block;
    char *text, *field;
    int size, i;

    reader->count = 0;
    file_construct(&file);
    if (g_language)
        name = language_language_file(g_language, name);
    if (!file_open(&file, name, 0, 1, 0)) {
        file_destruct(&file);
        return 0;
    }

    size = file.size;
    if (size < 0 || (size_t)size > SIZE_MAX - sizeof *block - 1) {
        file_destruct(&file);
        return 0;
    }
    block = malloc(sizeof *block + (size_t)size + 1);
    if (!block)
        table_allocation_failed();
    text = block->text;
    if (!file_read(&file, text, size)) {
        free(block);
        file_destruct(&file);
        return 0;
    }
    text[size] = 0;
    file_close(&file);
    file_destruct(&file);

    field = text;
    for (i = 0; i < size; i++) {
        switch (text[i]) {
        case '\n':
            field = text + i + 1;
            break;
        case '\r':
            text[i] = 0;
            if (*field && mode != TABLE_FIELDS && strncmp(field, "//", 2) != 0)
                append_field(reader, field);
            field = text + i + 1;
            break;
        case '#':
            if (strncmp(field, "//", 2) == 0)
                break;
            text[i] = 0;
            append_field(reader, field);
            if (mode == TABLE_RECORDS)
                append_field(reader, NULL);
            field = text + i + 1;
            break;
        }
    }
    block->next = reader->blocks;
    reader->blocks = block;
    return 1;
}
