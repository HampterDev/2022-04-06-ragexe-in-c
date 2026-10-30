#include <errno.h>
#include "core/fatal.h"
#include <malloc.h>
#include <mbctype.h>
#include <mbstring.h>
#include <stdlib.h>
#include <string.h>
#include "ui/ui_std.h"
#include "game/rodex_mail.h"
#include "ui/items/styling_shop_std.h"
#include "ui/social/guild_manage.h"

void ui_menu_entry_vec_push_move(struct Vec *v, struct UIMenuEntry *entry)
{
    vec_push(v, entry, sizeof *entry);
    string_init(&entry->text);
}

void ui_menu_entry_vec_clear(struct Vec *v)
{
    struct UIMenuEntry *p;

    for (p = v->first; p != v->last; p++)
        string_free(&p->text);
    vec_clear(v);
}

void ui_menu_entry_vec_free(struct Vec *v)
{
    ui_menu_entry_vec_clear(v);
    vec_free(v);
}

struct UIMenuEntry *ui_menu_entry_construct(struct UIMenuEntry *self, const char *text,
                                                     int id)
{
    string_init(&self->text);
    string_assign_cstr(&self->text, text);
    self->id = id;
    return self;
}

static const BYTE item_type_ranks[20] = {
    0, 0, 0, 6, 2, 3, 5, 6, 3, 3, 4, 2, 2, 2, 2, 3, 4, 4, 1, 4,
};
#define ITEM_TYPE_RANK_OTHER 10

int item_type_rank(int type)
{
    if ((unsigned)type > 19)
        return ITEM_TYPE_RANK_OTHER;
    return item_type_ranks[type];
}
