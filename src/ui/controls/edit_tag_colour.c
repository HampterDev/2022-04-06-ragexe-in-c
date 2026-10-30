#include "ui/controls/edit.h"
#include "ui/items/item_tags.h"

void ui_edit_ctrl3_set_tag_colour(struct UIEditCtrl3 *self, DWORD colour)
{
    size_t i;
    for (i = 0; i < self->tag_buttons.size; ++i) {
        struct UITextButton *button = &self->tag_buttons.items[i]->text;
        button->normal_colour = colour;
        button->text_colour = colour;
        button->hover_colour = colour;
    }
}
