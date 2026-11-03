#include "std/string.h"
#include "fileio/resource_api.h"
#include "ui/skin.h"
#include "ui/ui_std.h"

#include <stdlib.h>
#include <string.h>
#include "core/skin_path.h"
#include "ui/skin_manager.h"

struct SkinMgr g_skinMgr;
static struct String basic_skin_name;

static void skin_name_destroy(void)
{
    skin_mgr_destruct(&g_skinMgr);
}

static void basic_skin_name_destroy(void)
{
    string_free(&basic_skin_name);
}

void skin_name_init(void)
{
    struct Vec *vectors[] = {
        &g_skinMgr.names, &g_skinMgr.paths
    };
    int i;

    for (i = 0; i < 2; i++)
        vectors[i]->first = vectors[i]->last = vectors[i]->end = NULL;
    string_assign(&g_skinMgr.name, string_cstr(&basic_skin_name), basic_skin_name.size);
    atexit(skin_name_destroy);
}

void basic_skin_name_init(void)
{
    string_assign_cstr(&basic_skin_name, "<Basic Skin>");
    atexit(basic_skin_name_destroy);
}

void skin_mgr_destruct(struct SkinMgr *self)
{
    ui_skin_path_set_skin(NULL);
    string_vec_free(&self->paths);
    string_vec_free(&self->names);
    string_free(&self->name);
}

void ui_skin_color_chip(struct SkinMgr *self, int x, int y, int *red, int *green,
                                 int *blue)
{
    struct BitmapRes *chip = res_mgr_get(g_resMgr(), ui_skin_path(UI_SKIN "colorchip.bmp"));
    DWORD c = bitmap_res_get_pixel(chip, x, y);

    *red = (BYTE)(c >> 16);
    *green = (BYTE)(c >> 8);
    *blue = (BYTE)c;
}

static const char *entry_or_default(const struct Vec *v, int index)
{
    if (index == -1)
        return string_cstr(&basic_skin_name);
    return string_cstr((struct String *)v->first + index);
}

const char *skin_mgr_get_current_name(struct SkinMgr *self)
{
    return entry_or_default(&self->names, self->current);
}

const char *skin_mgr_get_current_path(struct SkinMgr *self)
{
    return entry_or_default(&self->paths, self->current);
}

/* Hands the selected folder to ui_skin_path; g_skinMgr is the only skin manager. */
static void publish_skin(struct SkinMgr *self)
{
    ui_skin_path_set_skin(VEC_COUNT(&self->paths, struct String) != 0 && self->current != -1
                          ? skin_mgr_get_current_path(self) : NULL);
}

const char *skin_mgr_get_name(struct SkinMgr *self, int index)
{
    return entry_or_default(&self->names, index);
}

static void push_string(struct Vec *v, const char *text)
{
    string_vec_push_text(v, text, strlen(text));
}

void skin_mgr_scan(struct SkinMgr *self)
{
    WIN32_FIND_DATAA fd;
    struct String dot, dotdot, path;
    struct String *s;
    HANDLE find;
    int count = 0;

    for (s = self->paths.first; s != self->paths.last; s++)
        string_free(s);
    self->paths.last = self->paths.first;
    for (s = self->names.first; s != self->names.last; s++)
        string_free(s);
    self->names.last = self->names.first;
    self->current = -1;
    memset(&fd, 0, sizeof fd);
    find = FindFirstFileA(".\\skin\\*.*", &fd);
    if (find == INVALID_HANDLE_VALUE) {
        FindClose(find);
        publish_skin(self);
        return;
    }
    string_init(&dot);
    string_assign_cstr(&dot, ".");
    string_init(&dotdot);
    string_assign_cstr(&dotdot, "..");
    do {
        if (string_equal_cstr(&dot, fd.cFileName) || string_equal_cstr(&dotdot, fd.cFileName))
            continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            string_init(&path);
            string_assign_cstr(&path, "..\\..\\skin\\");
            string_append_cstr(&path, fd.cFileName);
            string_append_cstr(&path, "\\");
            push_string(&self->paths, string_cstr(&path));
            push_string(&self->names, fd.cFileName);
            if (string_equal_cstr(&self->name, fd.cFileName))
                self->current = count;
            count++;
            string_free(&path);
        }
    } while (FindNextFileA(find, &fd));
    FindClose(find);
    string_free(&dotdot);
    string_free(&dot);
    publish_skin(self);
}

void skin_mgr_set_current_skin_num(struct SkinMgr *self, int index)
{
    int old = self->current;

    self->current = index;
    publish_skin(self);
    if (old != index)
        res_mgr_purge_by_extension(g_resMgr(), "bmp");
}
