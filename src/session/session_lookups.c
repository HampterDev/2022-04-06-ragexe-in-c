#include "contents/client_lua_calls.h"
#include "ui/resource/resource_paths.h"
#include "session/state.h"
#include "session/csession.h"
#include "game/context.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_state.h"

#include <string.h>

#define JOB_BARD                0x13
#define JOB_DANCER              0x14

BYTE session_is_bard_or_dancer(struct Session *self, int job, int female)
{
    if (!female)
        return job == JOB_BARD || job == 0xfb4 || job == 0xfca || job == 0xfe4 || job == 0xfeb
            || job == 0x1008 || job == 0x10a7;
    return job == JOB_DANCER || job == 0xfb5 || job == 0xfcb || job == 0xfe5 || job == 0xfec
        || job == 0x1009 || job == 0x10a8;
}

float session_get_baby_job_scale(struct Session *self, int job)
{
    if (!((job >= 0xfb7 && job <= 0xfbd) || job == 0xfcd || job == 0x105f
          || (unsigned)(job - 0xfbe) <= 0xe || (unsigned)(job - 0x1000) <= 0x10
          || (unsigned)(job - 0x103e) <= 0x18 || (job >= 0x1062 && job <= 0x1064) || job == 0x1061
          || (unsigned)(job - 0x106d) <= 5 || job == 0x107c || (unsigned)(job - 0x107e) <= 0x10
          || job == 0x1091 || job == 0x1092 || job == 0x1094 || job == 0x1097 || job == 0x1098))
        return 1.0f;
    if (session_is_doram_job(&g_session1, job) == 1)
        return 0.8f;
    if ((job >= 0xfb7 && job <= 0xfbd) || job == 0xfcd || job == 0x105f)
        return 0.75f;
    return 0.82f;
}

int session_remap_job(struct Session *self, int job)
{
    switch (job) {
    case 0x1015: return 0xfd1;
    case 0x1057: return 0xfa1;
    case 0x1058: return 0xfa2;
    case 0x1059: return 0xfa3;
    case 0x105a:
    case 0x105b: return 0xfa5;
    case 0x105c: return 0xfa6;
    case 0x105d: return 0xfa7;
    case 0x1068: return 0xfa8;
    default:     return job;
    }
}

int session_count_hidden_hotkeys_below(struct Session *self, int list, int index)
{
    int n = 0, i;

    if (list != 1)
        return 0;
    for (i = 0x1e; i < index; i++) {
        if (i == 0x1e || i == 0x21 || i == 0x22 || i == 0x24 || i == 0x35 || i == 0x38 || i == 0x3a
            || i == 0x2b || i == 0x2d || i == 0x36) {
            index++;
            n++;
        }
    }
    return n;
}

void session_erase_abr(struct Session *self, long long key)
{
    tree_erase_key(&self->abrHp, &key, tree_less_int64, NULL);
    tree_erase_key(&self->abrMaxHp, &key, tree_less_int64, NULL);
}

long long session_find_abr_hp(struct Session *self, long long key)
{
    struct Int64Pair *entry = tree_find(&self->abrHp, &key, tree_less_int64);

    return entry ? entry->value : -1;
}

long long session_find_abr(struct Session *self, long long key)
{
    struct Int64Pair *entry = tree_find(&self->abrMaxHp, &key, tree_less_int64);

    return entry ? entry->value : -1;
}

int session_find_combo_delay(struct Session *self, int key)
{
    const int *pairs = self->comboDelays.first;
    int count = (int)(((BYTE *)self->comboDelays.last - (BYTE *)pairs) >> 3), i;

    for (i = 0; i < count; i++)
        if (pairs[i * 2] == key)
            return ((const int *)self->comboDelays.first)[i * 2 + 1];
    return -1;
}

void *session_get_actor_by_aid(struct Session *self, DWORD aid)
{
    return game_find_actor((int)aid);
}

int session_get_default_option_int(struct Session *self, const char *name, int def)
{
    int value = -1;

    client_lua_call(self->luaState, "l_GetDefaultOptionValue", "s>d", name, &value);
    return value == -1 ? def : value;
}

struct String *tokens_at(struct Vec *tokens, int i)
{
    struct String *first = tokens->first;
    unsigned count = (unsigned)(((BYTE *)tokens->last - (BYTE *)first) / sizeof(struct String)), k;

    if (i < 0 || (unsigned)i >= count) {
        error_msg("Error in CTokenizer :: operator []", 0);
        first = tokens->first;
        for (k = 0; k < (unsigned)(((BYTE *)tokens->last - (BYTE *)tokens->first) / sizeof(struct String)); k++) {
            error_msg(string_cstr(&first[k]), 0);
            first = tokens->first;
        }
    }
    return &first[i];
}

BYTE session_is_emotion_text(struct Session *self, const char *text)
{
    struct Vec tokens = { 0 };
    struct String line;
    int emotion;
    BYTE is;

    if (!text)
        return 0;
    string_init(&line);
    string_assign_cstr(&line, text);
    tokens_split(&tokens, &line);
    string_free(&line);
    if ((((BYTE *)tokens.last - (BYTE *)tokens.first) / (int)sizeof(struct String)) == 0) {
        is = 0;
    } else {
        const char *word;
        text_to_lower(tokens_at(&tokens, 0));
        emotion = -1;
        word = string_cstr(tokens_at(&tokens, 0));
        is = client_lua_call(g_session1.luaState, "ReqEmotionByMsgStr", "s>d", word, &emotion) && emotion >= 0;
    }
    string_vec_free(&tokens);
    return is;
}

struct String *session_get_monster_act_name(struct Session *self,
                                                        struct String *out, int job)
{
    string_init(out);
    string_format(out, "몬스터\\%s.act", ((const char **)self->reqJobNames.first)[job]);
    if (!res_mgr_is_available(g_resMgr(), string_cstr(out)))
        string_clear(out);
    return out;
}
