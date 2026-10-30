#include "effects/rag/rag_effect.h"
#include "resources/ezeffect.h"
#include "effects/particle.h"
#include "fileio/resource_api.h"
#include <stdlib.h>
#include "effects/str_path.h"
#include <string.h>
#include "std/std.h"

static int is_word_char(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

static int split_str_path(const char *text, size_t length, size_t *folder_length)
{
    size_t slash = length, i, word;

    while (slash > 0 && text[slash - 1] != '\\')
        slash--;
    if (slash < 2)
        return 0;
    for (i = 0; i < slash - 1; i++)
        if (text[i] == '\n' || text[i] == '\r')
            return 0;
    if (length - slash < 5 || memcmp(text + length - 4, ".str", 4) != 0)
        return 0;
    for (word = slash; word < length - 4; word++)
        if (!is_word_char(text[word]))
            return 0;
    *folder_length = slash;
    return 1;
}

struct String *ez2str_min_path(struct String *out, const char *path)
{
    const char *text = path;
    size_t length = strlen(path);
    size_t folder_length;

    if (split_str_path(text, length, &folder_length)) {
        struct String candidate;

        string_init(&candidate);
        string_assign(&candidate, text, folder_length);
        string_append_cstr(&candidate, "min_");
        string_append(&candidate, text + folder_length, length - folder_length);

        if (res_mgr_is_available(g_resMgr(), string_cstr(&candidate)) == 1) {
            string_move(out, &candidate);
            return out;
        }
        string_free(&candidate);
    }
    string_init(out);
    string_assign(out, path, length);
    return out;
}

void ez2_str_particle_load(struct Ez2StrParticle *self, const char *path)
{
    char dir[256];

    if (self->res)
        res_unlock(&self->res->res);
    self->res = res_mgr_get(g_resMgr(), path);
    self->anim = self->res ? &self->res->anim : NULL;
    if (self->res)
        res_lock(&self->res->res);
    memset(dir, 0, sizeof dir);
    _splitpath(path, NULL, dir, NULL, NULL);
    string_assign_cstr(&self->path, path);
    string_assign_cstr(&self->texture_set, dir);
    ez2_str_particle_rewind(self);
}

void ez2_str_particle_open(struct Ez2StrParticle *self, const char *path)
{
    if (g_session1.map_rule_flags[0]) {
        struct String chosen;

        ez2str_min_path(&chosen, path);
        ez2_str_particle_load(self, string_cstr(&chosen));
        string_free(&chosen);
    } else {
        ez2_str_particle_load(self, path);
    }
}

void rag_str_unload(struct RagEffect *self)
{
    if (self->str_res)
        res_unlock(&self->str_res->res);
    self->str_res = NULL;
    self->str_anim = NULL;
    self->str_layers_done = self->str_layers_to_play = 0;
    memset(self->str_drawn, 0, sizeof self->str_drawn);
}

void rag_str_load(struct RagEffect *self, const char *name)
{
    rag_str_unload(self);
    self->str_res = res_mgr_get(g_resMgr(), name);
    if (!self->str_res)
        return;
    res_lock(&self->str_res->res);
    self->str_anim = &self->str_res->anim;
    rag_str_rewind(self);
}
