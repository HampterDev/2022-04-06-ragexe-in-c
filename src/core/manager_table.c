#include <stdio.h>
#include "core/fatal.h"
#include "core/manager_table.h"
#include "platform/window.h"

/* The checks of manager_table.h: a destroy rank used by two rows would leave one of them
   undestroyed, and a stop or warn row needs the text it shows. */
static void check_table(const struct ManagerTable *table)
{
    char text[160];
    size_t i, j;

    for (i = 0; i < table->count; i++) {
        const struct ManagerRow *row = &table->rows[i];

        if (row->on_failure.kind != MANAGER_FAILURE_IGNORE && !row->on_failure.text) {
            snprintf(text, sizeof text, "manager table: %s stops or warns without a text", row->name);
            fatal_error(text);
        }
        if (!row->destroy)
            continue;
        for (j = i + 1; j < table->count; j++)
            if (table->rows[j].destroy && table->rows[j].destroy_rank == row->destroy_rank) {
                snprintf(text, sizeof text, "manager table: %s and %s have destroy rank %d", row->name,
                         table->rows[j].name, row->destroy_rank);
                fatal_error(text);
            }
    }
}

const struct ManagerRow *manager_table_create(const struct ManagerTable *table)
{
    size_t i;

    check_table(table);
    for (i = 0; i < table->count; i++) {
        const struct ManagerRow *row = &table->rows[i];

        if (!row->create || row->create())
            continue;
        switch (row->on_failure.kind) {
        case MANAGER_FAILURE_STOP:
            return row;
        case MANAGER_FAILURE_WARN:
            platform_message_box(NULL, row->on_failure.text, "Error", 0);
            break;
        case MANAGER_FAILURE_IGNORE:
            break;
        }
    }
    return NULL;
}

void manager_table_destroy(const struct ManagerTable *table)
{
    const struct ManagerRow *last = NULL;

    check_table(table);
    for (;;) {
        const struct ManagerRow *next = NULL;
        size_t i;

        /* The row with the lowest rank above the last destroyed one. */
        for (i = 0; i < table->count; i++) {
            const struct ManagerRow *row = &table->rows[i];

            if (row->destroy && (!last || row->destroy_rank > last->destroy_rank) &&
                (!next || row->destroy_rank < next->destroy_rank))
                next = row;
        }
        if (!next)
            return;
        next->destroy();
        last = next;
    }
}
