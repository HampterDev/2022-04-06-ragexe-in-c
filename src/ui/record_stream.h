#ifndef UI_RECORD_STREAM_H
#define UI_RECORD_STREAM_H
#include "replay/snapshot.h"
#include "ui/record_tags.h"

enum { UI_RECORD_TEXT_CAPACITY = 0x104 };
int ui_window_records_validate(const void *data, size_t size);
int ui_window_records_open(struct SnapshotReader *reader, const void *data, size_t size, uint16_t begin);
#endif
