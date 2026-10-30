#include "std/vec.h"
#include "std/std.h"
#include "std/tree.h"
#include "core/text_util.h"
#include "platform/code_page.h"
#include <ctype.h>
#include <string.h>
#include "effects/ez2str_effect_mgr.h"
#include "json/json_loader.h"

#define MSG_DUPLICATE    JSON_MSG_DUPLICATE

struct EZ2STRRecord {
    struct String name;
    struct String file_path;
    double pos_x, pos_y;
    unsigned char is_infinite;
    unsigned char is_floor;
    struct String sound_path;
    unsigned char is_sound_loop;
    int quake_delay;
    int start_delay;
};

struct LoadClosure {
    struct Ez2StrEffectMgr *mgr;
};

void ez2_str_effect_mgr_add_record(struct LoadClosure *closure,
                                            const struct EZ2STRRecord *record);
unsigned char ez2_str_effect_mgr_read_table(struct JsonCppLoader *loader,
                                                     struct Tree *records);

static struct Ez2StrEffectMgr *g_ez2strMgr;

static int name_less(const void *a, const void *b)
{
    return string_compare(a, b) < 0;
}

static void destroy_record(void *value)
{
    struct EZ2STRRecord *record = value;

    string_free(&record->sound_path);
    string_free(&record->file_path);
    string_free(&record->name);
}

static void destroy_row(void *value)
{
    struct EZ2STRRow *row = value;

    string_free(&row->entry.sound_path);
    string_free(&row->entry.file_path);
    string_free(&row->name);
}

struct Ez2StrEffectMgr *ez2str_effect_mgr_instance(void)
{
    struct Ez2StrEffectMgr *mgr;

    if (g_ez2strMgr)
        return g_ez2strMgr;
    mgr = mem_try_alloc(sizeof *mgr);
    if (!mgr)
        return NULL;
    tree_init(&mgr->rows);
    g_ez2strMgr = mgr;
    return mgr;
}

void ez2str_effect_mgr_destroy(void)
{
    if (!g_ez2strMgr)
        return;
    tree_free(&g_ez2strMgr->rows, destroy_row);
    mem_free(g_ez2strMgr);
    g_ez2strMgr = NULL;
}

static struct EZ2STRRow *find_effect_id(const struct Ez2StrEffectMgr *self, int effect_id)
{
    struct TreeNode *node;

    for (node = tree_first(&self->rows); node != self->rows.head; node = tree_next(node)) {
        struct EZ2STRRow *row = TREE_VALUE(node, struct EZ2STRRow);

        if (effect_id == row->entry.index + EZ2STR_EX_FIRST_ID)
            return row;
    }
    return NULL;
}

static void lower_case(struct String *s)
{
    unsigned i;

    for (i = 0; i < s->size; i++)
        s->data[i] = (char)tolower((signed char)s->data[i]);
}

#define TABLE_NAME "EZ2STREffect"

unsigned char ez2_str_effect_mgr_read_table(struct JsonCppLoader *loader,
                                                     struct Tree *records)
{
    const struct JsonTree *table;
    size_t i;
    double pos_x = 0.0, pos_y = 0.0;
    unsigned char is_infinite = 0, is_floor = 0, is_sound_loop = 0;
    int quake_delay = 0, start_delay = 0;

    table = json_loader_table(loader, TABLE_NAME, sizeof(TABLE_NAME) - 1);
    if (!table)
        return 0;
    if (table->type != JSON_TREE_OBJECT)
        return 0;

    for (i = 0; i < table->count; i++) {
        const struct JsonTreeMember *row = &table->u.members[i];
        struct String name, file_path, sound_path;
        struct EZ2STRRecord record;
        int added;

        string_init(&name);
        string_assign(&name, row->key, row->key_length);
        string_init(&file_path);
        string_init(&sound_path);

        json_loader_read_string(loader, "FilePath", &row->value, &file_path);
        json_loader_read_double(loader, "PosX", &row->value, &pos_x);
        json_loader_read_double(loader, "PosY", &row->value, &pos_y);
        json_loader_read_bool(loader, "IsInfinite", &row->value, &is_infinite);
        json_loader_read_bool(loader, "IsFloor", &row->value, &is_floor);
        json_loader_read_string(loader, "SoundPath", &row->value, &sound_path);
        json_loader_read_bool(loader, "IsSoundLoop", &row->value, &is_sound_loop);
        json_loader_read_int(loader, "QuakeDelay", &row->value, &quake_delay);
        json_loader_read_int(loader, "StartDelayTime", &row->value, &start_delay);

        string_copy(&record.name, &name);
        string_copy(&record.file_path, &file_path);
        record.pos_x = pos_x;
        record.pos_y = pos_y;
        record.is_infinite = is_infinite;
        record.is_floor = is_floor;
        string_copy(&record.sound_path, &sound_path);
        record.is_sound_loop = is_sound_loop;
        record.quake_delay = quake_delay;
        record.start_delay = start_delay;
        tree_add(records, &record, sizeof record, name_less, &added);
        if (!added) {
            destroy_record(&record);
            json_loader_set_error(loader, MSG_DUPLICATE, string_cstr(&name));
            string_free(&sound_path);
            string_free(&file_path);
            string_free(&name);
            return 0;
        }
        string_free(&sound_path);
        string_free(&file_path);
        string_free(&name);
    }
    return loader->ok;
}

void ez2_str_effect_mgr_add_record(struct LoadClosure *closure,
                                            const struct EZ2STRRecord *record)
{
    struct Ez2StrEffectMgr *mgr = closure->mgr;
    struct String file_path, sound_path, converted, name;
    struct EZ2STRRow row;
    int added;
    unsigned flags = 0;
    float pos_x, pos_y;
    int index = (int)mgr->rows.size;

    string_init(&file_path);
    string_init(&sound_path);

    text_utf8_to_codepage(&converted, &record->file_path, g_codePage);
    string_assign(&file_path, string_cstr(&converted), converted.size);
    string_free(&converted);
    pos_x = (float)record->pos_x;
    pos_y = (float)record->pos_y;
    if (record->is_infinite)
        flags |= EZ2STR_EX_INFINITE;
    if (record->is_floor)
        flags |= EZ2STR_EX_FLOOR;

    text_utf8_to_codepage(&converted, &record->sound_path, g_codePage);
    string_assign(&sound_path, string_cstr(&converted), converted.size);
    string_free(&converted);
    if (record->is_sound_loop)
        flags |= EZ2STR_EX_SOUND_LOOP;

    text_utf8_to_codepage(&name, &record->name, g_codePage);
    lower_case(&name);

    string_copy(&row.name, &name);
    row.entry.index = index;
    string_copy(&row.entry.file_path, &file_path);
    row.entry.pos_x = pos_x;
    row.entry.pos_y = pos_y;
    row.entry.flags = flags;
    string_copy(&row.entry.sound_path, &sound_path);
    row.entry.quake_delay = record->quake_delay;
    row.entry.start_delay = record->start_delay;
    tree_add(&mgr->rows, &row, sizeof row, name_less, &added);
    if (!added)
        destroy_row(&row);
    string_free(&name);
    string_free(&sound_path);
    string_free(&file_path);
}

unsigned char ez2_str_effect_mgr_load(struct Ez2StrEffectMgr *self)
{
    struct File file;
    unsigned char ok;

    file_construct(&file);
    if (!file_open(&file, "ContentData\\EffectData\\EZ2STREffect.bson", 0, 1, 0)) {
        ok = 0;
    } else {
        struct JsonCppLoader loader;
        struct Vec bytes;

        json_loader_init(&loader);
        vec_init(&bytes);
        vec_assign(&bytes, file.buffer, (const char *)file.buffer + file.size, 1);
        ok = json_loader_load_bson(&loader, &bytes);
        vec_free(&bytes);
        if (ok) {
            struct Tree records;

            tree_init(&records);
            if (!ez2_str_effect_mgr_read_table(&loader, &records)) {
                ok = 0;
            } else {
                struct LoadClosure closure;
                struct TreeNode *node;

                tree_clear(&self->rows, destroy_row);
                closure.mgr = self;
                for (node = tree_first(&records); node != records.head; node = tree_next(node))
                    ez2_str_effect_mgr_add_record(&closure, TREE_VALUE(node, struct EZ2STRRecord));
                ok = 1;
            }
            tree_free(&records, destroy_record);
        }
        json_loader_free(&loader);
    }
    file_destruct(&file);
    return ok;
}

const struct EZ2STREntry *ez2_str_effect_mgr_find_by_effect_id(struct Ez2StrEffectMgr *self, int effect_id)
{
    struct EZ2STRRow *row = find_effect_id(self, effect_id);

    return row ? &row->entry : 0;
}

int ez2_str_effect_mgr_get_effect_id(struct Ez2StrEffectMgr *self,
                                             const struct String *name)
{
    struct String key;
    struct EZ2STRRow *row;
    int effect_id;

    string_copy(&key, name);
    lower_case(&key);
    row = tree_find(&self->rows, &key, name_less);
    effect_id = row ? row->entry.index + EZ2STR_EX_FIRST_ID : -1;
    string_free(&key);
    return effect_id;
}

struct Vec *ez2_str_effect_mgr_get_effect_ids(struct Ez2StrEffectMgr *self,
                                                            struct Vec *ids)
{
    struct TreeNode *node;

    ids->first = ids->last = ids->end = 0;
    for (node = tree_first(&self->rows); node != self->rows.head; node = tree_next(node)) {
        int effect_id = TREE_VALUE(node, struct EZ2STRRow)->entry.index + EZ2STR_EX_FIRST_ID;
        vec_push(ids, &effect_id, sizeof(int));
    }
    return ids;
}
