#ifndef UI_SKIN_MANAGER_H
#define UI_SKIN_MANAGER_H

#include "base.h"

struct SkinMgr {
    struct String name;
    int current;
    struct Vec names;
    struct Vec paths;
    struct Vec extracted;
    struct Vec is_zip;
};

extern struct SkinMgr g_skinMgr;

void skin_mgr_destruct(struct SkinMgr *self);
void skin_mgr_extract_files_if_zip_skin(struct SkinMgr *self);
const char *skin_mgr_get_current_name(struct SkinMgr *self);
const char *skin_mgr_get_current_path(struct SkinMgr *self);
const char *skin_mgr_get_name(struct SkinMgr *self, int index);
void skin_mgr_scan(struct SkinMgr *self);
void skin_mgr_set_current_skin_num(struct SkinMgr *self, int index);
void ui_skin_color_chip(struct SkinMgr *self, int x, int y, int *red, int *green, int *blue);

void skin_name_init(void);
void basic_skin_name_init(void);

#endif
