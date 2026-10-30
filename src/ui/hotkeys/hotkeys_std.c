#include <string.h>
#include "ui/hotkeys/hotkeys_std.h"

void key_cap_destruct(struct KeyCap *self)
{
    string_free(&self->label);
}

struct KeyCap *key_cap_construct(struct KeyCap *self, struct String label,
                                           struct KeyCapBox box0, struct KeyCapBox box1)
{
    string_init(&self->label);
    memset(self->boxes, 0, sizeof self->boxes);
    string_assign(&self->label, string_cstr(&label), label.size);
    self->boxes[0] = box0;
    self->boxes[1] = box1;
    string_free(&label);
    return self;
}

struct KeyCap *key_cap_assign(struct KeyCap *self, const struct KeyCap *other)
{
    if (self != other)
        string_assign(&self->label, string_cstr(&other->label), other->label.size);
    self->boxes[0] = other->boxes[0];
    self->boxes[1] = other->boxes[1];
    return self;
}
