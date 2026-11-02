#include "platform/language_api.h"
#include "iteminfo/item_table_internal.h"
#include "iteminfo/item_tables_internal.h"
#include "core/table_reader.h"
#include "session/state.h"
#include "core/memory.h"
#include "iteminfo/iteminfo.h"

#include <string.h>
#include "ui/ui_std.h"
#include "platform/window.h"

static struct TableReader item_table_reader;

static void item_table_reader_destroy(void)
{
    table_reader_release(&item_table_reader);
}

void table_reader_init(void)
{
    atexit(item_table_reader_destroy);
}

static int item_id_digit(unsigned char c)
{
    return c >= '0' && c <= '9';
}

unsigned char table_token_is_number(const char *text)
{
    const unsigned char *p = (const unsigned char *)text;

    for (; *p; p++)
        if (!item_id_digit(*p))
            return 0;
    return 1;
}

unsigned char read_int_file(struct Vec *out, const char *name)
{
    struct File file;
    struct Vec shrunk;
    int size, i;
    char *text, *start;

    out->last = out->first;
    file_construct(&file);
    if (!file_open(&file, name, 0, 1, 0)) {
        platform_message_box(NULL, "Can't open file", "ErrorMessage", MB_OK);
        file_destruct(&file);
        return 0;
    }
    size = file.size;
    if (size < 0) {
        file_destruct(&file);
        return 0;
    }
    text = mem_alloc((size_t)size + 1);
    if (!file_read(&file, text, size)) {
        mem_free(text);
        file_destruct(&file);
        return 0;
    }
    text[size] = 0;
    file_close(&file);

    start = text;
    for (i = 0; i < size; i++) {
        if (text[i] == '/' && text[i + 1] == '/') {
            while (text[i] != '\r' && i < size && text[++i] != '\n') {}
            start = text + i + 1;
        }
        if (text[i] == '\t' || text[i] == ' ') {
            int value;

            text[i] = 0;
            value = atoi(start);
            vec_push(out, &value, sizeof value);
            start = text + i + 1;
        } else if (text[i] == '\n' || text[i] == '\r') {
            start = text + i + 1;
        }
    }
    mem_free(text);

    vec_copy(&shrunk, out, sizeof(int));
    if (&shrunk != out) {
        struct Vec t = shrunk;
        shrunk = *out;
        *out = t;
    }
    vec_free(&shrunk);
    file_destruct(&file);
    return 1;
}

void item_tables_load(void)
{
    struct TableReader *tokens = &item_table_reader;
    struct Vec moveInfo;
    struct CardAffix *affix;
    int itemId;

    item_table_clear();
    card_affix_clear();
    metal_process_clear();
    table_reader_release(&item_table_reader);

    item_table_apply_lua();

    table_reader_read(tokens, "CardItemNameTable.txt", TABLE_FIELDS);
    for (size_t index = 0; index < tokens->count; index++) {
        char **p = tokens->fields + index;
        card_affix_at(atoi(p[0]));
    }
    table_reader_read(tokens, "Num2CardIllustNameTable.txt", TABLE_FIELDS);
    for (size_t index = 0; index + 2 <= tokens->count; index += 2) {
        char **p = tokens->fields + index;
        card_affix_at(atoi(p[0]))->illustName = p[1];
    }
    table_reader_read(tokens, "CardPrefixNameTable.txt", TABLE_FIELDS);
    for (size_t index = 0; index + 2 <= tokens->count; index += 2) {
        char **p = tokens->fields + index;
        card_affix_at(atoi(p[0]))->prefix = p[1];
    }
    table_reader_read(tokens, "CardPostfixNameTable.txt", TABLE_FIELDS);
    for (size_t index = 0; index < tokens->count; index++) {
        char **p = tokens->fields + index;
        affix = card_affix_at(atoi(p[0]));
        affix->postfix = affix->prefix;
        affix->prefix = "";
    }

    itemId = 0;
    table_reader_read(tokens, "MetalProcessItemList.txt", TABLE_FIELDS_AND_LINES);
    for (size_t index = 0; index < tokens->count; index++) {
        char **p = tokens->fields + index;
        char *line = *p;

        if (!line || !*line)
            continue;
        if (table_token_is_number(line))
            itemId = atoi(line);
        else if (strlen(line) != 0)
            metal_process_append(itemId, line);
    }

    item_table_set_unknown();
    card_affix_set_unknown();

    moveInfo.first = moveInfo.last = moveInfo.end = NULL;
    if (read_int_file(&moveInfo, "ItemMoveInfoV5.txt") && moveInfo.first != moveInfo.last) {
        size_t records = vec_count(&moveInfo, sizeof(int)) / 9;

        for (size_t index = 0; index < records; index++) {
            const int *line = (const int *)moveInfo.first + index * 9;
            item_move_info_add(line[0], line + 1);
        }
    }
    vec_free(&moveInfo);
}
