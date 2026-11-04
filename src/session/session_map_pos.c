#include "session/csession.h"
#include "session/session_state.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ui/ui_std.h"

void named_map_rect_destroy(void *value)
{
    struct NamedMapRect *entry = value;

    string_free(&entry->name);
}

static int name_less(const void *a, const void *b)
{
    return string_compare(a, b) < 0;
}

static int atoi_between(const char *start, const char *p)
{
    struct String s;
    int value;

    string_init(&s);
    string_assign(&s, start, (size_t)(p - start));
    value = atoi(string_cstr(&s));
    string_free(&s);
    return value;
}

static struct MapRect *named_rect_at(struct Tree *map, const struct String *name)
{
    struct NamedMapRect *entry = tree_find(map, name, name_less);
    struct NamedMapRect made;

    if (!entry) {
        string_copy(&made.name, name);
        memset(&made.rect, 0, sizeof made.rect);
        entry = tree_add(map, &made, sizeof made, name_less, NULL);
    }
    return &entry->rect;
}

static void set_group_box(struct Session *self, int group, const struct MapRect *box)
{
    struct GroupMapRect *entry = tree_get(&self->mapPosGroupBoxes, &group, sizeof group, sizeof *entry,
                                          tree_less_int);

    entry->rect = *box;
}

static void make_rect_maps(struct Session *self, const char *start, const char *p)
{
    struct Tree *maps = NULL;
    int count, i;

    self->rectMapCount = atoi_between(start, p);
    count = self->rectMapCount;
    if (count > 0 && (size_t)count <= SIZE_MAX / sizeof *maps)
        maps = mem_alloc((size_t)count * sizeof *maps);
    if (maps)
        for (i = 0; i < count; i++)
            tree_init(&maps[i]);
    self->rectMaps = maps;
}

int session_init_map_pos_table(struct Session *self)
{
    struct Tree *names = &self->mapPosRects;
    struct String name, group, field;
    struct Vec fields;
    struct File file;
    struct MapRect box, entry;
    const char *p, *start, *end;
    int groupId, result;

    self->rectMaps = NULL;
    tree_clear(names, named_map_rect_destroy);
    tree_clear(&self->mapPosGroupBoxes, NULL);
    file_construct(&file);
    if (!file_open(&file, "mapPosTable.txt", 0, 1, 0)) {
        result = 0;
    } else {
        end = (const char *)file.buffer + file.size;
        groupId = 0;
        string_init(&name);
        fields.first = fields.last = fields.end = NULL;
        string_init(&group);
        memset(&box, 0, sizeof(box));
        for (start = p = (const char *)file.buffer; p < end; p++) {
            switch (*p) {
            case '\n':
            case '\r':
                start = p + 1;
                break;
            case '@':
                if (self->rectMapCount == -1)
                    make_rect_maps(self, start, p);
                break;
            case '#':
                if (group.size == 0) {
                    int id;

                    string_assign(&group, start, (size_t)(p - start));
                    id = atoi_between(start, p);
                    if (groupId != id && box.set) {
                        struct MapRect empty;

                        set_group_box(self, groupId, &box);
                        memset(&empty, 0, sizeof(empty));
                        box = empty;
                        groupId = id;
                    }
                } else if (name.size == 0) {
                    string_assign(&name, start, (size_t)(p - start));
                } else if (VEC_COUNT(&fields, struct String) < 3) {
                    string_init(&field);
                    string_assign(&field, start, (size_t)(p - start));
                    string_vec_push_move(&fields, &field);
                    string_free(&field);
                } else {
                    const struct String *f;
                    int left, top, right, bottom;

                    string_init(&field);
                    string_assign(&field, start, (size_t)(p - start));
                    string_vec_push_move(&fields, &field);
                    string_free(&field);
                    f = (const struct String *)fields.first;
                    bottom = atoi(string_cstr(&f[3]));
                    right = atoi(string_cstr(&f[2]));
                    top = atoi(string_cstr(&f[1]));
                    left = atoi(string_cstr(&f[0]));
                    entry.rect.left = left;
                    entry.rect.top = top;
                    entry.rect.right = right;
                    entry.rect.bottom = bottom;
                    entry.set = 1;
                    *named_rect_at(&self->rectMaps[groupId], &name) = entry;
                    *named_rect_at(names, &name) = entry;
                    if (box.set) {
                        if (right > box.rect.right)
                            box.rect.right = right;
                        if (bottom > box.rect.bottom)
                            box.rect.bottom = bottom;
                        if (left < box.rect.left)
                            box.rect.left = left;
                        if (top < box.rect.top)
                            box.rect.top = top;
                    } else {
                        box.rect = entry.rect;
                        box.set = 1;
                    }
                    string_clear(&group);
                    string_clear(&name);
                    string_vec_clear(&fields);
                }
                start = p + 1;
                break;
            default:
                break;
            }
        }
        if (box.set)
            set_group_box(self, groupId, &box);
        file_close(&file);
        result = 1;
        string_free(&group);
        string_vec_free(&fields);
        string_free(&name);
    }
    file_destruct(&file);
    return result;
}
