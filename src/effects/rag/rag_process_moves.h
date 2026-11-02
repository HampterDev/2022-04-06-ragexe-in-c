#ifndef EFFECTS_RAG_PROCESS_MOVES_H
#define EFFECTS_RAG_PROCESS_MOVES_H

#include "effects/rag/rag_effect.h"

#define MONSTER_DIR "sprite\\몬스터\\"
#define UI_ITEM "유저인터페이스\\item\\"
#define MAGIC_CIRCLE "effect\\마법진"
#define SPHERE "effect\\구체"
#define SPIN_COUNTER "effect\\회전카운터"

#define GAME_MSG_QUAKE 0x7a

void rag_quake(void);

void rag_wave_at(struct RagEffect *self, int frame, const char *wave);

void rag_wave_plain(const char *wave);

unsigned char rag_str_step_up80(struct RagEffect *self);

void rag_str_or_rewind(struct RagEffect *self);

void rag_flicker_tint(struct RagEffect *self, int first, int last, int always, int even, int r, int g, int b);

void rag_tint_mark(struct RagEffect *self);

void rag_tint_early(struct RagEffect *self, int r, int g, int b);

void rag_tinted_rings(struct RagEffect *self, const char *texture, char style, int r, int g, int b);

void rag_master_spin(struct RagEffect *self);

void rag_master_flash(struct RagEffect *self, unsigned argb);

void rag_flying_kick_wave(struct RagEffect *self);

void rag_streams_tint(struct RagEffect *self);

void rag_spell_circles(struct RagEffect *self, const char *texture, char style);

void rag_spell_circles_low(struct RagEffect *self, const char *texture, char style45, char style25);

void rag_spell_rings(struct RagEffect *self, const char *texture, char style);


void rag_plant_actor(struct RagEffect *self);

unsigned char rag_process_800(struct RagEffect *self);

#endif
