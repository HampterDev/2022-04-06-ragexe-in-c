#include "game/box_report.h"

int box_report_begin(FILE *file)
{
    return fputs("ID\tname\tcount\tproperty\n\n", file) >= 0;
}

int box_report_item(FILE *file, int item_id, const char *name, int count, int total)
{
    double share = (double)((float)count / (float)total * 100.0f);
    return fprintf(file, "%d\t%s\t%d\t%.5f%%\n", item_id, name, count, share) >= 0;
}

int box_report_end(FILE *file, int total)
{
    return fprintf(file, "\nresult item count:%d\n", total) >= 0;
}
