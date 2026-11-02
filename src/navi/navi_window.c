#include <limits.h>
#include <stdio.h>
#include "navi/navi_manager.h"
#include "game/cells.h"
#include "render/texture_cache.h"
#include "std/string.h"
#include "contents/client_lua_calls.h"
#include "world/pathfinder_state.h"
#include "navi/navi_search.h"
#include "resources/attr.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "platform/win_types.h"
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"

#include <string.h>
#include "core/msg_str.h"

static struct String *keep_text(struct NaviManager *self, int i, struct String *out, const char *text)
{
    string_assign_cstr(&self->texts[i], text);
    string_copy(out, &self->texts[i]);
    return out;
}

static int step_valid(struct NaviManager *self)
{
    return self->routeLinks.first != self->routeLinks.last && self->routeStep >= 0;
}

struct String *navi_manager_step_text_dots(struct NaviManager *self, struct String *out)
{
    char buf[50];

    memset(buf, 0, 0x32);
    if (step_valid(self)) {
        struct NaviObject *link;
        struct NaviPoint a, b;
        int y;

        if (VEC_COUNT(&self->routeLinks, struct NaviLink *) <= (unsigned)self->routeStep)
            return keep_text(self, 3, out, buf);
        link = &((struct NaviLink **)self->routeLinks.first)[self->routeStep]->base;
        y = navi_object_position(link, &a)->y;
        sprintf(buf, "%s %d %d...", navi_map_name(navi_object_map(link)),
                    navi_object_position(link, &b)->x, y);
        return keep_text(self, 3, out, buf);
    }
    if (self->guideState == NAVI_GUIDE_ACTIVE) {
        struct NaviMap *map = navi_manager_target_map(self);
        struct NaviPoint a, b;

        navi_manager_target_pos(self, &a);
        navi_manager_target_pos(self, &b);
        sprintf(buf, "%s %d %d...", navi_map_name(map), b.x, a.y);
    }
    return keep_text(self, 3, out, buf);
}

struct String *navi_manager_target_chat_text(struct NaviManager *self, struct String *out)
{
    char buf[50];
    struct NaviMap *map;

    memset(buf, 0, 0x32);
    map = navi_manager_target_map(self);
    if (map) {
        struct NaviPoint a, b;

        if (navi_manager_target_pos(self, &a)->x > 0) {
            navi_manager_target_pos(self, &a);
            navi_manager_target_pos(self, &b);
            sprintf(buf, ">.%s %d %d", navi_map_name(map), b.x, a.y);
        } else {
            sprintf(buf, ">.%s", navi_map_name(map));
        }
    }
    return keep_text(self, 4, out, buf);
}

struct String *navi_manager_step_text(struct NaviManager *self, struct String *out)
{
    char buf[50];

    memset(buf, 0, 0x32);
    if (step_valid(self)) {
        struct NaviObject *link;
        struct NaviPoint a, b;
        int y;

        if (VEC_COUNT(&self->routeLinks, struct NaviLink *) <= (unsigned)self->routeStep) {
            string_init(out);
            string_assign_cstr(out, buf);
            return out;
        }
        link = &((struct NaviLink **)self->routeLinks.first)[self->routeStep]->base;
        y = navi_object_position(link, &a)->y;
        sprintf(buf, "%s %d %d", navi_map_name(navi_object_map(link)),
                    navi_object_position(link, &b)->x, y);
        return keep_text(self, 1, out, buf);
    }
    if (self->guideState == NAVI_GUIDE_ACTIVE) {
        struct NaviMap *map = navi_manager_target_map(self);
        struct NaviPoint a, b;

        navi_manager_target_pos(self, &a);
        navi_manager_target_pos(self, &b);
        sprintf(buf, "%s %d %d", navi_map_name(map), b.x, a.y);
    }
    return keep_text(self, 1, out, buf);
}

struct String *navi_manager_step_map_name(struct NaviManager *self, struct String *out)
{
    char buf[50];

    memset(buf, 0, 0x32);
    if (step_valid(self)) {
        struct NaviObject *link;

        if (VEC_COUNT(&self->routeLinks, struct NaviLink *) <= (unsigned)self->routeStep)
            return keep_text(self, 0, out, buf);
        link = &((struct NaviLink **)self->routeLinks.first)[self->routeStep]->base;
        return keep_text(self, 0, out, navi_map_display_name(navi_object_map(link)));
    }
    if (self->guideState == NAVI_GUIDE_ACTIVE)
        return keep_text(self, 0, out, navi_map_display_name(navi_manager_target_map(self)));
    return keep_text(self, 0, out, buf);
}

struct String *navi_manager_target_label(struct NaviManager *self, struct String *out)
{
    char buf[50];
    struct NaviMap *map;

    memset(buf, 0, 0x32);
    map = navi_manager_target_map(self);
    if (map) {
        struct NaviPoint a, b;

        if (navi_manager_target_pos(self, &a)->x > 0) {
            navi_manager_target_pos(self, &a);
            navi_manager_target_pos(self, &b);
            snprintf(buf, 0x32, "[%s] -> %s %d %d", msg_str(0xe01), navi_map_name(map),
                        b.x, a.y);
        } else {
            snprintf(buf, 0x32, "[%s] %s", msg_str(0xe01), navi_map_name(map));
        }
    }
    return keep_text(self, 2, out, buf);
}

void navi_manager_report_status(struct NaviManager *self, int status, int how)
{
    struct String text;
    int id;

    string_init(&text);
    switch (status) {
    case -100: id = 0x8f6; break;
    case -99:  id = 0x8f7; break;
    case -98:  id = 0x8f8; break;
    case -97:  id = 0x8f9; break;
    case -96:  id = 0x8fa; break;
    case -95:  id = 0x8fb; break;
    case -94:  id = 0x8fc; break;
    case -93:  id = 0x8fd; break;
    case -92:  id = 0x8fe; break;
    case 0:    id = 0x8ff; break;
    case 1:    id = 0x900; break;
    case 2:    id = 0x901; break;
    case 3:    id = 0x902; break;
    case 4:    id = 0x903; break;
    case 5:    id = 0x904; break;
    case 6:    id = 0x905; break;
    case 7:    id = 0x906; break;
    case 8:    id = 0x907; break;
    case 9:    id = 0x909; break;
    case 10:   id = 0x908; break;
    case 11:   id = 0x90a; break;
    default:   id = -1; break;
    }
    if (id != -1) {
        const char *msg = msg_str(id);

        string_assign_cstr(&text, msg);
    } else {
        string_assign_cstr(&text, "NoMsg.");
    }

    if (how == NAVI_REPORT_BOTH || how == NAVI_REPORT_CHAT) {
        char line[250];

        memset(line, 0, 0xfa);
        sprintf(line, msg_str(0x90b), string_cstr(&text));
        ui_navigation_chat(line);
        if (status == NAVI_STATUS_ROUTE && self->routeStep != -1
                && (unsigned)self->routeStep < VEC_COUNT(&self->routeLinks, struct NaviLink *)) {
            struct NaviLink **links = (struct NaviLink **)self->routeLinks.first;
            int type = navi_object_type(&(links[self->routeStep])->base);

            if (type == NAVI_TYPE_LINK_NPC_SCRIPT || type == NAVI_TYPE_LINK_KAFRA_DUNGEON || type == NAVI_TYPE_LINK_COOL_EVENT_DUNGEON || type == NAVI_TYPE_LINK_NPC_WARPER) {
                struct NaviMap *dest;

                sprintf(line, msg_str(0x90c), navi_object_name(&(links[self->routeStep])->base));
                ui_navigation_chat(line);
                dest = navi_link_dest_map(links[self->routeStep]);
                if (dest) {
                    char where[260];

                    memset(where, 0, 0x104);
                    sprintf(where, " %s[ %s ] ", navi_map_display_name(dest), navi_map_name(dest));
                    sprintf(line, msg_str(0x90d), where);
                }
            } else if (type == NAVI_TYPE_LINK_AIRPORT) {
                sprintf(line, msg_str(0x90c), navi_object_name(&(links[self->routeStep])->base));
                ui_navigation_chat(line);
                sprintf(line, msg_str(0x90f), 0x90f);
            } else {
                sprintf(line, msg_str(0x910), 0x910);
            }
            ui_navigation_chat(line);
        }
    }

    if (how == NAVI_REPORT_BOTH || how == NAVI_REPORT_BOX) {
        ui_navigation_message_box(string_cstr(&text));
        if (how == 4) {
            struct String box;
            struct NaviLink *first;
            const char *msg;

            first = g_navi.routeLinks.last != g_navi.routeLinks.first
                  ? *(struct NaviLink **)g_navi.routeLinks.first : NULL;
            msg = msg_str(0x911);
            string_init(&box);
            string_assign_cstr(&box, msg);
            msg = navi_object_name(&first->base);
            string_append_cstr(&box, msg);
            string_append_cstr(&box, " ");
            msg = navi_link_dest_map_name(first);
            string_append_cstr(&box, msg);
            ui_navigation_message_box(string_cstr(&box));
            string_free(&box);
        }
    }
    string_free(&text);
}

unsigned char navi_manager_describe_destination(struct NaviManager *self, char *out, int size)
{
    char buf[260];
    void *obj;
    int len;

    if (!out || !self->destination.obj)
        return 0;
    memset(out, 0, size);
    memset(buf, 0, 0x104);
    obj = self->destination.obj;
    if (self->destination.kind == NAVI_RESULT_NPC) {
        struct NaviMap *map = navi_object_map(obj);
        struct NaviPoint a, b;
        int y = navi_object_position(obj, &a)->y;

        sprintf(buf, "%s %d %d", navi_map_name(map), navi_object_position(obj, &b)->x, y);
    } else if (self->destination.kind == NAVI_RESULT_MOB) {
        sprintf(buf, "%s", navi_map_name(navi_object_map(obj)));
    } else if (self->destination.kind == NAVI_RESULT_MAP) {
        sprintf(buf, "%s", navi_map_name(obj));
    } else if (self->destination.kind == NAVI_RESULT_PLACE) {
        struct NaviPoint a, b;
        int y = navi_manager_target_pos(self, &a)->y;

        sprintf(buf, "%s %d %d", navi_map_name(obj),
                    navi_manager_target_pos(self, &b)->x, y);
    } else {
        return 0;
    }

    len = (int)strlen(buf);
    if (size > len) {
        memcpy(out, buf, len);
    } else {
        char cut[260];

        memset(cut, 0, 0x104);
        memcpy(cut, buf, size - 4);
        sprintf(out, "%s...", cut);
    }
    return 1;
}

static unsigned char navigate_on(struct NaviManager *self, const char *mapName, struct NaviMap *map,
        struct SearchResult *r, struct String *label, int kind, int flags, unsigned char silent,
        int x, int y, int id)
{
    const char *name = navi_map_name(map);
    char recorded_map[30];
    size_t recorded_size = strlen(mapName);
    void *obj = map;
    int status;

    memcpy(recorded_map, mapName, recorded_size < sizeof recorded_map ? recorded_size : sizeof recorded_map);
    string_assign_cstr(label, name);
    if (kind == NAVI_RESULT_NPC || kind == NAVI_RESULT_MOB) {
        struct NaviObject *found = kind == NAVI_RESULT_NPC ? navi_map_npc_at(map, x, y)
                                                           : navi_map_mob_by_sprite(map, id);

        if (!found) {
            navi_manager_report_status(self, NAVI_STATUS_NOT_FOUND, NAVI_REPORT_CHAT);
            return 0;
        }
        string_append_char(label, ' ');
        name = navi_object_name(found);
        string_append_cstr(label, name);
        obj = found;
    } else if (kind != NAVI_RESULT_MAP && kind != NAVI_RESULT_PLACE) {
        return 0;
    }

    r->obj = obj;
    search_result_assign(&self->guided, search_result_assign(&self->destination, r));
    string_assign(&self->searchText, string_cstr(label), label->size);
    self->windowHidden = silent;
    self->routeUseAirship = (flags >> 2) & 1;
    self->routeUseKafra = flags & 1;
    self->routeUseScrolls = (flags >> 1) & 1;
    status = navi_manager_guide_to(self, x, y);
    navi_manager_report_status(self, status, NAVI_REPORT_CHAT);
    if (status < 0)
        return 0;

    ui_navigation_refresh_map(NAVI_MAP_GUIDED | NAVI_MAP_RESULTS | NAVI_MAP_ROUTE);
    self->windowPage = 1;
    if (ui_navigation_is_open()) {
        self->windowHidden = 0;
        ui_navigation_relayout();
    } else if (!self->windowHidden) {
        ui_navigation_open();
    }

    return 1;
}

int navi_navigate(struct NaviManager *self, const char *mapName,
        int kind, int flags, unsigned char silent, int x, int y, int id)
{
    struct NaviMap *map = NULL;
    unsigned char ok = 0;

    if (self && (unsigned)kind <= 4 && mapName && *mapName)
        map = navi_map_named(self, mapName);
    if (map) {
        struct SearchResult r;
        struct String label;

        search_result_construct(&r, kind, NULL);
        string_init(&label);
        ok = navigate_on(self, mapName, map, &r, &label, kind, flags, silent, x, y, id);
        string_free(&label);
        search_result_destroy(&r);
    }
    return ok;
}

int navi_open_search(struct NaviManager *self, const char *text, size_t size,
        int filter, unsigned char silent)
{
    if (!self || (!text && size) || size > INT_MAX || (unsigned)filter > 4) return 0;
    string_assign(&self->searchText, text ? text : "", size);
    self->windowHidden = silent;
    self->searchFilter = filter;
    navi_manager_run_search(self);
    if (!self->windowHidden) ui_navigation_show_search();
    return 1;
}

int navi_show_map(struct NaviManager *self, const char *mapName, int x, int y)
{
    struct NaviMap *map;
    const char *canonical;
    if (!self || !mapName || !(map = navi_manager_map_by_name(self, mapName))) return 0;

    self->guided.obj = map;
    self->guided.kind = x == 1000 ? NAVI_RESULT_MAP : NAVI_RESULT_PLACE;
    self->guided.x = x == 1000 ? -1 : x;
    self->guided.y = x == 1000 ? -1 : y;
    canonical = navi_map_name(map);
    string_assign_cstr(&self->guided.name, canonical);
    if (x != 1000) {
        char coordinates[32];
        int size = snprintf(coordinates, sizeof coordinates, " %d,%d", x, y);
        string_append(&self->guided.name, coordinates, (size_t)size);
    }
    self->windowPage = 0;
    self->windowCollapsed = 0;
    self->windowHidden = 0;
    if (self->guideState != NAVI_GUIDE_ACTIVE) self->guideState = NAVI_GUIDE_IDLE;
    navi_manager_update_window(self);
    ui_navigation_show_map();
    ui_navigation_refresh_map(NAVI_MAP_GUIDED | NAVI_MAP_RESULTS);
    return 1;
}

unsigned char navi_manager_run_search(struct NaviManager *self)
{
    struct String query, text;

    self->windowPage = 0;
    string_copy(&query, &self->searchText);
    if (!navi_manager_search(self, query, 0, &self->results))
        return 0;
    navi_results_sort((struct SearchResult *)self->results.first, (struct SearchResult *)self->results.last);
    navi_manager_build_groups(self);
    string_copy(&text, &self->searchText);
    if (text.size && VEC_COUNT(&self->resultGroups, struct Vec) >= 2) {
        struct Vec front;
        struct Vec *groups;
        unsigned i, n;

        vec_copy(&front, (struct Vec *)self->resultGroups.first, sizeof(int));
        groups = (struct Vec *)self->resultGroups.first;
        n = (unsigned)VEC_COUNT(&self->resultGroups, struct Vec);
        for (i = 0; i < n; i++) {
            const struct SearchResult *head =
                &((struct SearchResult *)self->results.first)[*(int *)groups[i].first];

            if (head->name.size == text.size
                    && !memcmp(string_cstr(&head->name), string_cstr(&text), text.size)) {
                vec_assign(&groups[0], groups[i].first, groups[i].last, sizeof(int));
                vec_assign(&((struct Vec *)self->resultGroups.first)[i], front.first, front.last, sizeof(int));
                break;
            }
        }
        vec_free(&front);
    }
    string_free(&text);
    return 1;
}

void navi_manager_search_again(struct NaviManager *self)
{
    struct String query;

    string_copy(&query, &self->searchText);
    if (navi_manager_search(self, query, self->searchFilter, &self->results))
        ui_navigation_refresh_results();
}
