#include "contents/whisper_block.h"
#include <stdlib.h>
#include <string.h>

struct WhisperBlockMgr *g_whisperBlockMgr;

static size_t lower_bound(const struct WhisperBlockMgr *self, const char *name)
{
    size_t first = 0, last = self->names.size;
    while (first < last) {
        size_t middle = first + (last - first) / 2;
        if (strcmp(owned_text_cstr(&self->names.items[middle]), name) < 0)
            first = middle + 1;
        else
            last = middle;
    }
    return first;
}

void whisper_block_init(struct WhisperBlockMgr *self)
{
    text_array_init(&self->names);
}

void whisper_block_reset(struct WhisperBlockMgr *self)
{
    text_array_clear(&self->names);
}

int whisper_block_create(void)
{
    struct WhisperBlockMgr *manager;
    if (g_whisperBlockMgr)
        return 0;
    manager = malloc(sizeof *manager);
    if (!manager)
        return 0;
    whisper_block_init(manager);
    g_whisperBlockMgr = manager;
    return 1;
}

void whisper_block_destroy(void)
{
    if (!g_whisperBlockMgr)
        return;
    whisper_block_reset(g_whisperBlockMgr);
    free(g_whisperBlockMgr);
    g_whisperBlockMgr = NULL;
}

int whisper_block_add(struct WhisperBlockMgr *self, const char *name)
{
    size_t index = lower_bound(self, name), size = self->names.size;
    struct OwnedText copy = {0};
    if (index < size && !strcmp(owned_text_cstr(&self->names.items[index]), name)) {
        return 1;
    }
    if (size == SIZE_MAX || !owned_text_assign(&copy, text_view_cstr(name)))
        return 0;
    if (!text_array_resize(&self->names, size + 1)) {
        owned_text_clear(&copy);
        return 0;
    }
    memmove(self->names.items + index + 1, self->names.items + index,
            (size - index) * sizeof *self->names.items);
    self->names.items[index] = copy;
    return 1;
}

int whisper_block_contains(const struct WhisperBlockMgr *self, const char *name)
{
    size_t index = lower_bound(self, name);
    return index < self->names.size && !strcmp(owned_text_cstr(&self->names.items[index]), name);
}

void whisper_block_remove(struct WhisperBlockMgr *self, const char *name)
{
    size_t index = lower_bound(self, name), size = self->names.size;
    if (index < size && !strcmp(owned_text_cstr(&self->names.items[index]), name)) {
        owned_text_clear(&self->names.items[index]);
        memmove(self->names.items + index, self->names.items + index + 1,
                (size - index - 1) * sizeof *self->names.items);
        owned_text_init(&self->names.items[size - 1]);
        text_array_resize(&self->names, size - 1);
    }
}

void whisper_block_clear(struct WhisperBlockMgr *self)
{
    text_array_clear(&self->names);
}

size_t whisper_block_count(const struct WhisperBlockMgr *self)
{
    return self->names.size;
}

const char *whisper_block_name(const struct WhisperBlockMgr *self, size_t index)
{
    return index < self->names.size ? owned_text_cstr(&self->names.items[index]) : NULL;
}
