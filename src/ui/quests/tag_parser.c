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

static void tag_string_destruct(struct STagString *self)
{
    string_free(&self->text);
}

static void tag_string_copy(struct STagString *self, const struct STagString *from)
{
    self->prototype = from->prototype;
    self->start = from->start;
    string_copy(&self->text, &from->text);
}

static void tag_strings_free(struct Vec *v)
{
    struct STagString *p;

    for (p = v->first; p != v->last; p++)
        tag_string_destruct(p);
    vec_free(v);
}

static void tag_link_destruct(struct STagInfoV2 *self)
{
    string_free(&self->args);
    string_free(&self->label);
}

static void tag_link_copy(struct STagInfoV2 *self, const struct STagInfoV2 *from)
{
    self->kind = from->kind;
    self->start = from->start;
    self->end = from->end;
    string_copy(&self->label, &from->label);
    string_copy(&self->args, &from->args);
}

static void tag_links_clear(struct Vec *links)
{
    struct STagInfoV2 *p;

    for (p = links->first; p != links->last; p++)
        tag_link_destruct(p);
    vec_clear(links);
}

void tag_links_free(struct Vec *links)
{
    tag_links_clear(links);
    vec_free(links);
}

static struct Vec *tag_text_find_tags(struct Vec *out, const struct String *text)
{
    struct TagPrototype *proto;
    int kind = 0;

    vec_init(out);
    for (proto = g_tagPrototypes.first; proto != g_tagPrototypes.last; proto++, kind++) {
        struct String rest;
        unsigned consumed = 0;

        string_copy(&rest, text);
        while (consumed < text->size) {
            int open_at, close_at;
            unsigned end, cut;

            open_at = text_view_find(string_view(&rest), string_view(&proto->open), 0);
            if (open_at == -1)
                break;
            close_at = text_view_find(string_view(&rest), string_view(&proto->close), 0);
            if (close_at == -1) {
                end = proto->open.size;
            } else {
                struct STagString tag, copy;

                end = close_at + proto->close.size;
                tag.prototype = kind;
                tag.start = consumed + open_at;
                string_init(&tag.text);
                string_substring(&tag.text, &rest, open_at, end - open_at);
                tag_string_copy(&copy, &tag);
                vec_push(out, &copy, sizeof copy);
                tag_string_destruct(&tag);
            }
            cut = end < rest.size ? end : rest.size;
            string_erase(&rest, 0, cut);
            consumed += end;
        }
        string_free(&rest);
    }
    return out;
}

static unsigned char tag_start_less(const void *a, const void *b, void *pred_unused)
{
    return ((const struct STagString *)a)->start < ((const struct STagString *)b)->start;
}

struct String *tag_item_name(struct String *out, int item_id)
{
    const struct ItemTableEntry *entry;

    entry = item_table_get(item_id);
    string_init(out);
    string_assign_cstr(out, entry->name);
    return out;
}

static struct TagLabel *tag_text_label(struct TagLabel *out, const struct String *text, int kind)
{
    const struct TagPrototype *proto;
    struct String part;
    int close_at;

    string_init(&out->label);
    string_init(&out->args);
    if (!text->size)
        return out;
    proto = TAG_PROTOTYPE(kind);
    if (text_view_find(string_view(text), string_view(&proto->open), 0) == STRING_NPOS)
        return out;
    close_at = text_view_find(string_view(text), string_view(&proto->close), 0);
    if (close_at == -1)
        return out;

    switch (proto->id) {
    case TAG_KIND_ITEML: {
        struct Vec vals = { NULL, NULL, NULL };
        struct String name;

        string_init(&part);
        string_substring(&part, text, 0, 1);
        string_free(&out->label);
        string_move(&out->label, &part);
        string_substring(&part, text, proto->open.size, close_at - proto->open.size);
        string_free(&out->args);
        string_move(&out->args, &part);
        tag_item_link_decode(&out->args, &vals);
        tag_item_name(&name, ((int *)vals.first)[2]);
        string_append(&out->label, string_cstr(&name), name.size);
        string_free(&name);
        string_substring(&part, text, text->size - 1, 1);
        string_append(&out->label, string_cstr(&part), part.size);
        string_free(&part);
        vec_free(&vals);
        break;
    }
    case TAG_KIND_NAVI:
    case TAG_KIND_ITEM: {
        int info_at, info_end;

        info_at = text_view_find(string_view(text), string_view(&text_tag_INFO), 0);
        if (info_at == -1)
            return out;
        info_end = text_view_find(string_view(text), string_view(&text_tag_INFO_END), 0);
        if (info_end == -1)
            return out;
        string_init(&part);
        string_substring(&part, text, proto->open.size, info_at - proto->open.size);
        string_free(&out->label);
        string_move(&out->label, &part);
        string_substring(&part, text, text_tag_INFO.size + info_at, info_end - text_tag_INFO.size - info_at);
        string_free(&out->args);
        string_move(&out->args, &part);
        break;
    }
    }
    return out;
}

struct String *tag_list_box_split_tags(struct String *out, const struct String *text, struct Vec *links)
{
    struct Vec tags;
    struct String plain;
    unsigned pos = 0, i, count;

    tag_text_find_tags(&tags, text);
    if (tags.first == tags.last) {
        string_copy(out, text);
        tag_strings_free(&tags);
        return out;
    }
    tag_links_clear(links);
    count = VEC_COUNT(&tags, struct STagString);
    std_sort(tags.first, tags.last, sizeof(struct STagString), tag_start_less, NULL);
    string_init(&plain);
    for (i = 0; i < count; i++) {
        struct STagString *tag = &((struct STagString *)tags.first)[i];
        struct STagInfoV2 info, copy;
        struct TagLabel label;
        struct String part;

        string_init(&info.label);
        string_init(&info.args);
        string_init(&part);
        string_substring(&part, text, pos, tag->start - pos);
        string_append(&plain, string_cstr(&part), part.size);
        string_free(&part);
        info.start = plain.size;
        tag_text_label(&label, &tag->text, tag->prototype);
        string_append(&plain, string_cstr(&label.label), label.label.size);
        info.end = plain.size;
        string_assign(&info.label, string_cstr(&label.label), label.label.size);
        string_assign(&info.args, string_cstr(&label.args), label.args.size);
        info.kind = TAG_PROTOTYPE(tag->prototype)->id;
        tag_link_copy(&copy, &info);
        vec_push(links, &copy, sizeof copy);
        pos = tag->start + tag->text.size;
        string_free(&label.args);
        string_free(&label.label);
        tag_link_destruct(&info);
    }
    if (pos < text->size - 1) {
        struct String part;

        string_init(&part);
        string_substring(&part, text, pos, text->size - pos);
        string_append(&plain, string_cstr(&part), part.size);
        string_free(&part);
    }
    *out = plain;
    tag_strings_free(&tags);
    return out;
}

struct String *tag_text_strip_codes(struct String *out, const char *text, int len)
{
    const char *p, *end;

    string_init(out);
    string_reserve(out, len);
    if (!text)
        return out;
    end = text + len;
    for (p = text; p < end;) {
        if (text_is_colour_code(p, end)) {
            p += 7;
        } else if (end - p >= 3 && text_find_charset_tag(p, 3, !g_useCharsetTag) == 0) {
            p += 3;
        } else {
            string_append_char(out, *p);
            p++;
        }
    }
    return out;
}

int tag_text_line_length(const char *text, int len, int width)
{
    const char *p = text, *end, *stop;

    if (!text)
        return -1;
    end = text + len;
    stop = text + width;
    while (p < stop && p < end) {
        if (*p == '\n')
            return (int)(p - text) + 1;
        if (text_is_colour_code(p, end)) {
            p += 7;
            stop += 7;
        } else if (end - p >= 3 && text_find_charset_tag(p, 3, !g_useCharsetTag) == 0) {
            p += 3;
            stop += 3;
        } else {
            p = language_char_next(p);
        }
    }
    return (int)(p - text);
}

int tag_links_snap_to_link(int pos, const struct Vec *links)
{
    const struct STagInfoV2 *link;

    for (link = links->first; link != links->last; link++)
        if (pos > link->start && pos < link->end)
            return link->start;
    return pos;
}

static const char item_link_separators[14] = "!#$%&'()*+,-/";
#define ITEM_LINK_VALUES 0x104
#define ITEM_LINK_CARDS 6
#define SEP_CARD 7
#define SEP_OPTION 9

static int item_link_base62(const struct String *text)
{
    static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    size_t size = text->size + 1;
    char *buf = mem_alloc(size);
    unsigned value = 0;
    int count, i;

    strcpy_s(buf, size, string_cstr(text));
    count = (int)size - 1;
    for (i = 0; i < count; i++) {
        unsigned place = 1;
        int digit, k;

        for (digit = 0; digit < 62 && digits[digit] != buf[i]; digit++)
            ;
        if (digit == 62)
            continue;
        for (k = count - i; k > 1; k--)
            place *= 62;
        value += place * digit;
    }
    if (buf)
        mem_free(buf);
    return (int)value;
}

static int item_link_cards(struct Vec *vals, const char *buf, unsigned *i, char sep, size_t len, int at)
{
    struct String field;
    struct Vec list = { NULL, NULL, NULL };
    int count = 1;

    string_init(&field);
    for (++*i; *i < len + 1; ++*i) {
        char c = buf[*i];

        if ((signed char)c >= '0') {
            string_append_char(&field, c);
        } else if (c == sep) {
            count++;
            *(int *)vec_add(&list, 1, sizeof(int)) = item_link_base62(&field);
            string_clear(&field);
        } else {
            unsigned j, n;

            --*i;
            *(int *)vec_add(&list, 1, sizeof(int)) = item_link_base62(&field);
            ((int *)vals->first)[at] = count;
            n = VEC_COUNT(&list, int);
            for (j = 0; j < n; j++)
                ((int *)vals->first)[at + 1 + j] = ((int *)list.first)[j];
            break;
        }
    }
    vec_free(&list);
    string_free(&field);
    return count;
}

static void item_link_option(struct Vec *out, const char *buf, unsigned *i, size_t len, char sep_a,
                             char sep_b)
{
    struct String field;

    string_init(&field);
    for (++*i; *i < len + 1; ++*i) {
        char c = buf[*i];

        if ((signed char)c >= '0') {
            string_append_char(&field, c);
        } else if (c == sep_a || c == sep_b) {
            *(int *)vec_add(out, 1, sizeof(int)) = item_link_base62(&field);
            string_clear(&field);
        } else {
            --*i;
            *(int *)vec_add(out, 1, sizeof(int)) = item_link_base62(&field);
            break;
        }
    }
    string_free(&field);
}

void tag_item_link_decode(const struct String *code, struct Vec *vals)
{
    size_t size = code->size + 1;
    struct String field;
    struct Vec options = { NULL, NULL, NULL };
    int option_count = 0, card_count = 0, slot = 2;
    unsigned i;
    char *buf;

    if ((int)size < 6)
        return;
    buf = mem_alloc(size);
    strcpy_s(buf, size, string_cstr(code));
    string_init(&field);
    vec_resize(vals, ITEM_LINK_VALUES, sizeof(int));
    ((int *)vals->first)[ITEM_LINK_CARDS] = -1;
    for (i = 0; i < 5; i++)
        string_append_char(&field, buf[i]);
    ((int *)vals->first)[0] = item_link_base62(&field);
    string_clear(&field);
    string_append_char(&field, buf[5]);
    ((int *)vals->first)[1] = item_link_base62(&field);
    string_clear(&field);

    for (i = 6; i < code->size + 1; i++) {
        char c = buf[i];
        int sep;

        if ((signed char)c >= '0') {
            string_append_char(&field, c);
            continue;
        }
        for (sep = 0; sep < (int)sizeof item_link_separators && item_link_separators[sep] != c; sep++)
            ;
        if (sep == (int)sizeof item_link_separators)
            continue;
        if (field.size)
            ((int *)vals->first)[slot] = item_link_base62(&field);
        if (sep == SEP_CARD) {
            card_count = item_link_cards(vals, buf, &i, item_link_separators[SEP_CARD], code->size,
                                         ITEM_LINK_CARDS);
        } else if (sep == SEP_OPTION) {
            option_count++;
            item_link_option(&options, buf, &i, code->size, ',', '-');
        }
        string_clear(&field);
        slot = sep;
    }
    if (option_count) {
        unsigned j, n = VEC_COUNT(&options, int);

        ((int *)vals->first)[card_count + 8] = option_count;
        for (j = 0; j < n; j++)
            ((int *)vals->first)[card_count + 9 + j] = ((int *)options.first)[j];
    }
    vec_free(&options);
    string_free(&field);
}

BYTE tag_item_link_fill_item(const struct Vec *vals, struct ItemInfo *item)
{
    const int *v = vals->first;
    unsigned n = VEC_COUNT(vals, int);
    const struct ItemTableEntry *entry;
    struct String name;
    int card_count = 0, i;
    BYTE filled;

    if (vals->first == vals->last)
        return 0;
    entry = item_table_get(v[2]);
    item_info_set_name(item, entry->name);
    string_init(&name);
    string_assign_cstr(&name, entry->name);
    text_to_lower(&name);
    if (string_find(&name, "unknown item", 0) != STRING_NPOS) {
        filled = 0;
    } else {
        item->equip_location = v[0];
        item_info_set_item_id(item, v[2]);
        if (v[1])
            item->type = 4;
        if (n > 3)
            item->refine = v[3];
        item->view = v[4];
        item->grade = (short)v[5];
        if (n > ITEM_LINK_CARDS) {
            card_count = v[ITEM_LINK_CARDS];
            for (i = 1; i < card_count + 1; i++) {
                int index = i - 1;

                item_info_set_slot(item, &v[ITEM_LINK_CARDS + i], &index);
            }
        }
        if ((unsigned)(card_count + 8) < n) {
            int options = v[card_count + 8];

            item->option_count = options;
            for (i = 1; i < options + 1; i++) {
                const int *o = &v[card_count + 9 + 3 * (i - 1)];
                int index = i - 1;

                item_info_set_option(item, (WORD)o[0] | ((DWORD)(WORD)o[2] << 16), (BYTE)o[1], &index);
            }
        }
        item->identified = 1;
        filled = 1;
    }
    string_free(&name);
    return filled;
}
