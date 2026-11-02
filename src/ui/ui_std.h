#include "game/mode_misc.h"
#include "std/vec.h"
#include "std/bit_vector.h"
#include "ui/cooldown.h"
#include "std/string.h"
#include "iteminfo/item_list.h"
#include "session/state.h"
#ifndef UI_UI_STD_H
#define UI_UI_STD_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "core/fatal.h"
#include "base.h"
#include "std/std.h"
#include <time.h>

struct Mode;
struct HotkeyGuideItem;
struct ItemInfo;
struct List;
struct ListNode;
struct String;
struct Tree;
struct TreeNode;
struct Vec;
struct TagLink;

struct BitIter {
    DWORD *word;
    unsigned bit;
};
struct BitIter *bitvec_copy(struct BitIter *out, DWORD *first, unsigned first_bit, DWORD *last,
                                    unsigned last_bit, DWORD *dest, unsigned dest_bit);

#include <stdlib.h>

struct StrIntPair {
    struct String key;
    int value;
};

struct UiTagLink {
    int pos;
    int x, y;
    struct String label;
    struct String args;
};
void ui_tag_link_copy(struct UiTagLink *where, const struct UiTagLink *value);

struct UIMenuEntry {
    struct String text;
    int id;
};
struct UIMenuEntry *ui_menu_entry_construct(struct UIMenuEntry *self, const char *text,
                                                     int id);
void ui_menu_entry_vec_push_move(struct Vec *v, struct UIMenuEntry *entry);
void ui_menu_entry_vec_clear(struct Vec *v);
void ui_menu_entry_vec_free(struct Vec *v);
struct ItemInfo;

struct MailRef {
    int type;
    int id_align;
    long long id;
};
SIZE_ASSERT(MailRef, 0x10);
struct Tree;
BYTE rodex_inbox_mail_before(const struct MailRef *a, const struct MailRef *b);
void std_sort_mailrefs(struct MailRef *first, struct MailRef *last, int ideal);
void std_insertion_sort_mailrefs(struct MailRef *first, struct MailRef *last);
void std_make_heap_mailrefs(struct MailRef *first, struct MailRef *last);
void std_med3_mailrefs(struct MailRef *first, struct MailRef *mid, struct MailRef *last);
struct MailRefRange {
    struct MailRef *first, *last;
};
struct MailRefRange *std_partition_mailrefs(struct MailRefRange *out, struct MailRef *first,
                                                    struct MailRef *last);
void std_pop_heap_hole_mailrefs(struct MailRef *first, int hole, int bottom,
                                        const struct MailRef *value);

struct StdBitIter *std_vector_bool_erase(struct StdBitVector *self,
                                                    struct StdBitIter *out, unsigned *first_word,
                                                    unsigned first_bit, unsigned *last_word, unsigned last_bit);
struct StdBitIter *std_vector_bool_insert(struct StdBitVector *self,
                                                     struct StdBitIter *out, unsigned *where_word,
                                                     unsigned where_bit, size_t count, const BYTE *value);

void number_text(struct String *out, int value);

#endif
