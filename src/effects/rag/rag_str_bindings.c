#include "contents/sound.h"
#include "effects/effect_data.h"
#include "effects/rag/rag_effect.h"
#include "game/context.h"

#define GAME_MSG_QUAKE 0x7a

static void run_cues(struct RagEffect *self, const struct RagStrBinding *b, unsigned char after_str)
{
    int i, k;

    for (i = 0; i < b->cue_count; i++) {
        const struct RagStrCue *cue = &b->cues[i];

        if (cue->after_str != after_str || (cue->at >= 0 && self->frame != cue->at))
            continue;
        for (k = 0; k < cue->op_count; k++) {
            const struct RagStrOp *op = &cue->ops[k];

            switch (op->kind) {
            case RAG_STR_FOLLOW:
                rag_follow_master(self);
                break;
            case RAG_STR_WAVE:
                rag_play_wave(self, op->wave, op->height);
                break;
            case RAG_STR_PLAYER_WAVE:
                sound_play_wave(op->wave, 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
                break;
            case RAG_STR_QUAKE:
                game_send_message(GAME_MSG_QUAKE, op->value, 0, 0, 0);
                break;
            case RAG_STR_DURATION:
                self->duration = op->value;
                break;
            }
        }
    }
}

/* One frame of an effect that only plays its STR (data/effects/effects.json, "Play"):
   the cues before the step, the STR step, the cues after it; 0 ends the effect. */
unsigned char rag_str_play(struct RagEffect *self, const struct RagStrBinding *b)
{
    unsigned char going;

    run_cues(self, b, 0);
    going = rag_str_step(self, b->offset[0], b->offset[1], b->offset[2]);
    run_cues(self, b, 1);
    if (going || b->on_end == RAG_STR_END_CONTINUE)
        return 1;
    if (b->on_end == RAG_STR_END_REWIND) {
        rag_str_rewind(self);
        return 1;
    }
    if (b->stop_frame >= 0)
        self->frame = b->stop_frame;
    return 0;
}
