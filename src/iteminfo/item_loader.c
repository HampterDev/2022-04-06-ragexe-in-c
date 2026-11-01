#include "platform/error_message.h"
#include "platform/language_api.h"
#include "iteminfo/item_table_internal.h"
#include "iteminfo/item_tables_internal.h"
#include "core/table_reader.h"
#include "session/state.h"
#include "core/memory.h"
#include "iteminfo/iteminfo.h"

#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "core/msg_str.h"
#include "session/session_names.h"
#include "ui/ui_std.h"

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

static const char ITEM_SCRIPT_ERROR[] =
    "item.txt 스크립트를 확인해주세요.";

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
    int size;
    char *text, *cursor, *end;

    out->last = out->first;
    file_construct(&file);
    if (g_language)
        name = language_language_file(g_language, name);
    if (!file_open(&file, name, 0, 1, 0)) {
        MessageBoxA(NULL, "Can't open file", "ErrorMessage", MB_OK);
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

    cursor = text;
    end = text + size;
    while (cursor < end) {
        char *next;
        long parsed;
        int value;

        if (isspace((unsigned char)*cursor)) {
            ++cursor;
            continue;
        }
        if (*cursor == '/' && cursor + 1 < end && cursor[1] == '/') {
            while (cursor < end && *cursor != '\r' && *cursor != '\n')
                ++cursor;
            continue;
        }

        errno = 0;
        parsed = strtol(cursor, &next, 10);
        if (next == cursor || errno == ERANGE || parsed < INT_MIN || parsed > INT_MAX
            || (next < end && !isspace((unsigned char)*next)
                && !(*next == '/' && next + 1 < end && next[1] == '/'))) {
            out->last = out->first;
            mem_free(text);
            file_destruct(&file);
            return 0;
        }
        value = (int)parsed;
        vec_push(out, &value, sizeof value);
        cursor = next;
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

static void load_item_file(struct TableReader *tokens, const char *name)
{
    char **p;

    table_reader_read(tokens, name, TABLE_RECORDS);
    if (!tokens->count)
        return;
    p = tokens->fields;
    do {
        struct ItemTableEntry *entry = item_table_at(atoi(p[0]));

        p += 2;
        if (p >= (tokens->fields + tokens->count))
            error_msg(ITEM_SCRIPT_ERROR, 0);
        if (*p)
            session_remove_underbar(*p, *p);
        entry->name = *p;

        p += 2;
        if (p >= (tokens->fields + tokens->count))
            error_msg(ITEM_SCRIPT_ERROR, 0);
        if (*p) {
            do {
                item_table_append_description(entry, 1, *p);
            } while (*++p);
        }
        p++;
        if (p >= (tokens->fields + tokens->count))
            error_msg(ITEM_SCRIPT_ERROR, 0);

        if (*p && !strstr(*p, "idenitfied equal unidentified")) {
            session_remove_underbar(*p, *p);
            entry->unidName = *p;
            p += 2;
            if (p >= (tokens->fields + tokens->count))
                error_msg(ITEM_SCRIPT_ERROR, 0);
            if (*p) {
                do {
                    item_table_append_description(entry, 0, *p);
                } while (*++p);
            }
        } else {
            item_table_use_identified_description(entry);
            p++;
        }
        p++;
    } while (p < (tokens->fields + tokens->count));
}

static void load_display_names(struct TableReader *tokens, const char *name, int identified)
{
    if (!file_make_file_name(name, 0))
        return;
    table_reader_read(tokens, name, TABLE_FIELDS);
    for (size_t i = 0; i + 2 <= tokens->count; i += 2) {
        char **p = tokens->fields + i;
        struct ItemTableEntry *entry = item_table_at(atoi(p[0]));

        session_remove_underbar(p[1], p[1]);
        if (identified)
            entry->name = p[1];
        else
            entry->unidName = p[1];
    }
}

static void load_descriptions(struct TableReader *tokens, const char *name, int identified)
{
    int itemId;

    if (!file_make_file_name(name, 0))
        return;
    itemId = 0;
    table_reader_read(tokens, name, TABLE_FIELDS_AND_LINES);
    for (size_t i = 0; i < tokens->count; i++) {
        char **p = tokens->fields + i;
        char *line = *p;
        if (!line || !*line)
            continue;
        if (table_token_is_number(line)) {
            itemId = atoi(line);
            continue;
        }
        text_decode(line);
        item_table_append_description(item_table_at(itemId), identified, *p);
    }
}

static void load_res_names(struct TableReader *tokens, const char *name, int identified)
{
    if (!file_make_file_name(name, 0))
        return;
    table_reader_read(tokens, name, TABLE_FIELDS);
    for (size_t i = 0; i + 2 <= tokens->count; i += 2) {
        char **p = tokens->fields + i;
        if (identified)
            item_table_at(atoi(p[0]))->resName = p[1];
        else
            item_table_at(atoi(p[0]))->unidResName = p[1];
    }
}

void item_tables_load(void)
{
    static const char *const itemFiles[2] = { "item.txt", "eventItem.txt" };
    struct TableReader *tokens = &item_table_reader;
    struct Vec moveInfo;
    struct ItemTableEntry *entry;
    struct CardAffix *affix;
    unsigned char haveItemFile = 0;
    int i, itemId;

    item_table_clear();
    card_affix_clear();
    metal_process_clear();
    book_items_clear();
    table_reader_release(&item_table_reader);

    for (i = 0; i < 2; i++) {
        if (!file_make_file_name(itemFiles[i], 0))
            continue;
        load_item_file(tokens, itemFiles[i]);
        haveItemFile = 1;
    }

    if (!haveItemFile) {
        load_display_names(tokens, "EventNum2ItemDisplayNameTable.txt", 0);
        load_descriptions(tokens, "EventNum2itemDescTable.txt", 0);
        load_display_names(tokens, "EventIdNum2ItemDisplayNameTable.txt", 1);
        load_descriptions(tokens, "EventIdNum2itemDescTable.txt", 1);
        item_table_apply_lua();
    }
    load_res_names(tokens, "EventNum2ItemResNameTable.txt", 0);
    load_res_names(tokens, "EventIdNum2ItemResNameTable.txt", 1);

    table_reader_read(tokens, "itemSlotTable.txt", TABLE_FIELDS);
    for (size_t index = 0; index + 2 <= tokens->count; index += 2) {
        char **p = tokens->fields + index;
        item_table_at(atoi(p[0]))->slotTableValue = atoi(p[1]);
    }

    table_reader_read(tokens, "itemParamTable.txt", TABLE_FIELDS);
    for (size_t index = 0; index + 3 <= tokens->count; index += 3) {
        char **p = tokens->fields + index;
        entry = item_table_at(atoi(p[0]));
        item_table_append_params(entry, atoi(p[1]), atoi(p[2]));
    }

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

    table_reader_read(tokens, "BookItemNameTable.txt", TABLE_FIELDS);
    for (size_t index = 0; index < tokens->count; index++) {
        char **p = tokens->fields + index;
        book_item_add(atoi(*p));
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
