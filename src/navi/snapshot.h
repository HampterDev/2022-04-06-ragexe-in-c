#ifndef NAVI_SNAPSHOT_H
#define NAVI_SNAPSHOT_H
#include "replay/snapshot.h"
struct NaviManager;
struct ReplayRecorder;

int navi_state_save(const struct NaviManager *manager, struct ReplayBuffer *out);
int navi_state_load(struct NaviManager *manager, const void *data, size_t size);
int navi_state_restore(struct NaviManager *manager, struct ReplayRecorder *recorder);

int navi_results_save(const struct NaviManager *manager, struct ReplayBuffer *out);
int navi_results_load(struct NaviManager *manager, const void *data, size_t size);
#endif
