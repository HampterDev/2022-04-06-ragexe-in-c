#include "session/csession.h"
#include "session/session_items.h"

#include <string.h>

struct PartyMember *party_member_info_copy(struct PartyMember *self,
                                                    const struct PartyMember *src)
{
    self->found = src->found;
    self->aid = src->aid;
    self->char_id = src->char_id;
    string_copy(&self->name, &src->name);
    string_copy(&self->map_name, &src->map_name);
    self->role = src->role;
    self->state = src->state;
    self->color = src->color;
    self->job = src->job;
    self->level = src->level;
    self->dead = src->dead;
    return self;
}

struct PartyMember *party_member_assign(struct PartyMember *self, const struct PartyMember *src)
{
    self->found = src->found;
    self->aid = src->aid;
    self->char_id = src->char_id;
    string_assign(&self->name, string_cstr(&src->name), src->name.size);
    string_assign(&self->map_name, string_cstr(&src->map_name), src->map_name.size);
    self->role = src->role;
    self->state = src->state;
    self->color = src->color;
    self->job = src->job;
    self->level = src->level;
    self->dead = src->dead;
    return self;
}

void names_resize(struct Vec *v, size_t count, const char *fill)
{
    size_t i = vec_count(v, sizeof fill);

    vec_resize(v, count, sizeof fill);
    for (; i < count; i++)
        ((const char **)v->first)[i] = fill;
}
