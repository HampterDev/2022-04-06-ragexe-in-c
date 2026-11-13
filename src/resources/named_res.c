#include "resources/named_res.h"
#include "fileio/resource_api.h"

void named_res_init(struct NamedRes *self)
{
    string_init(&self->name);
    self->res = NULL;
    self->looked_up = 0;
}

static void drop(struct NamedRes *self)
{
    if (self->res)
        res_unlock(self->res);
    self->res = NULL;
    self->looked_up = 0;
}

void named_res_free(struct NamedRes *self)
{
    drop(self);
    string_free(&self->name);
}

void named_res_assign(struct NamedRes *self, const char *text, size_t size)
{
    drop(self);
    string_assign(&self->name, text, size);
}

void named_res_take(struct NamedRes *self, struct String *name)
{
    drop(self);
    string_free(&self->name);
    string_move(&self->name, name);
}

const char *named_res_text(const struct NamedRes *self)
{
    return string_cstr(&self->name);
}

size_t named_res_size(const struct NamedRes *self)
{
    return self->name.size;
}

struct Res *named_res_get(struct NamedRes *self)
{
    if (!self->looked_up) {
        self->looked_up = 1;
        self->res = self->name.size ? res_mgr_get(g_resMgr(), string_cstr(&self->name)) : NULL;
        if (self->res)
            res_lock(self->res);
    }
    return self->res;
}
