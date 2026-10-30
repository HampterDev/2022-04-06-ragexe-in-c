#include "ui/controls/buttons.h"
#include <string.h>

void ui_sprite_button_set_sprite(struct UIWindow *wnd, const char *path)
{
    struct UISpriteButton *self = ((struct UISpriteButton *)wnd);
    struct String base = {0};
    size_t length = strlen(path);

    if (length < 3)
        return;
    string_assign(&base, path, length - 3);
    string_assign(&self->spr, string_cstr(&base), base.size);
    string_append_cstr(&self->spr, "spr");
    string_assign(&self->act, string_cstr(&base), base.size);
    string_append_cstr(&self->act, "act");
    string_free(&base);
}
