#ifndef UI_SKIN_H
#define UI_SKIN_H

#include "fileio/resource_api.h"

#include "ui/window.h"
#include "ui/skin_manager.h"

#define UI_SKIN "유저인터페이스\\"

#include "resources/bitmap.h"
#include "core/skin_path.h"
static __inline struct BitmapRes *ui_skin_bitmap(const char *path)
{
    return (struct BitmapRes *)res_mgr_get(g_resMgr(), ui_skin_path(path));
}

static __inline COLORREF ui_skin_colour(int x, int y)
{
    int r, g, b;

    ui_skin_color_chip(&g_skinMgr, x, y, &r, &g, &b);
    return RGB(r, g, b);
}
static __inline DWORD ui_skin_argb(int x, int y)
{
    int r, g, b;

    ui_skin_color_chip(&g_skinMgr, x, y, &r, &g, &b);
    return 0xff000000u | RGB(r, g, b);
}

#endif
