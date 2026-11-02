#include "platform/code_page.h"
#include "core/text_format.h"
#include "core/markup_tags.h"
#include "ui/quests/tag_list_box.h"
#include "core/text_util.h"

struct Vec g_tagPrototypes;

static void tag_prototype_destruct(struct TagPrototype *self)
{
    string_free(&self->close);
    string_free(&self->open);
}

static void tag_prototype_copy(struct TagPrototype *self, const struct TagPrototype *from)
{
    self->id = from->id;
    string_copy(&self->open, &from->open);
    string_copy(&self->close, &from->close);
}

void tag_prototypes_free(struct Vec *v)
{
    struct TagPrototype *p;

    for (p = v->first; p != v->last; p++)
        tag_prototype_destruct(p);
    vec_free(v);
}

static void add_prototype(struct Vec *table, int id, const struct String *open,
                          const struct String *close)
{
    struct TagPrototype proto, copy;

    proto.id = id;
    string_copy(&proto.open, open);
    string_copy(&proto.close, close);
    tag_prototype_copy(&copy, &proto);
    vec_push(table, &copy, sizeof copy);
    tag_prototype_destruct(&proto);
}

struct Vec *tag_prototypes_construct(struct Vec *table)
{
    vec_init(table);
    add_prototype(table, TAG_KIND_NAVI, &text_tag_NAVI, &text_tag_NAVI_END);
    add_prototype(table, TAG_KIND_ITEM, &text_tag_ITEM, &text_tag_ITEM_END);
    add_prototype(table, TAG_KIND_ITEML, &text_tag_ITEML, &text_tag_ITEML_END);
    return table;
}
