#include "session/character_name.h"
#include "session/csession.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_lua.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "session/session_talk.h"

#include <stdlib.h>
#include <string.h>
#include "iteminfo/iteminfo.h"
#include "ui/ui_std.h"

static unsigned char less_cstr_first(const void *a, const void *b, void *pred)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b) < 0;
}

static unsigned char less_string_text(const void *a, const void *b, void *pred)
{
    return strcmp(string_cstr((const struct String *)a), string_cstr((const struct String *)b)) < 0;
}

static unsigned char less_string(const void *a, const void *b, void *pred)
{
    return string_compare((const struct String *)a, (const struct String *)b) < 0;
}

int session_load_buying_store_items(struct Session *self)
{
    struct TableReader *words = &self->tableReader;
    struct Vec *items = &self->buyingStoreItems;
    BYTE ok;

    items->last = items->first;
    vec_reserve(items, 0x200, sizeof(int));
    ok = table_reader_read(words, "BuyingStoreItemList.txt", TABLE_FIELDS);
    for (size_t i = 0; i < words->count; i++) {
        char **w = words->fields + i;
        int id = atoi(*w);
        vec_push(items, &id, sizeof(int));
    }
    return ok;
}

int session_load_exception_minimap_names(struct Session *self)
{
    struct TableReader *words = &self->tableReader;
    struct Vec *names = &self->exceptionMinimapNames;
    BYTE ok;

    string_vec_clear(names);
    vec_reserve(names, 0x200, sizeof(struct String));
    ok = table_reader_read(words, "ExceptionMinimapNameTable.txt", TABLE_FIELDS);
    for (size_t i = 0; i < words->count; i++) {
        char **w = words->fields + i;

        string_vec_push_text(names, *w, strlen(*w));
    }
    std_sort(names->first, names->last, sizeof(struct String), less_string, NULL);
    return ok;
}

BYTE session_is_not_minimap_exception(struct Session *self)
{
    struct String arg, name;
    const struct String *s;
    const char *text;
    BYTE result = 1;

    string_init(&arg);
    string_assign_cstr(&arg, g_session1.mapName);
    text = game_mode_short_name(arg);
    string_init(&name);
    string_assign_cstr(&name, text);
    for (s = (const struct String *)self->exceptionMinimapNames.first;
         s != (const struct String *)self->exceptionMinimapNames.last; s++)
        if (strcmp(string_cstr(s), string_cstr(&name)) == 0) {
            result = 0;
            break;
        }
    string_free(&name);
    return result;
}

int session_load_indoor_rsws(struct Session *self)
{
    struct TableReader *reader = &self->tableReader;
    table_reader_read(reader, "indoorRswTable.txt", TABLE_FIELDS);
    self->indoorRsws.last = self->indoorRsws.first;
    for (size_t i = 0; i < reader->count; i++) {
        const void *name = reader->fields[i];
        vec_push(&self->indoorRsws, &name, sizeof name);
    }
    if (reader->count > 1)
        std_sort(self->indoorRsws.first, self->indoorRsws.last, sizeof(const char *), less_cstr_first, NULL);
    return 1;
}

int session_scale_rect_maps(struct Session *self)
{
    struct TreeNode *it;
    int i;

    for (i = 0; i < self->rectMapCount; i++)
        for (it = tree_first(&self->rectMaps[i]); it != self->rectMaps[i].head; it = tree_next(it))
            scale_rect_to_screen(&TREE_VALUE(it, struct NamedMapRect)->rect.rect);
    for (it = tree_first(&self->mapPosGroupBoxes); it != self->mapPosGroupBoxes.head; it = tree_next(it))
        scale_rect_to_screen(&TREE_VALUE(it, struct GroupMapRect)->rect.rect);
    return 1;
}

void session_clear_homun_commands(struct Session *self)
{
    self->homun_command[3] = 0;
    self->homun_command[4] = 0;
    self->homun_command[5] = 0;
    self->homun_command[0] = 0;
    self->homun_command[1] = 0;
    self->homun_command[2] = 0;
}

void named_strings_clear(struct Vec *v)
{
    struct NamedStrings *entry;

    for (entry = (struct NamedStrings *)v->first; entry != (struct NamedStrings *)v->last; entry++) {
        string_vec_free(&entry->values);
        string_free(&entry->name);
    }
    vec_clear(v);
}

void named_strings_free(struct Vec *v)
{
    named_strings_clear(v);
    vec_free(v);
}

static int load_named_table(struct Session *self, struct Vec *table, const char *fileName,
                            size_t count)
{
    struct String name, field;
    struct Vec values;
    struct File file;
    const char *p, *start, *end;
    int result;

    named_strings_clear(table);
    file_construct(&file);
    if (!file_open(&file, fileName, 0, 1, 0)) {
        result = 0;
    } else {
        end = (const char *)file.buffer + file.size;
        string_init(&name);
        values.first = values.last = values.end = NULL;
        for (start = p = (const char *)file.buffer; p < end; p++) {
            if (*p == '\n' || *p == '\r') {
                start = p + 1;
                continue;
            }
            if (*p != '#')
                continue;
            if (name.size == 0) {
                string_assign(&name, start, (size_t)(p - start));
            } else if (VEC_COUNT(&values, struct String) < count - 1) {
                string_init(&field);
                string_assign(&field, start, (size_t)(p - start));
                string_vec_push_move(&values, &field);
                string_free(&field);
            } else {
                struct NamedStrings entry;

                string_init(&field);
                string_assign(&field, start, (size_t)(p - start));
                string_vec_push_move(&values, &field);
                string_free(&field);
                string_copy(&entry.name, &name);
                string_vec_copy(&entry.values, &values);
                vec_push(table, &entry, sizeof entry);
                string_clear(&name);
                string_vec_clear(&values);
            }
            start = p + 1;
        }
        std_sort(table->first, table->last, sizeof(struct NamedStrings), less_string_text, NULL);
        file_close(&file);
        result = 1;
        string_vec_free(&values);
        string_free(&name);
    }
    file_destruct(&file);
    return result;
}

int session_load_fog_parameters(struct Session *self)
{
    return load_named_table(self, &self->fogParameters, "fogParameterTable.txt", 4);
}

int session_load_view_points(struct Session *self)
{
    return load_named_table(self, &self->viewPoints, "ViewPointTable.txt", 9);
}

struct NamedInts {
    struct String name;
    struct Vec values;
};

void session_reload_texts(struct Session *self)
{
    msg_str_load();
    session_load_monster_skill_info(self);
    item_tables_load();
}

void session_clear_merc_commands(struct Session *self)
{
    self->merc_command[3] = 0;
    self->merc_command[5] = 0;
    self->merc_command[6] = 0;
    self->merc_command[0] = 0;
    self->merc_command[1] = 0;
    self->merc_command[2] = 0;
}

int session_load_mp3_names(struct Session *self)
{
    struct TableReader *words = &self->tableReader;
    struct Vec *names = &self->mp3Names;
    BYTE ok;

    names->last = names->first;
    vec_reserve(names, 0x200, sizeof(struct NamedText));
    ok = table_reader_read(words, "mp3NameTable.txt", TABLE_FIELDS);
    for (size_t i = 0; i + 2 <= words->count; i += 2) {
        char **w = words->fields + i;
        struct NamedText entry;
        entry.name = w[0];
        entry.text = w[1];
        vec_push(names, &entry, sizeof entry);
    }
    std_sort(names->first, names->last, sizeof(struct NamedText), less_cstr_first, NULL);
    return ok;
}

void session_load_mini_party_info(struct Session *self)
{
    struct File file;
    struct PlateRecord entry;
    char name[0x18];
    char *buf;
    int size, i, state;

    file_construct(&file);
    if (file_open(&file, "savedata\\MiniPartyInfo.lua", 0, 0, 0)) {
        size = file.size;
        buf = (char *)mem_alloc((size_t)size + 1);
        file_read(&file, buf, size);
        buf[size] = 0;
        file_close(&file);
        memset(name, 0, sizeof(name));
        state = 1;
        for (i = 0; i < size; i++) {
            entry.found = 1;
            switch (buf[i]) {
            case ',':
                if (!strstr(name, session_character_name(&g_session1)))
                    break;
                if (state == 2) {
                    i++;
                    entry.id = atoi(buf + i);
                    state = 3;
                } else if (state == 3) {
                    i++;
                    entry.cell = atoi(buf + i);
                    state = 1;
                }
                break;
            case '{': {
                char text[0x18];
                char *p = text;
                char c;

                i++;
                memset(name, 0, 0x17);
                memset(text, 0, sizeof(text));
                c = buf[i];
                while (c != ',') {
                    *p++ += c;
                    c = buf[i + 1];
                    i++;
                }
                strcpy_s(name, 0x18, text);
                if (state == 1) {
                    i++;
                    entry.found = atoi(buf + i);
                    state = 2;
                }
                break;
            }
            case '}':
                list_push_back(&self->plateRecords, &entry, sizeof entry);
                break;
            }
        }
        if (buf)
            mem_free(buf);
    }
    file_destruct(&file);
}

void session_init_table(struct Session *self)
{
    int i;

    session_init_job_hit_wave_name(self);
    session_load_mp3_names(self);
    session_load_indoor_rsws(self);
    session_load_view_points(self);
    session_init_skill_effect_id_maps(self);
    session_reload_texts(self);
    session_load_fog_parameters(self);
    session_init_lua_files(self);
    session_load_weapon_hit_waves(self);
    session_init_sprite_name_tables(self);
    session_init_pc_name_table(self);
    session_init_job_name_tables(self);
    session_load_shadow_factors(self);
    session_init_talk_type_table_(self);
    session_init_variables_(self);
    session_clear_character_lists(self);
    list_clear(&self->itemList, item_info_destroy);
    list_clear(&self->pingSamples, NULL);
    for (i = 0; i < EQUIP_SLOT_COUNT; i++)
        item_info_clear(&self->equipSlots[i]);
    session_init_short_cut_texts(self);
    session_load_buying_store_items(self);
    self->rectMapsScaled = 0;
    self->rectMapCount = -1;
    session_init_map_pos_table(self);
    session_load_exception_minimap_names(self);
}
