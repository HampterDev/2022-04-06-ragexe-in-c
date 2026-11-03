#include "core/fatal.h"
#include "core/object_type.h"
#include <string.h>

static const struct ObjectTypeTable *installed;

void object_types_install(const struct ObjectTypeTable *table)
{
    size_t a, i, b, j;

    for (a = 0; a < table->count; ++a)
        for (i = 0; i < table->parts[a]->count; ++i)
            for (b = a; b < table->count; ++b)
                for (j = b == a ? i + 1 : 0; j < table->parts[b]->count; ++j)
                    if (table->parts[a]->rows[i].table == table->parts[b]->rows[j].table)
                        fatal_error("object types: an operation table with two rows");
    installed = table;
}

static const struct ObjectTypeRow *find(const void *table)
{
    size_t a, i;

    for (a = 0; installed && a < installed->count; ++a)
        for (i = 0; i < installed->parts[a]->count; ++i)
            if (installed->parts[a]->rows[i].table == table)
                return &installed->parts[a]->rows[i];
    return NULL;
}

void *object_as_type(void *object, enum ObjectType type)
{
    const struct ObjectTypeRow *row;
    const void *table;
    uint32_t requested;
    if (!object || (unsigned)type >= OBJECT_TYPE_COUNT)
        return NULL;
    memcpy(&table, object, sizeof(table));
    requested = UINT32_C(1) << type;
    row = find(table);
    if (row)
        return row->types & requested ? object : NULL;
    return NULL;
}

const char *object_type_name(const void *table)
{
    const struct ObjectTypeRow *row = find(table);

    return row ? row->name : "?";
}
