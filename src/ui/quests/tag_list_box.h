#include "platform/language.h"
#include "platform/language_api.h"
#include "iteminfo/item_api.h"
#include "navi/navi_manager.h"
#ifndef UI_QUESTS_TAG_LIST_BOX_H
#define UI_QUESTS_TAG_LIST_BOX_H

#include "ui/quests/quests.h"
#include "ui/ui_std.h"

#define TAG_KIND_NAVI   0x1ae
#define TAG_KIND_ITEM   0x1c8
#define TAG_KIND_ITEML  0x1ed

struct TagPrototype {
    int id;
    struct String open;
    struct String close;
};

struct STagString {
    int prototype;
    int start;
    struct String text;
};
OFFSET_ASSERT(STagString, text, 0x08);

struct STagInfoV2 {
    int kind;
    int start;
    int end;
    struct String label;
    struct String args;
};

struct TagLabel {
    struct String label;
    struct String args;
};

extern struct Vec g_tagPrototypes;
#define TAG_PROTOTYPE(i) (&((struct TagPrototype *)g_tagPrototypes.first)[i])

struct UITagButton {
    struct UITextButton button;
    struct ItemInfo item;
};
struct UITagButton *ui_tag_button_create(void);
struct UITagButton *ui_tag_button_construct(struct UITagButton *self);

#define UI_TAG_LIST_BOX_SIZE 0xdc
struct UITagListBox {
    struct UIListBox box;
    struct Vec buttons;
};
struct TagLinkPlace {
    int line;
    int x;
};

struct Vec *tag_prototypes_construct(struct Vec *table);
void tag_links_free(struct Vec *links);
struct String *tag_list_box_split_tags(struct String *out, const struct String *text,
                                       struct Vec *links);
struct String *tag_text_strip_codes(struct String *out, const char *text, int len);
int tag_text_line_length(const char *text, int len, int width);
int tag_links_snap_to_link(int pos, const struct Vec *links);
void tag_item_link_decode(const struct String *code, struct Vec *vals);
BYTE tag_item_link_fill_item(const struct Vec *vals, struct ItemInfo *item);

void ui_tag_list_box_add_text(struct UITagListBox *self, const char *text, int width,
                                     int beside_bar, int wrap);
void ui_tag_list_box_on_link_click(struct UITagListBox *self, int kind,
                                         struct UITagButton *button);

struct NaviManager;
#define WND_ITEM_DESCRIBE 0xc
#define ITEM_DESCRIBE_MSG_SHOW 0x18
struct Language;

typedef unsigned char (*StdSortLessFn)(const void *a, const void *b, void *pred);

void tag_prototypes_free(struct Vec *v);

#endif
