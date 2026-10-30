#include "world/world.h"

int cell_ids_less(const void *a, const void *b)
{
    return *(const float *)a < *(const float *)b;
}

void cell_ids_destroy(void *value)
{
    struct CellIds *cell = value;

    list_free(&cell->ids, NULL);
}
