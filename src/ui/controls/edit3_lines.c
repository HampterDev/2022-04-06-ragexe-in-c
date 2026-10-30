#include "ui/controls/edit.h"
#include "ui/items/item_tags.h"
#include <stdlib.h>
#include <string.h>

static void assign(struct OwnedText *text, struct TextView value)
{
    if (!owned_text_assign(text, value)) abort();
}

static void append(struct OwnedText *text, struct TextView value)
{
    if (!owned_text_concat(text, owned_text_view(text), value)) abort();
}

static void cut(struct OwnedText *text, size_t start, size_t count)
{
    if (start > text->size) start = text->size;
    if (count > text->size - start) count = text->size - start;
    assign(text, text_view(owned_text_cstr(text) + start, count));
}

static int find(const struct OwnedText *text, char character)
{
    const char *begin = owned_text_cstr(text);
    const char *found = memchr(begin, character, text->size);
    return found ? (int)(found - begin) : -1;
}

static void swap(struct OwnedText *a, struct OwnedText *b)
{
    struct OwnedText value = *a;
    *a = *b;
    *b = value;
}

static void push(struct TextArray *lines, const struct OwnedText *line)
{
    if (!text_array_append(lines, owned_text_view(line))) abort();
}

void ui_edit_ctrl3_rewrap(struct UIEditCtrl3 *self, struct TextArray *lines)
{
    struct OwnedText cur = {0};
    unsigned li;
    for (li = 0; li < lines->size; ++li) {
        struct TextArray sub = {0};
        unsigned n, j;
        int f, offset;
        ui_window_break_coloured_lines(&self->edit.base, text_array_at(lines, li).data,
                                     &sub, NULL, self->edit.base.width - 6);
        n = sub.size;
        if (n > 1) {
            for (j = 0; j < n; ++j) {
                struct OwnedText *piece = &sub.items[j];
                if (j == 0) assign(&lines->items[li], owned_text_view(piece));
                if (li + j < lines->size && j == n - 1) {
                    struct OwnedText *next = &lines->items[li + j];
                    append(piece, owned_text_view(next));
                    swap(next, piece);
                } else if (j != 0) {
                    struct TextArray rebuilt = {0};
                    unsigned k;
                    for (k = 0; k < lines->size; ++k)
                        if (lines->items[k].size) push(&rebuilt, &lines->items[k]);
                    if (piece->size) push(&rebuilt, piece);
                    for (k = li + 1; k < lines->size; ++k)
                        if (lines->items[k].size) push(&rebuilt, &lines->items[k]);
                    if (rebuilt.size) {
                        struct TextArray old = *lines;
                        *lines = rebuilt;
                        rebuilt = old;
                    }
                    text_array_clear(&rebuilt);
                }
                if (li > 0x28) break;
            }
        }
        if (li < lines->size) {
            assign(&cur, text_array_at(lines, li));
            offset = 0;
            while ((f = find(&cur, '>')) != -1) {
                offset += f + 1;
                cut(&cur, f + 1, (size_t)-1);
            }
            f = find(&cur, '<');
            if (f != -1) {
                struct OwnedText rest = {0};
                offset += f;
                cut(&cur, f, (size_t)-1);
                assign(&rest, text_view(owned_text_cstr(&cur) + 1, cur.size - 1));
                while ((f = find(&rest, '<')) != -1) {
                    offset += f;
                    assign(&cur, owned_text_view(&rest));
                    assign(&rest, text_view(owned_text_cstr(&cur) + 1, cur.size - 1));
                }
                if (li + 1 < lines->size && find(&lines->items[li + 1], '>') != -1) {
                    append(&cur, text_array_at(lines, li + 1));
                    swap(&lines->items[li + 1], &cur);
                    cut(&lines->items[li], 0, (unsigned)offset);
                }
                owned_text_clear(&rest);
            }
        }
        text_array_clear(&sub);
    }
    owned_text_clear(&cur);
}

struct String *ui_edit_ctrl3_join_lines(struct UIEditCtrl3 *self, struct String *out)
{
    size_t i;
    string_clear(out);
    for (i = 0; i < self->lines.size; ++i) {
        struct TextView line = text_array_at(&self->lines, i);

        string_append(out, line.data, line.size);
    }
    text_array_resize(&self->lines, 0);
    return out;
}

void ui_edit_ctrl3_place_tags(struct UIEditCtrl3 *self, const int *x, const int *y,
                          const int *first, const int *last, const int *line)
{
    size_t i;

    for (i = 0; i < self->tag_buttons.size; ++i) {
        struct UIItemTagButton *button = self->tag_buttons.items[i];
        if (button->line >= *first && button->line < *last) {
            if (*line == button->line)
                ui_window_move_base(&button->text.button.base, *x - 1 + button->line_x, *y);
        } else {
            ui_window_move_base(&button->text.button.base, -100, -100);
        }
    }
}
