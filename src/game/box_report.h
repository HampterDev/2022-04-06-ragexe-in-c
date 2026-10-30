#ifndef GAMEMODE_BOX_REPORT_H
#define GAMEMODE_BOX_REPORT_H

#include <stdio.h>

int box_report_begin(FILE *file);
int box_report_item(FILE *file, int item_id, const char *name, int count, int total);
int box_report_end(FILE *file, int total);

#endif
