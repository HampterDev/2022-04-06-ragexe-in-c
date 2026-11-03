#include "std/string.h"
#include "fileio/resource_api.h"
#include "ui/skin.h"
#include "ui/ui_std.h"

#include <direct.h>
#include <io.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/file_util.h"
#include "core/skin_path.h"
#include "core/text_util.h"
#include "core/app.h"

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
        &g_skinMgr.names, &g_skinMgr.paths, &g_skinMgr.extracted, &g_skinMgr.is_zip
    };
    int i;

    for (i = 0; i < 4; i++)
        vectors[i]->first = vectors[i]->last = vectors[i]->end = NULL;
    string_assign(&g_skinMgr.name, string_cstr(&basic_skin_name), basic_skin_name.size);
    atexit(skin_name_destroy);
}

void basic_skin_name_init(void)
{
    string_assign_cstr(&basic_skin_name, "<Basic Skin>");
    atexit(basic_skin_name_destroy);
}

#define SKIN_TMP_FORMAT "%s\\_tmpskin_"

static void delete_tmp_skin_folder(void)
{
    char path[0x800];

    sprintf(path, SKIN_TMP_FORMAT, g_clientDir);
    if (_access(path, 0) == 0)
        dir_remove_tree(path);
}

void skin_mgr_destruct(struct SkinMgr *self)
{
    delete_tmp_skin_folder();
    vec_free(&self->is_zip);
    vec_free(&self->extracted);
    string_vec_free(&self->paths);
    string_vec_free(&self->names);
    string_free(&self->name);
}

static void tmp_skin_path(char *path, const struct String *name)
{
    sprintf(path, SKIN_TMP_FORMAT, g_clientDir);
    strcat(path, "\\");
    strcat(path, string_cstr(name));
}

void skin_mgr_extract_files_if_zip_skin(struct SkinMgr *self)
{
    char path[0x800], opt_n[0x80], zip[0x80], opt_d[0x80], dest[0x80];
    const struct String *name;
    int cur = self->current;

    if (cur == -1 || !((int *)self->is_zip.first)[cur] || ((int *)self->extracted.first)[cur])
        return;
    name = (struct String *)self->names.first + cur;
    tmp_skin_path(path, name);
    if (_access(path, 0) == 0)
        return;
    sprintf(path, SKIN_TMP_FORMAT, g_clientDir);
    if (_access(path, 0) != 0 && _mkdir(path) != 0)
        return;
    tmp_skin_path(path, name);
    if (_access(path, 0) != 0 && _mkdir(path) != 0)
        return;
    strcpy(opt_n, "-n");
    strcpy(zip, "skin/");
    strcat(zip, string_cstr(name));
    strcpy(opt_d, "-d");
    strcpy(dest, "_tmpskin_/");
    strcat(dest, string_cstr(name));
    _spawnl(_P_WAIT, "runzip.exe", "runzip.exe", opt_n, zip, opt_d, dest, NULL);
    ((int *)self->extracted.first)[self->current] = 1;
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

const char *skin_mgr_get_name(struct SkinMgr *self, int index)
{
    return entry_or_default(&self->names, index);
}

static void push_string(struct Vec *v, const char *text)
{
    string_vec_push_text(v, text, strlen(text));
}

static void drop_extension(struct String *s)
{
    string_resize(s, s->size - 4, 0);
}

void skin_mgr_scan(struct SkinMgr *self)
{
    WIN32_FIND_DATAA fd;
    struct String dot, dotdot, path, ext, name;
    struct String *s;
    const char *extension;
    HANDLE find;
    int count = 0, zero = 0, one = 1;

    for (s = self->paths.first; s != self->paths.last; s++)
        string_free(s);
    self->paths.last = self->paths.first;
    for (s = self->names.first; s != self->names.last; s++)
        string_free(s);
    self->names.last = self->names.first;
    self->is_zip.last = self->is_zip.first;
    self->current = -1;
    memset(&fd, 0, sizeof fd);
    find = FindFirstFileA(".\\skin\\*.*", &fd);
    if (find == INVALID_HANDLE_VALUE) {
        FindClose(find);
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
            vec_push(&self->is_zip, &zero, sizeof zero);
            vec_push(&self->extracted, &zero, sizeof zero);
            if (string_equal_cstr(&self->name, fd.cFileName))
                self->current = count;
            count++;
            string_free(&path);
            continue;
        }
        /* A file without an extension is not a zip skin: skip it like any other file. */
        extension = str_find_last(fd.cFileName, '.');
        if (!extension)
            continue;
        string_init(&ext);
        string_assign_cstr(&ext, extension);
        text_to_lower(&ext);
        if (string_equal_cstr(&ext, ".zip")) {
            string_init(&path);
            string_assign_cstr(&path, "..\\..\\_tmpskin_\\");
            string_append_cstr(&path, fd.cFileName);
            drop_extension(&path);
            string_append_cstr(&path, "\\");
            push_string(&self->paths, string_cstr(&path));
            string_init(&name);
            string_assign_cstr(&name, fd.cFileName);
            drop_extension(&name);
            push_string(&self->names, string_cstr(&name));
            vec_push(&self->is_zip, &one, sizeof one);
            vec_push(&self->extracted, &zero, sizeof zero);
            if (string_equal(&self->name, &name))
                self->current = count;
            count++;
            string_free(&name);
            string_free(&path);
        }
        string_free(&ext);
    } while (FindNextFileA(find, &fd));
    FindClose(find);
    string_free(&dotdot);
    string_free(&dot);
}

void skin_mgr_set_current_skin_num(struct SkinMgr *self, int index)
{
    int old = self->current;

    self->current = index;
    if (old != index)
        res_mgr_purge_by_extension(g_resMgr(), "bmp");
}
