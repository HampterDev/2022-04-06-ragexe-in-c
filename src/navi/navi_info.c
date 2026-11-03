#include "render/texture_cache.h"
#include "render/texture_image.h"
#include "session/state.h"
#include "core/object_type.h"
#include "navi/navi_manager.h"
#include "game/cells.h"
#include "std/string.h"
#include "contents/client_lua_calls.h"
#include "world/pathfinder_state.h"
#include "navi/navi_search.h"
#include "resources/attr.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include <mmsystem.h>
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"

#include <windows.h>
#include <string.h>
#include <wchar.h>
#include "core/msg_str.h"
#include "session/session_names.h"

struct NaviUiRecord *navi_ui_record_construct(struct NaviUiRecord *self, int style,
        int x, int y, const char *text, unsigned colour, unsigned colour2)
{
    self->x = 0;
    self->y = 0;
    string_init(&self->text);
    self->style = style;
    self->x = x;
    self->y = y;
    string_assign_cstr(&self->text, text);
    self->colour = colour;
    self->colour2 = colour2;
    return self;
}

void navi_ui_record_destroy(struct NaviUiRecord *self)
{
    string_clear(&self->text);
    string_free(&self->text);
}

static void push_line(struct NaviManager *self, const struct NaviUiRecord *rec)
{
    struct NaviUiRecord copy;

    copy.style = rec->style;
    copy.x = rec->x;
    copy.y = rec->y;
    string_copy(&copy.text, &rec->text);
    copy.colour = rec->colour;
    copy.colour2 = rec->colour2;
    vec_push(&self->infoLines, &copy, sizeof copy);
}

static void add_line(struct NaviManager *self, int style, int x, int y, const char *text)
{
    struct NaviUiRecord rec;

    navi_ui_record_construct(&rec, style, x, y, text, 0xdcdcdc, 0xcb6b54);
    push_line(self, &rec);
    navi_ui_record_destroy(&rec);
}

int navi_manager_add_text_lines(struct NaviManager *self, int style, int x, int y,
        const char *text)
{
    struct String s;
    wchar_t wide[100];
    char line[40];
    struct NaviUiRecord *last;
    int len, lines;

    string_init(&s);
    string_assign_cstr(&s, "[");
    string_append_cstr(&s, text);
    memset(wide, 0, 0xc8);
    MultiByteToWideChar(CP_UTF8, 0, string_cstr(&s), -1, wide, s.size);
    len = (int)wcsnlen(wide, 100);
    memset(line, 0, sizeof line);
    lines = len / 12;
    if (lines >= 0) {
        const wchar_t *start = wide;
        int i, dy = 0, end = 11;

        for (i = 0; i <= lines; i++, dy += 14, end += 11, start += 11) {
            const wchar_t *stop = wide + (len <= end ? len : end);
            struct WString ws;
            struct NaviUiRecord rec;

            wstring_init(&ws);
            wstring_assign(&ws, start, (size_t)(stop - start));
            WideCharToMultiByte(CP_UTF8, 0, wstring_cstr(&ws), -1, line, 0x28, NULL, NULL);
            rec.x = 0;
            rec.y = 0;
            string_init(&rec.text);
            rec.style = style;
            rec.x = x;
            rec.y = y + dy;
            string_assign_cstr(&rec.text, line);
            rec.colour = 0xdcdcdc;
            rec.colour2 = 0xcb6b54;
            push_line(self, &rec);
            navi_ui_record_destroy(&rec);
            wstring_free(&ws);
        }
    }
    last = (struct NaviUiRecord *)self->infoLines.first
         + (VEC_COUNT(&self->infoLines, struct NaviUiRecord) - 1);
    string_append_cstr(&last->text, "]");
    string_free(&s);
    return lines + 1;
}

#define MOB_TRAIT_ELEMENT(traits)   ((unsigned char)((traits) >> 16))
#define MOB_TRAIT_RACE(traits)      ((unsigned char)(traits))
#define MOB_TRAIT_SIZE(traits)      ((traits) & 0xff00)

static const char *mob_element_msg(int element)
{
    switch (element % 10) {
    case 1: return msg_str(0x8d7);
    case 2: return msg_str(0x8d8);
    case 3: return msg_str(0x8d9);
    case 4: return msg_str(0x8da);
    case 5: return msg_str(0x8db);
    case 6: return msg_str(0x8dc);
    case 7: return msg_str(0x8dd);
    case 8: return msg_str(0x8de);
    case 9: return msg_str(0x8df);
    default: return msg_str(0x8e0);
    }
}

static void mob_race_line(char *buf, int race)
{
    int id;

    switch (race) {
    case 1: id = 0x8e4; break;
    case 2: id = 0x8e5; break;
    case 3: id = 0x8e6; break;
    case 4: id = 0x8e7; break;
    case 5: id = 0x8e8; break;
    case 6: id = 0x8e9; break;
    case 7: id = 0x8ea; break;
    case 8: id = 0x8eb; break;
    case 9: id = 0x8ec; break;
    default: id = 0x8ed; break;
    }
    sprintf(buf, msg_str(id), id);
}

static void mob_size_line(char *buf, int traits)
{
    int id;

    if (MOB_TRAIT_SIZE(traits) == 0x100)
        id = 0x8e1;
    else if (MOB_TRAIT_SIZE(traits) == 0x200)
        id = 0x8e2;
    else
        id = 0x8e3;
    sprintf(buf, msg_str(id), id);
}

static unsigned char guided_info(struct NaviManager *self)
{
    void *obj;
    char buf[50];
    int kind;

    navi_manager_clear_info_lines(self);
    obj = self->guided.obj;
    if (!obj)
        return 0;
    memset(buf, 0, 0x32);
    kind = self->guided.kind;

    if (kind == NAVI_RESULT_MAP) {
        navi_manager_add_text_lines(self, 1, 0x8d, 0x2e, navi_map_display_name(obj));
    } else if (kind == NAVI_RESULT_NPC) {
        struct NaviPoint a, b;
        int cy;

        navi_manager_add_text_lines(self, 2, 0x8d, 0x2e, navi_object_name(obj));
        cy = navi_object_position(obj, &a)->y;
        sprintf(buf, "( %d %d )", navi_object_position(obj, &b)->x, cy);
        add_line(self, 2, 0x8e, 0x95, buf);
    } else if (kind == NAVI_RESULT_MOB) {
        struct NaviPoint pos;
        struct NaviUiRecord rec;
        int traits;

        navi_manager_add_text_lines(self, 2, 0x8d, 0x2e, navi_object_name(obj));
        sprintf(buf, "Lv. %d", navi_object_position(obj, &pos)->x);
        add_line(self, 2, 0x8e, 0x4d, buf);

        traits = navi_object_position(obj, &pos)->y;
        sprintf(buf, mob_element_msg(MOB_TRAIT_ELEMENT(traits)), MOB_TRAIT_ELEMENT(traits) / 20);
        add_line(self, 2, 0x8e, 0x5d, buf);
        mob_race_line(buf, MOB_TRAIT_RACE(traits));
        add_line(self, 2, 0x8e, 0x6d, buf);
        mob_size_line(buf, traits);
        add_line(self, 2, 0x8e, 0x7d, buf);

        if (navi_object_type(obj) == NAVI_TYPE_MOB_BOSS) {
            navi_ui_record_construct(&rec, 2, 0x8e, 0x95, msg_str(0x8b6), 0xc8c8c8, 0xff);
            push_line(self, &rec);
            navi_ui_record_destroy(&rec);
        }
    } else if (kind == NAVI_RESULT_PLACE) {
        navi_manager_add_text_lines(self, 1, 0x8d, 0x2e, navi_map_name(obj));
        sprintf(buf, "( %d %d )", self->guided.x, self->guided.y);
        add_line(self, 1, 0x8e, 0x95, buf);
    }
    return 1;
}

static int find_sprite(struct NaviObject *obj, int kind, char *spr, char *act)
{
    static const char npcSpr[] = "npc\\%s.spr", npcAct[] = "npc\\%s.act";
    static const char mobSpr[] = "몬스터\\%s.spr", mobAct[] = "몬스터\\%s.act";
    const char *firstSpr, *firstAct, *thenSpr, *thenAct;

    if (kind == NAVI_RESULT_NPC) {
        firstSpr = npcSpr; firstAct = npcAct; thenSpr = mobSpr; thenAct = mobAct;
    } else if (kind == NAVI_RESULT_MOB) {
        firstSpr = mobSpr; firstAct = mobAct; thenSpr = npcSpr; thenAct = npcAct;
    } else {
        return 0;
    }
    sprintf(spr, firstSpr, session_get_job_name(&g_session1, navi_object_sprite(obj), -1));
    sprintf(act, firstAct, session_get_job_name(&g_session1, navi_object_sprite(obj), -1));
    if (res_mgr_is_available(g_resMgr(), spr) && res_mgr_is_available(g_resMgr(), act))
        return 1;
    sprintf(spr, thenSpr, session_get_job_name(&g_session1, navi_object_sprite(obj), -1));
    sprintf(act, thenAct, session_get_job_name(&g_session1, navi_object_sprite(obj), -1));
    return res_mgr_is_available(g_resMgr(), spr) && res_mgr_is_available(g_resMgr(), act);
}

unsigned char navi_manager_update_window(struct NaviManager *self)
{
    void *obj;
    int kind, mapType;
    const char *name;

    self->minimapX = -1;
    self->minimapY = -1;
    self->sprRes = NULL;
    self->actRes = NULL;
    self->picture = NULL;
    if (!self->guided.obj || !guided_info(self))
        return 0;

    kind = self->guided.kind;
    obj = self->guided.obj;
    self->pictureKind = NAVI_PICTURE_MAP;
    if (kind == NAVI_RESULT_PLACE || kind == NAVI_RESULT_MAP) {
        if (!obj)
            return 0;
        mapType = navi_map_map_type((struct NaviMap *)obj);
        name = navi_map_name((struct NaviMap *)obj);
    } else if (kind == NAVI_RESULT_ROUTE_LINK) {
        if (!obj)
            return 0;
        if (navi_object_type(obj) == NAVI_TYPE_WARP_SCROLL) {
            mapType = NAVI_MAP_TYPE_SCROLL;
            name = navi_object_name(obj);
        } else {
            mapType = navi_map_map_type(navi_object_map(obj));
            name = navi_map_name(navi_object_map(obj));
        }
    } else {
        self->sprRes = NULL;
        self->actRes = NULL;
        if (!obj)
            return 0;
        mapType = navi_map_map_type(navi_object_map(obj));
        name = navi_map_name(navi_object_map(obj));
        if (navi_object_sprite(obj) >= 0) {
            char spr[0x80], act[0x80];

            memset(spr, 0, 0x80);
            memset(act, 0, 0x80);
            if (find_sprite((struct NaviObject *)obj, self->guided.kind, spr, act)) {
                self->sprRes = object_as_type(res_mgr_get(g_resMgr(), spr), OBJECT_SPRITE_RESOURCE);
                self->actRes = object_as_type(res_mgr_get(g_resMgr(), act), OBJECT_ACTION_RESOURCE);
            }
        }
        self->pictureKind = NAVI_PICTURE_SPRITE;
    }

    self->picture = NULL;
    if (mapType == NAVI_MAP_TYPE_SCROLL) {
        self->picture = texture_cache_get(&g_textureCache,
            NAVI_UI_DIR "collection\\불의주문서.bmp", 0, 0);
    } else if (mapType == NAVI_MAP_TYPE_DEFAULT || mapType == 0x138d) {
        char path[0x3c];

        memset(path, 0, 0x3c);
        sprintf(path, NAVI_UI_DIR "map\\%s.bmp", name);
        self->picture = texture_cache_get(&g_textureCache, path, 0, 0);
        if (self->picture) {
            struct NaviPoint at, a, b, *pos = NULL;

            kind = self->guided.kind;
            if (kind == NAVI_RESULT_NPC || kind == NAVI_RESULT_ROUTE_LINK) {
                struct NaviObject *o = (struct NaviObject *)self->guided.obj;
                int y = navi_object_position(o, &a)->y;
                int x = navi_object_position(o, &b)->x;

                pos = navi_manager_minimap_pos(self, &at, navi_object_map(o), x, y, 126.0f);
            } else if (kind == NAVI_RESULT_PLACE) {
                if (self->guided.x < 0) {
                    int y = navi_manager_target_pos(self, &a)->y;
                    int x = navi_manager_target_pos(self, &b)->x;

                    pos = navi_manager_minimap_pos(self, &at, navi_manager_target_map(self), x, y, 126.0f);
                } else {
                    pos = navi_manager_minimap_pos(self, &at, (struct NaviMap *)self->guided.obj,
                                                 self->guided.x, self->guided.y, 126.0f);
                }
            }
            if (pos) {
                self->minimapX = pos->x;
                self->minimapY = pos->y;
            }
        }
    } else {
        self->picture = texture_cache_get(&g_textureCache, NAVI_UI_DIR "navigation_interface\\Interior.bmp", 0, 0);
    }
    if (!self->picture || texture_image_width(self->picture) == 1)
        self->picture = texture_cache_get(&g_textureCache, NAVI_UI_DIR "navigation_interface\\NoImage.bmp", 0, 0);
    return 1;
}
