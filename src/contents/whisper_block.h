#ifndef CONTENTS_WHISPER_BLOCK_H
#define CONTENTS_WHISPER_BLOCK_H

#include <stdint.h>
#include "core/text_array.h"

struct WhisperBlockMgr {
    struct TextArray names;
};

extern struct WhisperBlockMgr *g_whisperBlockMgr;

void whisper_block_init(struct WhisperBlockMgr *self);
void whisper_block_reset(struct WhisperBlockMgr *self);
int whisper_block_create(void);
void whisper_block_destroy(void);

int whisper_block_add(struct WhisperBlockMgr *self, const char *name);
int whisper_block_contains(const struct WhisperBlockMgr *self, const char *name);
void whisper_block_remove(struct WhisperBlockMgr *self, const char *name);
void whisper_block_clear(struct WhisperBlockMgr *self);

size_t whisper_block_count(const struct WhisperBlockMgr *self);
const char *whisper_block_name(const struct WhisperBlockMgr *self, size_t index);

#endif
