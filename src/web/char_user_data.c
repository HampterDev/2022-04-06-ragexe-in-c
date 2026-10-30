#include "skills/skill_bundle.h"
#include "web/service_manager_state.h"
#include "web/char_user_data.h"
#include "skills/skill_tree.h"
#include "skills/skill_info.h"
#include "session/state.h"
#include "session/skill_levels.h"
#include "session/option_pairs.h"
#include "ui/window_manager_state.h"
#include <stdlib.h>
#include <string.h>
#include "json/service_json.h"
#include "contents/game_session.h"
#include "ui/wndmgr/wndmgr.h"

#define CHAR_DATA_TYPE_DATA 1
#define CHAR_DATA_TYPE_NONE 4
#define SKILL_KIND_PLAYER 7
#define SKILL_KIND_HOMUN  8

static int get_int(const struct JsonTree *object, const char *key)
{
    struct JsonTree fallback, field;
    int value;

    json_value_from_int(&fallback, 0);
    json_value_get(object, &field, key, &fallback);
    value = json_value_as_int(&field);
    json_tree_free(&field);
    json_tree_free(&fallback);
    return value;
}

static void read_levels(struct JsonTree *data, const char *key, const char *id_key, const char *level_key,
                        void (*set_level)(int id, int level))
{
    struct JsonTree list;
    size_t it;
    int id, level;

    json_value_copy(&list, json_value_member(data, key));
    if (!json_value_is_null(&list)) {
        it = 0;
        while (it < json_value_size(&list)) {
            if (json_value_is_object(json_value_child(&list, it))) {
                id = get_int(json_value_child(&list, it), id_key);
                level = get_int(json_value_child(&list, it), level_key);
                set_level(id, level);
            }

            ++it;
        }
    }
    json_tree_free(&list);
}

static void set_homun_level(int id, int level)
{
    session_set_homun_skill_use_level(&g_session1, id, level);
}

static void set_skill_level(int id, int level)
{
    player_skill_bundle_set_chosen_level(&g_session1.skills, id, level);
}

static void build_urls(struct CharUserDataMgr *self)
{
    session_service_urls(&g_session1, &self->save_url, "/charconfig/save",
        &self->load_url, "/charconfig/load");
}

char char_data_load_work_run(struct CharDataLoadWork *self)
{
    char aid[0x40], gid[0x40];
    char *world, *body;
    struct HttpPost *post;

    memset(aid, 0, sizeof aid);
    _snprintf_s(aid, sizeof aid, (size_t)-1, "%lu", self->aid);
    memset(gid, 0, sizeof gid);
    _snprintf_s(gid, sizeof gid, (size_t)-1, "%lu", self->gid);
    world = http_text_to_utf8(owned_text_cstr(&self->world));
    post = http_post_create(owned_text_cstr(&self->url));

    http_post_add_text(post, "AID", aid);
    http_post_add_text(post, "GID", gid);
    http_post_add_text(post, "AuthToken", owned_text_cstr(&self->token));
    http_post_add_text(post, "WorldName", world);

    if (http_post_perform(post) == 0 && (body = http_text_from_utf8(http_post_body(post, NULL))) != NULL) {
        self->succeeded = owned_text_assign(&self->response, text_view_cstr(body));
        free(body);
    }
    http_post_free(post);
    free(world);
    return 0;
}

void char_data_load_work_done(struct CharDataLoadWork *self)
{
    char_user_data_on_loaded(g_charUserDataMgr, self->succeeded,
        self->aid, self->gid, owned_text_cstr(&self->world), owned_text_cstr(&self->response));
}

char char_data_save_work_run(struct CharDataSaveWork *self)
{
    char aid[0x40], gid[0x40];
    char *world, *data;
    struct HttpPost *post;

    memset(aid, 0, sizeof aid);
    _snprintf_s(aid, sizeof aid, (size_t)-1, "%lu", self->aid);
    memset(gid, 0, sizeof gid);
    _snprintf_s(gid, sizeof gid, (size_t)-1, "%lu", self->gid);
    world = http_text_to_utf8(owned_text_cstr(&self->world));
    data = http_text_to_utf8(owned_text_cstr(&self->data));
    post = http_post_create(owned_text_cstr(&self->url));

    http_post_add_text(post, "AID", aid);
    http_post_add_text(post, "GID", gid);
    http_post_add_text(post, "AuthToken", owned_text_cstr(&self->token));
    http_post_add_text(post, "WorldName", world);
    http_post_add_text(post, "data", data);

    if (http_post_perform(post) != 0)
        self->succeeded = 0;
    else
        self->succeeded = 1;
    http_post_free(post);
    free(data);
    free(world);
    return 0;
}

void char_data_save_work_done(void)
{
    char_user_data_save_answered(g_charUserDataMgr);
}

void char_user_data_on_loaded(struct CharUserDataMgr *self, int ok, int aid, int gid,
                                         const char *world, const char *response)
{
    struct JsonTree root;

    char parsed;
    int type;

    if (ok) {
        json_tree_init(&root);

        parsed = json_parse_text(&root, response, strlen(response)) && json_value_is_object(&root);
        if (parsed && owned_text_assign(&self->world, text_view_cstr(world))) {
            type = json_value_as_int(json_value_member(&root, "Type"));
            if (type == CHAR_DATA_TYPE_DATA) {
                char_user_data_read_skills(json_value_member(&root, "data"));
                char_user_data_read_homun_skills(json_value_member(&root, "data"));
                self->loaded = 1;
            } else if (type == CHAR_DATA_TYPE_NONE) {
                ui_window_mgr_init_skill_use_level_from_reg(&g_windowMgr);
                session_set_pair(&g_session1, SKILL_KIND_PLAYER, 1);
                session_set_pair(&g_session1, SKILL_KIND_HOMUN, 1);
                self->loaded = 1;
            }
            self->aid = aid;
            self->gid = gid;
        }

        json_tree_free(&root);
    }
}

void char_user_data_read_homun_skills(struct JsonTree *data)
{
    read_levels(data, "HomunSkillInfo", "HomunSKID", "HomunSKLv", set_homun_level);
}

void char_user_data_read_skills(struct JsonTree *data)
{
    read_levels(data, "UseSkillInfo", "SKID", "SKLv", set_skill_level);
}

void char_user_data_load(struct CharUserDataMgr *self, int aid, int gid, const char *world)
{
    if (aid != 0 && gid != 0)
        async_work_mgr_char_data_load(g_asyncWorkMgr, aid, gid, text_view_cstr(world),
            owned_text_view(&self->load_url), text_view(g_session1.webAuthToken, sizeof g_session1.webAuthToken));
}

void char_user_data_reset(struct CharUserDataMgr *self)
{
    self->aid = 0;
    self->gid = 0;
    owned_text_clear(&self->world);
    owned_text_assign(&self->world, text_view_cstr("unknown"));
    self->loaded = 0;
    build_urls(self);
}
