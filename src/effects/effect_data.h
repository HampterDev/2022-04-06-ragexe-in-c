#ifndef EFFECTS_EFFECT_DATA_H
#define EFFECTS_EFFECT_DATA_H

/* The effect data file, data/effects/effects.json (format: data/effects/README.md). The
   build embeds it in the executable; with RAGEXE_EFFECT_DATA set to a file name the client
   reads that file instead, and falls back to the embedded data when it cannot. It is read
   once, on the first lookup. Names are CP949, as the client's own string literals are. */

enum RagStrOpKind {
    RAG_STR_FOLLOW,         /* the effect moves to its master */
    RAG_STR_WAVE,           /* a wave at the effect, at a height */
    RAG_STR_PLAYER_WAVE,    /* a wave at the player */
    RAG_STR_QUAKE,          /* the quake message 0x7a */
    RAG_STR_DURATION,       /* the effect's duration is set */
};

struct RagStrOp {
    unsigned char kind;
    int value;              /* the quake parameter or the duration */
    float height;
    const char *wave;
};

/* Operations that run on frame `at`, or on every frame when at is -1; before the STR step,
   or after it when after_str is set. */
struct RagStrCue {
    int at;
    unsigned char after_str;
    int op_count;
    const struct RagStrOp *ops;
};

enum RagStrEnd {
    RAG_STR_END_STOP,       /* the effect ends */
    RAG_STR_END_CONTINUE,   /* the effect goes on until its duration */
    RAG_STR_END_REWIND,     /* the STR starts again */
};

/* One RagEffect ID: the STR file rag_effect_init loads, and when `plays` is set, the
   frames rag_str_play runs instead of a dispatch case. */
struct RagStrBinding {
    const char *file;       /* NULL: no STR; with variants, a pattern with one %d */
    const char *min_file;   /* used when map_rule_flags[0] is not 0 */
    int variants;           /* 0, or n: %d is 1 + rand() % n */
    int loops;              /* str_max_loops, or -1 to keep it */
    unsigned char plays;
    unsigned char on_end;
    int stop_frame;         /* frame set when the effect ends, or -1 */
    float offset[3];        /* screen offset of the STR step */
    int cue_count;
    const struct RagStrCue *cues;
};

enum Ez2StrOpen {
    EZ2STR_LOAD,            /* the file, or min_file when map_rule_flags[0] is 1 */
    EZ2STR_OPEN,            /* ez2_str_particle_open, which looks for a min_ file itself */
    EZ2STR_FULL_ONLY,       /* the file only when map_rule_flags[0] is 0 */
};

/* One EZ2STR effect type of ez2_str_effect_open_for_type. */
struct Ez2StrFile {
    const char *file;       /* with variants, a pattern with one %d */
    const char *min_file;
    unsigned char open;
    int variants;
};

const struct RagStrBinding *effect_data_rag_str(int effect_id);
const struct Ez2StrFile *effect_data_ez2str(int type);

/* Reads the tables now rather than on the first lookup; returns 0 when even the embedded
   data cannot be read. */
int effect_data_load(void);

#endif
