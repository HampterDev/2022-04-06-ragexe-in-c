#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/effects/effect.c"
#include "../src/effects/skills/svg_spirit_effect.c"

static void *blocks[16];
static unsigned block_count, frees[16], free_count;
static int allocation_fails;

void *mem_try_alloc(size_t bytes)
{
    return allocation_fails ? NULL : mem_alloc(bytes);
}

void *mem_alloc(size_t bytes)
{
    void *block = calloc(1, bytes ? bytes : 1);
    assert(block && block_count < 16);
    blocks[block_count++] = block;
    return block;
}

void mem_free(void *block)
{
    unsigned i;
    if (!block) return;
    for (i = 0; i < block_count; ++i) {
        if (blocks[i] == block) {
            blocks[i] = NULL;
            frees[free_count++] = i;
            free(block);
            return;
        }
    }
    assert(!"free of an unowned or already freed block");
}

void vec_free(struct Vec *vector)
{
    mem_free(vector->first);
    vector->first = vector->last = vector->end = NULL;
}

void string_free(struct String *text)
{
    if (text->data)
        mem_free(text->data);
    memset(text, 0, sizeof *text);
}

static void sound_cue_storage(struct Effect *effect)
{
    struct SoundCue *cue = mem_alloc(sizeof *cue);
    effect->sound_cues.first = cue;
    effect->sound_cues.last = effect->sound_cues.end = cue + 1;
    cue->wave.capacity = 64;
    cue->wave.data = mem_alloc(65);
    cue->wave.size = 64;
}

static void reset_allocations(void)
{
    unsigned i;
    for (i = 0; i < block_count; ++i) assert(!blocks[i]);
    block_count = free_count = 0;
}

static void base_effect_cases(void)
{
    struct Effect *effect = mem_alloc(sizeof *effect);
    effect->ops = &effect_ops;
    effect->ops->destroy(effect);
    assert(free_count == 1 && frees[0] == 0);
    reset_allocations();

    effect = mem_alloc(sizeof *effect);
    effect->ops = &effect_ops;
    sound_cue_storage(effect);
    effect->ops->destroy(effect);
    assert(free_count == 3 && frees[0] == 2 && frees[1] == 1 && frees[2] == 0);
    reset_allocations();
}

static void factory_creation(void)
{
    struct Effect *effect;
    allocation_fails = 1;
    assert(!svg_spirit_effect_new());
    assert(block_count == 0);
    allocation_fails = 0;
    effect = svg_spirit_effect_new();
    assert(effect && block_count == 1);
    assert(effect->ops == &svg_spirit_effect_ops && effect->type == 0x468);
    assert(effect->duration == 60 && effect->frame == 0);
    assert(!((struct SvgSpiritEffect *)effect)->spirit);
    effect->ops->destroy(effect);
    assert(free_count == 1);
    reset_allocations();
}

int main(void)
{
    factory_creation();
    base_effect_cases();
    puts("Effect destruction: real base callbacks, factory creation, resource order and single free passed.");
    return 0;
}

#include "effect_destruction_unreached.h"
