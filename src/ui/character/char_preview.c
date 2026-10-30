#include "ui/character/char_preview.h"

void char_preview_init(struct CharPreview *self)
{
    int i;

    for (i = 0; i < 5; i++) {
        string_init(&self->act[i]);
        string_init(&self->spr[i]);
    }
    string_init(&self->imf);
    string_init(&self->head_palette_name);
    string_init(&self->body_palette_name);
}

void char_preview_destroy(struct CharPreview *self)
{
    int i;

    string_free(&self->body_palette_name);
    string_free(&self->head_palette_name);
    string_free(&self->imf);
    for (i = 4; i >= 0; i--)
        string_free(&self->spr[i]);
    for (i = 4; i >= 0; i--)
        string_free(&self->act[i]);
}
