#include "effects/effect_data.h"
#include "fileio/resource_path.h"
#include "platform/error_message.h"
#include "yyjson.h"
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* data/effects/effects.json, compiled in by tools/bin2c.c (build.bat). */
extern const unsigned char g_effectData[];
extern const size_t g_effectData_size;

#define FORMAT          "ragexe effect data 1"
#define MAX_ID          65535
#define MAX_FILE_BYTES  (16u << 20)
#define RAG_NAME_MAX    63      /* rag_effect_init builds the name in char[64] */
#define EZ2STR_NAME_MAX 255     /* ez2_str_effect_open_for_type builds it in char[256] */

struct Chunk {
    struct Chunk *next;
    size_t used, size;
    double align;
};

struct Tables {
    struct Chunk *chunks;
    const struct RagStrBinding **rag;
    int rag_count;
    const struct Ez2StrFile **ez2str;
    int ez2str_count;
};

struct Reader {
    struct Tables t;
    char error[400];
};

static struct Tables g_tables;
static int g_state;     /* 0: not read yet, 1: read, -1: failed */

/* ---- errors and storage ---- */

static int fail(struct Reader *r, const char *where, const char *fmt, ...)
{
    va_list args;
    size_t n;
    if (r->error[0])
        return 0;
    n = (size_t)snprintf(r->error, sizeof r->error, "%s%s", where, where[0] ? ": " : "");
    if (n >= sizeof r->error)
        n = sizeof r->error - 1;
    va_start(args, fmt);
    vsnprintf(r->error + n, sizeof r->error - n, fmt, args);
    va_end(args);
    return 0;
}

static void *take(struct Reader *r, size_t bytes)
{
    struct Chunk *c = r->t.chunks;
    char *p;
    bytes = (bytes + 7) & ~(size_t)7;
    if (!c || c->size - c->used < bytes) {
        size_t size = bytes > 65536 ? bytes : 65536;
        c = malloc(sizeof *c + size);
        if (!c) {
            fail(r, "", "out of memory");
            return NULL;
        }
        c->next = r->t.chunks;
        c->used = 0;
        c->size = size;
        r->t.chunks = c;
    }
    p = (char *)(c + 1) + c->used;
    c->used += bytes;
    memset(p, 0, bytes);
    return p;
}

static void free_tables(struct Tables *t)
{
    while (t->chunks) {
        struct Chunk *next = t->chunks->next;
        free(t->chunks);
        t->chunks = next;
    }
    memset(t, 0, sizeof *t);
}

/* ---- values ---- */

static int key_is(yyjson_val *key, const char *name)
{
    return yyjson_get_len(key) == strlen(name) && !memcmp(yyjson_get_str(key), name, strlen(name));
}

/* Every key of obj must be one of names; returns 0 with an error otherwise. */
static int known_keys(struct Reader *r, const char *where, yyjson_val *obj, const char *const *names)
{
    yyjson_val *key, *value;
    size_t i, max, k, seen = 0;
    if (!yyjson_is_obj(obj))
        return fail(r, where, "must be an object");
    yyjson_obj_foreach(obj, i, max, key, value) {
        for (k = 0; names[k] && !key_is(key, names[k]); ++k)
            ;
        if (!names[k])
            return fail(r, where, "unknown key \"%.*s\"", (int)yyjson_get_len(key), yyjson_get_str(key));
        if (seen & ((size_t)1 << k))
            return fail(r, where, "key \"%s\" given twice", names[k]);
        seen |= (size_t)1 << k;
    }
    (void)value;
    return 1;
}

static int read_int(struct Reader *r, const char *where, const char *name, yyjson_val *v, long lo, long hi, int *out)
{
    int in_range;
    if (!yyjson_is_int(v))
        return fail(r, where, "%s must be an integer", name);
    /* yyjson reads integers from 0 up as unsigned and negative ones as signed. */
    if (yyjson_is_uint(v))
        in_range = hi >= 0 && yyjson_get_uint(v) <= (uint64_t)hi && (lo <= 0 || yyjson_get_uint(v) >= (uint64_t)lo);
    else
        in_range = yyjson_get_sint(v) >= lo && yyjson_get_sint(v) <= hi;
    if (!in_range)
        return fail(r, where, "%s must be from %ld to %ld", name, lo, hi);
    *out = yyjson_is_uint(v) ? (int)yyjson_get_uint(v) : (int)yyjson_get_sint(v);
    return 1;
}

/* A number that a float holds exactly: the C code these values come from used float
   constants, and a rounded value would change the effect. */
static int read_float(struct Reader *r, const char *where, const char *name, yyjson_val *v, float *out)
{
    double d;
    if (!yyjson_is_num(v))
        return fail(r, where, "%s must be a number", name);
    d = yyjson_get_num(v);
    if (!(d >= -1e30 && d <= 1e30) || (double)(float)d != d)
        return fail(r, where, "%s must be a number a float holds exactly", name);
    *out = (float)d;
    return 1;
}

static int read_true(struct Reader *r, const char *where, const char *name, yyjson_val *v)
{
    if (!yyjson_is_true(v))
        return fail(r, where, "%s must be true", name);
    return 1;
}

/* A resource name, stored in CP949. With pattern set it holds exactly one %d and no other
   %, without it no % at all; the C code formats these names with snprintf. max is the
   byte count allowed without the %d. */
static const char *read_name(struct Reader *r, const char *where, const char *name, yyjson_val *v, size_t max,
                             int pattern)
{
    const char *s, *pct;
    char converted[512];
    size_t n, i;
    char *out;
    if (!yyjson_is_str(v)) {
        fail(r, where, "%s must be a string", name);
        return NULL;
    }
    s = yyjson_get_str(v);
    n = yyjson_get_len(v);
    if (!n || memchr(s, 0, n)) {
        fail(r, where, "%s must be a file name", name);
        return NULL;
    }
    for (i = 0; i < n && (unsigned char)s[i] < 0x80; ++i)
        ;
    if (i < n) {
        if (n >= sizeof converted || !resource_name_to_cp949(converted, sizeof converted, s)) {
            fail(r, where, "%s has characters CP949 does not hold", name);
            return NULL;
        }
        s = converted;
        n = strlen(converted);
    }
    pct = strchr(s, '%');
    if (pattern ? !pct || pct[1] != 'd' || strchr(pct + 1, '%') : pct != NULL) {
        fail(r, where, pattern ? "%s must hold one %%d for the variant" : "%s must not hold %%", name);
        return NULL;
    }
    if (n - (pattern ? 2 : 0) > max) {
        fail(r, where, "%s is longer than %u bytes", name, (unsigned)max);
        return NULL;
    }
    out = take(r, n + 1);
    if (out)
        memcpy(out, s, n + 1);
    return out;
}

/* ---- the RagEffect table ---- */

static int read_op(struct Reader *r, const char *where, yyjson_val *v, struct RagStrOp *op)
{
    static const char *const keys[] = { "Follow", "Wave", "Height", "AtPlayer", "Quake", "Duration", NULL };
    yyjson_val *wave, *height, *player, *quake, *duration, *follow;
    int actions;
    if (!known_keys(r, where, v, keys))
        return 0;
    follow = yyjson_obj_get(v, "Follow");
    wave = yyjson_obj_get(v, "Wave");
    height = yyjson_obj_get(v, "Height");
    player = yyjson_obj_get(v, "AtPlayer");
    quake = yyjson_obj_get(v, "Quake");
    duration = yyjson_obj_get(v, "Duration");
    actions = !!follow + !!wave + !!quake + !!duration;
    if (actions != 1)
        return fail(r, where, "an operation is one of Follow, Wave, Quake or Duration");
    if ((height || player) && !wave)
        return fail(r, where, "Height and AtPlayer belong to a Wave");
    if (height && player)
        return fail(r, where, "a wave at the player has no Height");
    if (follow) {
        op->kind = RAG_STR_FOLLOW;
        return read_true(r, where, "Follow", follow);
    }
    if (quake) {
        op->kind = RAG_STR_QUAKE;
        return read_int(r, where, "Quake", quake, INT_MIN, INT_MAX, &op->value);
    }
    if (duration) {
        op->kind = RAG_STR_DURATION;
        return read_int(r, where, "Duration", duration, 0, INT_MAX, &op->value);
    }
    op->kind = player ? RAG_STR_PLAYER_WAVE : RAG_STR_WAVE;
    if (player && !read_true(r, where, "AtPlayer", player))
        return 0;
    if (height && !read_float(r, where, "Height", height, &op->height))
        return 0;
    op->wave = read_name(r, where, "Wave", wave, 259, 0);
    return op->wave != NULL;
}

static int read_cue(struct Reader *r, const char *where, yyjson_val *v, struct RagStrCue *cue)
{
    static const char *const keys[] = { "At", "AfterStr", "Do", NULL };
    yyjson_val *at, *after, *ops, *op;
    struct RagStrOp *out;
    size_t i, max;
    char here[64];
    if (!known_keys(r, where, v, keys))
        return 0;
    at = yyjson_obj_get(v, "At");
    after = yyjson_obj_get(v, "AfterStr");
    ops = yyjson_obj_get(v, "Do");
    cue->at = -1;
    if (at && !read_int(r, where, "At", at, 0, INT_MAX, &cue->at))
        return 0;
    if (after) {
        if (!yyjson_is_bool(after))
            return fail(r, where, "AfterStr must be true or false");
        cue->after_str = (unsigned char)yyjson_get_bool(after);
    }
    if (!yyjson_is_arr(ops) || !yyjson_arr_size(ops))
        return fail(r, where, "Do must be a list of operations");
    out = take(r, yyjson_arr_size(ops) * sizeof *out);
    if (!out)
        return 0;
    yyjson_arr_foreach(ops, i, max, op) {
        snprintf(here, sizeof here, "%s, operation %u", where, (unsigned)i + 1);
        if (!read_op(r, here, op, &out[i]))
            return 0;
    }
    cue->ops = out;
    cue->op_count = (int)max;
    return 1;
}

static int read_play(struct Reader *r, const char *where, yyjson_val *v, struct RagStrBinding *b)
{
    static const char *const keys[] = { "Timeline", "Offset", "OnStrEnd", "StopFrame", NULL };
    yyjson_val *timeline, *offset, *end, *stop, *item;
    struct RagStrCue *cues;
    size_t i, max;
    char here[64];
    if (!known_keys(r, where, v, keys))
        return 0;
    b->plays = 1;
    b->offset[1] = -80.0f;
    timeline = yyjson_obj_get(v, "Timeline");
    offset = yyjson_obj_get(v, "Offset");
    end = yyjson_obj_get(v, "OnStrEnd");
    stop = yyjson_obj_get(v, "StopFrame");
    if (timeline) {
        if (!yyjson_is_arr(timeline))
            return fail(r, where, "Timeline must be a list");
        cues = take(r, (yyjson_arr_size(timeline) + 1) * sizeof *cues);
        if (!cues)
            return 0;
        yyjson_arr_foreach(timeline, i, max, item) {
            snprintf(here, sizeof here, "%s, cue %u", where, (unsigned)i + 1);
            if (!read_cue(r, here, item, &cues[i]))
                return 0;
        }
        b->cues = cues;
        b->cue_count = (int)max;
    }
    if (offset) {
        if (!yyjson_is_arr(offset) || yyjson_arr_size(offset) != 3)
            return fail(r, where, "Offset must be a list of three numbers");
        yyjson_arr_foreach(offset, i, max, item)
            if (!read_float(r, where, "Offset", item, &b->offset[i]))
                return 0;
    }
    b->on_end = RAG_STR_END_STOP;
    if (end) {
        const char *s = yyjson_get_str(end);
        if (s && !strcmp(s, "Stop"))
            b->on_end = RAG_STR_END_STOP;
        else if (s && !strcmp(s, "Continue"))
            b->on_end = RAG_STR_END_CONTINUE;
        else if (s && !strcmp(s, "Rewind"))
            b->on_end = RAG_STR_END_REWIND;
        else
            return fail(r, where, "OnStrEnd must be \"Stop\", \"Continue\" or \"Rewind\"");
    }
    if (stop) {
        if (b->on_end != RAG_STR_END_STOP)
            return fail(r, where, "StopFrame belongs to OnStrEnd \"Stop\"");
        if (!read_int(r, where, "StopFrame", stop, 0, INT_MAX, &b->stop_frame))
            return 0;
    }
    return 1;
}

static int read_str(struct Reader *r, const char *where, yyjson_val *v, struct RagStrBinding *b)
{
    static const char *const keys[] = { "File", "MinFile", "Variants", "Loops", NULL };
    yyjson_val *file, *min_file, *variants, *loops;
    if (!known_keys(r, where, v, keys))
        return 0;
    file = yyjson_obj_get(v, "File");
    min_file = yyjson_obj_get(v, "MinFile");
    variants = yyjson_obj_get(v, "Variants");
    loops = yyjson_obj_get(v, "Loops");
    if (!file)
        return fail(r, where, "File is missing");
    if (variants) {
        if (!read_int(r, where, "Variants", variants, 2, 99, &b->variants))
            return 0;
        if (min_file)
            return fail(r, where, "a file with Variants has no MinFile");
    }
    if (loops && !read_int(r, where, "Loops", loops, 0, INT_MAX, &b->loops))
        return 0;
    /* With Variants, the name once %d is replaced by one or two digits. */
    b->file = read_name(r, where, "File", file, RAG_NAME_MAX - (b->variants > 9 ? 2 : variants ? 1 : 0),
                        variants != NULL);
    if (!b->file)
        return 0;
    if (min_file && !(b->min_file = read_name(r, where, "MinFile", min_file, RAG_NAME_MAX, 0)))
        return 0;
    return 1;
}

static int read_rag_row(struct Reader *r, const char *where, yyjson_val *v, struct RagStrBinding *b)
{
    static const char *const keys[] = { "Str", "Play", NULL };
    yyjson_val *str, *play;
    char here[48];
    if (!known_keys(r, where, v, keys))
        return 0;
    str = yyjson_obj_get(v, "Str");
    play = yyjson_obj_get(v, "Play");
    if (!str && !play)
        return fail(r, where, "a row needs Str or Play");
    b->loops = -1;
    b->stop_frame = -1;
    snprintf(here, sizeof here, "%s Str", where);
    if (str && !read_str(r, here, str, b))
        return 0;
    snprintf(here, sizeof here, "%s Play", where);
    if (play && !read_play(r, here, play, b))
        return 0;
    return 1;
}

/* ---- the EZ2STR table ---- */

static int read_ez2str_row(struct Reader *r, const char *where, yyjson_val *v, struct Ez2StrFile *f)
{
    static const char *const keys[] = { "File", "MinFile", "Open", "FullOnly", "Variants", NULL };
    yyjson_val *file, *min_file, *open, *full_only, *variants;
    if (!known_keys(r, where, v, keys))
        return 0;
    file = yyjson_obj_get(v, "File");
    min_file = yyjson_obj_get(v, "MinFile");
    open = yyjson_obj_get(v, "Open");
    full_only = yyjson_obj_get(v, "FullOnly");
    variants = yyjson_obj_get(v, "Variants");
    if (!file)
        return fail(r, where, "File is missing");
    if (!!min_file + !!open + !!full_only + !!variants > 1)
        return fail(r, where, "MinFile, Open, FullOnly and Variants exclude each other");
    f->open = EZ2STR_LOAD;
    if (open) {
        if (!read_true(r, where, "Open", open))
            return 0;
        f->open = EZ2STR_OPEN;
    }
    if (full_only) {
        if (!read_true(r, where, "FullOnly", full_only))
            return 0;
        f->open = EZ2STR_FULL_ONLY;
    }
    if (variants && !read_int(r, where, "Variants", variants, 2, 99, &f->variants))
        return 0;
    f->file = read_name(r, where, "File", file, EZ2STR_NAME_MAX - (variants ? 2 : 0), variants != NULL);
    if (!f->file)
        return 0;
    if (min_file && !(f->min_file = read_name(r, where, "MinFile", min_file, EZ2STR_NAME_MAX, 0)))
        return 0;
    return 1;
}

/* ---- tables ---- */

static int read_id(struct Reader *r, const char *table, yyjson_val *key, int *id)
{
    const char *s = yyjson_get_str(key);
    size_t n = yyjson_get_len(key), i;
    long v = 0;
    if (!n || n > 5 || (n > 1 && s[0] == '0'))
        return fail(r, table, "\"%.*s\" is not an ID", (int)n, s);
    for (i = 0; i < n; ++i) {
        if (s[i] < '0' || s[i] > '9')
            return fail(r, table, "\"%.*s\" is not an ID", (int)n, s);
        v = v * 10 + (s[i] - '0');
    }
    if (v > MAX_ID)
        return fail(r, table, "ID %ld is over %d", v, MAX_ID);
    *id = (int)v;
    return 1;
}

typedef int (*ReadRow)(struct Reader *, const char *, yyjson_val *, void *);

static int read_table(struct Reader *r, yyjson_val *root, const char *table, size_t row_size, ReadRow read_row,
                      const void ***index, int *count)
{
    yyjson_val *rows = yyjson_obj_get(root, table), *key, *value;
    size_t i, max;
    int id, top = -1;
    const void **slots;
    char where[48];
    if (!yyjson_is_obj(rows))
        return fail(r, "", "%s must be an object of rows by ID", table);
    yyjson_obj_foreach(rows, i, max, key, value) {
        if (!read_id(r, table, key, &id))
            return 0;
        if (id > top)
            top = id;
    }
    slots = take(r, (size_t)(top + 1) * sizeof *slots + 1);
    if (!slots)
        return 0;
    yyjson_obj_foreach(rows, i, max, key, value) {
        void *row;
        read_id(r, table, key, &id);
        snprintf(where, sizeof where, "%s %d", table, id);
        if (slots[id])
            return fail(r, where, "given twice");
        row = take(r, row_size);
        if (!row || !read_row(r, where, value, row))
            return 0;
        slots[id] = row;
    }
    *index = slots;
    *count = top + 1;
    return 1;
}

static int read_rag_any(struct Reader *r, const char *where, yyjson_val *v, void *row)
{
    return read_rag_row(r, where, v, row);
}

static int read_ez2str_any(struct Reader *r, const char *where, yyjson_val *v, void *row)
{
    return read_ez2str_row(r, where, v, row);
}

static int read_tables(struct Reader *r, const char *bytes, size_t size)
{
    static const char *const keys[] = { "Format", "RagEffect", "Ez2StrFile", NULL };
    yyjson_read_err err;
    yyjson_doc *doc;
    yyjson_val *root, *format;
    int ok;
    memset(r, 0, sizeof *r);
    doc = yyjson_read_opts((char *)bytes, size, YYJSON_READ_NOFLAG, NULL, &err);
    if (!doc)
        return fail(r, "", "not JSON (%s at byte %u)", err.msg ? err.msg : "empty", (unsigned)err.pos);
    root = yyjson_doc_get_root(doc);
    format = yyjson_obj_get(root, "Format");
    ok = known_keys(r, "", root, keys);
    if (ok && (!yyjson_is_str(format) || strcmp(yyjson_get_str(format), FORMAT)))
        ok = fail(r, "", "Format must be \"" FORMAT "\"");
    ok = ok && read_table(r, root, "RagEffect", sizeof(struct RagStrBinding), read_rag_any,
                          (const void ***)&r->t.rag, &r->t.rag_count);
    ok = ok && read_table(r, root, "Ez2StrFile", sizeof(struct Ez2StrFile), read_ez2str_any,
                          (const void ***)&r->t.ez2str, &r->t.ez2str_count);
    yyjson_doc_free(doc);
    if (!ok)
        free_tables(&r->t);
    return ok;
}

static int read_file(struct Reader *r, const char *path)
{
    FILE *f = fopen(path, "rb");
    char *bytes;
    long size;
    int ok;
    memset(r, 0, sizeof *r);
    if (!f)
        return fail(r, "", "cannot open the file");
    if (fseek(f, 0, SEEK_END) || (size = ftell(f)) < 0 || (unsigned long)size > MAX_FILE_BYTES ||
        fseek(f, 0, SEEK_SET)) {
        fclose(f);
        return fail(r, "", "cannot read the file, or it is over 16 MB");
    }
    bytes = malloc((size_t)size + 1);
    if (!bytes || fread(bytes, 1, (size_t)size, f) != (size_t)size) {
        free(bytes);
        fclose(f);
        return fail(r, "", "cannot read the file");
    }
    fclose(f);
    ok = read_tables(r, bytes, (size_t)size);
    free(bytes);
    return ok;
}

/* ---- loading ---- */

static void report(const char *source, const char *error, const char *then)
{
    char text[700];
    snprintf(text, sizeof text, "Effect data %s: %s.%s", source, error, then);
    error_msg(text, 0);
}

int effect_data_load(void)
{
    struct Reader r;
    const char *path;
    if (g_state)
        return g_state > 0;
    path = getenv("RAGEXE_EFFECT_DATA");
    if (path && path[0]) {
        if (read_file(&r, path)) {
            g_tables = r.t;
            g_state = 1;
            return 1;
        }
        report(path, r.error, "\nThe client uses its built-in effect data instead.");
    }
    if (read_tables(&r, (const char *)g_effectData, g_effectData_size)) {
        g_tables = r.t;
        g_state = 1;
        return 1;
    }
    report("built into the client", r.error, "");
    g_state = -1;
    return 0;
}

const struct RagStrBinding *effect_data_rag_str(int effect_id)
{
    if (!g_state)
        effect_data_load();
    if (effect_id < 0 || effect_id >= g_tables.rag_count)
        return NULL;
    return g_tables.rag[effect_id];
}

const struct Ez2StrFile *effect_data_ez2str(int type)
{
    if (!g_state)
        effect_data_load();
    if (type < 0 || type >= g_tables.ez2str_count)
        return NULL;
    return g_tables.ez2str[type];
}
