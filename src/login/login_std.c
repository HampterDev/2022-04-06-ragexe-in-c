#include <stdlib.h>
#include "core/fatal.h"
#include "login/loginmode.h"
#include "std/std.h"
#include <string.h>

#include <errno.h>

struct LoginConnEntry *login_conn_entry_construct(struct LoginConnEntry *self)
{
    string_init(&self->display);
    string_init(&self->desc);
    string_init(&self->balloon);
    string_init(&self->address);
    string_init(&self->port);
    return self;
}

struct LoginConnEntry *login_conn_entry_copy(struct LoginConnEntry *self,
                                                        const struct LoginConnEntry *src)
{
    login_conn_entry_construct(self);
    string_assign(&self->display, string_cstr(&src->display), src->display.size);
    string_assign(&self->desc, string_cstr(&src->desc), src->desc.size);
    string_assign(&self->address, string_cstr(&src->address), src->address.size);
    string_assign(&self->port, string_cstr(&src->port), src->port.size);
    string_assign(&self->balloon, string_cstr(&src->balloon), src->balloon.size);
    return self;
}

static void conn_entry_destroy(struct LoginConnEntry *e)
{
    string_free(&e->port);
    string_free(&e->address);
    string_free(&e->balloon);
    string_free(&e->desc);
    string_free(&e->display);
}

void login_conn_entries_free(struct Vec *v)
{
    struct LoginConnEntry *e;

    for (e = (struct LoginConnEntry *)v->first; e != (struct LoginConnEntry *)v->last; e++)
        conn_entry_destroy(e);
    vec_free(v);
}
