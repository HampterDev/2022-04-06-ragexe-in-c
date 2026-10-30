#ifndef NAVI_SNAPSHOT_RECORDS_H
#define NAVI_SNAPSHOT_RECORDS_H
#include "navi/snapshot.h"
struct NaviObject;
struct NaviLink;
struct SearchResult;

int navi_link_record_save(struct NaviLink *link, struct ReplayBuffer *out);
int navi_result_record_save(const struct SearchResult *result, struct ReplayBuffer *out);
int navi_link_record_valid(struct SnapshotSpan record);
int navi_result_record_valid(struct SnapshotSpan record);
int navi_link_record_load(struct NaviManager *manager, struct SnapshotSpan record, struct NaviLink **out);
int navi_result_record_load(struct NaviManager *manager, struct SnapshotSpan record, struct SearchResult *out);
#endif
