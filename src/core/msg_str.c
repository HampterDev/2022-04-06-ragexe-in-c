#include "core/msg_str.h"
#include "core/table_reader.h"
#include "core/client.h"
#include "std/std.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct TableReader message_reader;
static char missing_message[32];
static const char no_message[] = "NO MSG";

void msg_str_unload(void)
{
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
    if ((unsigned)id >= MSG_STRING_COUNT) {
        snprintf(missing_message, sizeof missing_message, "NO MSG : %d", id);
        return missing_message;
    }
    return (char *)msg_str_to_compare(id);
}
