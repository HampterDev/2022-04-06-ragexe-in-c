#include "core/msg_str.h"
#include "core/table_reader.h"
#include "core/client.h"
#include "std/std.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/language.h"

static struct TableReader message_reader;
static char *tagged_messages[MSG_STRING_COUNT];
static char missing_message[32];
static const char no_message[] = "NO MSG";

void msg_str_unload(void)
{
    size_t id;
    for (id = 0; id < MSG_STRING_COUNT; id++) {
        free(tagged_messages[id]);
        tagged_messages[id] = NULL;
    }
    table_reader_release(&message_reader);
}

void msg_strings_init(void)
{
    atexit(msg_str_unload);
}

void msg_str_load(void)
{
    msg_str_unload();
    table_reader_read(&message_reader, "msgStringTable.txt", TABLE_FIELDS);
}

const char *msg_str_field(unsigned id)
{
    if (id < message_reader.count)
        return message_reader.fields[id];
    return no_message;
}

const char *msg_str_to_compare(int id)
{
    return (unsigned)id < MSG_STRING_COUNT ? msg_str_field((unsigned)id) : no_message;
}

char *msg_str(int id)
{
    const char *text;
    if ((unsigned)id >= MSG_STRING_COUNT) {
        snprintf(missing_message, sizeof missing_message, "NO MSG : %d", id);
        return missing_message;
    }
    text = msg_str_to_compare(id);
    if (!g_useCharsetTag || text[0] == '/' || strcmp(text, no_message) == 0)
        return (char *)text;

    if (!tagged_messages[id]) {
        char buffer[260];
        char *copy;
        size_t length;
        snprintf(buffer, sizeof buffer, "%s%s", language_charset_tag(g_language, 1), text);
        length = strlen(buffer) + 1;
        copy = malloc(length);
        if (!copy)
            return (char *)text;
        memcpy(copy, buffer, length);
        free(tagged_messages[id]);
        tagged_messages[id] = copy;
    }
    return tagged_messages[id];
}
