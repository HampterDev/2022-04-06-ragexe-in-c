#include "ui/window_manager_state.h"
#include "game/context.h"
#include "session/csession.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "ui/ui_std.h"

static char *node_text(struct XmlElement *node)
{
    struct String *text = xml_node_text(node);

    return text->size ? text->data : "";
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

BYTE g_talkWordCount = 13;
const char *g_talkWords[] = {
    "아빠",
    "형님",
    "오빠",
    "도련님",
    "형씨",
    "도련님은",
    "도련님을",
    "도련님과",
    "아빠",
    "아빠",
    "아빠",
    "아빠",
    "아빠",
    "엄마",
    "누님",
    "언니",
    "아가씨",
    "아가씨",
    "아가씨는",
    "아가씨를",
    "아가씨와",
    "엄마",
    "엄마",
    "엄마",
    "엄마",
    "엄마",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

void session_get_monster_skill_info(struct Session *self, const char *monsterName,
                                            int skillId, int *action, int *loopType)
{
    char number[8], actionText[8], loopText[8], skillKey[8];
    char key[0x100], name[0x100];
    struct XmlElement *node, *actionNode, *loopNode;

    *action = -1;
    strcpy(key, "Monster_Action_Table");
    node = xml_find_child(&self->monsterSkillTable.root, text_decode(key));
    if (!node)
        return;
    _itoa(skillId, number, 10);
    sprintf(skillKey, "_%s", number);
    strcpy(name, monsterName);
    _strlwr(name);
    node = xml_find_child(node, text_decode(name));
    if (!node)
        return;
    node = xml_find_child(node, text_decode(skillKey));
    if (!node)
        return;
    strcpy(key, "action");
    actionNode = xml_find_child(node, text_decode(key));
    strcpy(key, "loopType");
    loopNode = xml_find_child(node, text_decode(key));

    if (session_is_admin_option(&g_session1, 1) && (!actionNode || !loopNode)) {
        *action = 0;
        *loopType = 0;
        ui_window_mgr_message_box(&g_windowMgr,
                   "에러발생. 클라이언트 담당자 "
                   "에게 에러 사실을 통보 하세요. "
                   "위치 - CPlayer :: GetMonsterSkillInfo",
                   0, 1, 0, 0, "메시지", 0x118, 0x78, 0);
        ui_window_mgr_message_box(&g_windowMgr,
                   "action 또는 loopType 이 존재하지 않음.",
                   0, 1, 0, 0, "메시지", 0x118, 0x78, 0);
        return;
    }
    sprintf(actionText, text_decode(node_text(actionNode)));
    sprintf(loopText, text_decode(node_text(loopNode)));
    *action = atoi(actionText);
    *loopType = atoi(loopText);
}

struct String *session_get_monster_sprite_name(struct Session *self,
                                                         struct String *out, int job)
{
    string_init(out);
    string_format(out, "몬스터\\%s.spr", ((const char **)self->reqJobNames.first)[job]);
    if (!res_mgr_is_available(g_resMgr(), string_cstr(out)))
        string_clear(out);
    return out;
}

const char *session_get_state_name(struct Session *self, int state)
{
    switch (state) {
    case 0: return "attack";
    case 1: return "castskill";
    case 2: return "useskill";
    case 3: return "skillattacked";
    case 4: return "hp50";
    case 5: return "hp25";
    case 6: return "dead";
    default: return NULL;
    }
}

void session_get_monster_talk_info(struct Session *self, int job, int state, int value,
                                           char *out, int aid)
{
    char number[12], line[0x300], key[0x100], name[0x100];
    struct XmlElement *node, *stateNode, *n;
    struct Vec lines;
    const char *stateName;
    size_t count;

    sprintf(out, "");
    strcpy(key, "enemy_monster_talk_table");
    node = xml_find_child(&self->monsterTalkTable.root, text_decode(key));
    if (!node)
        return;
    strcpy(name, ((const char **)g_session1.reqJobNames.first)[job]);
    _strlwr(name);
    node = xml_find_child(node, text_decode(name));
    if (!node)
        return;
    stateName = session_get_state_name(&g_session1, state);
    stateNode = xml_find_child(node, text_decode((char *)stateName));
    if (!stateNode)
        return;

    lines.first = lines.last = lines.end = NULL;
    if ((unsigned)(state - 1) <= 2) {
        _itoa(value, number, 10);
        for (n = xml_find_child(stateNode, text_decode(number)); n;
             n = xml_find_next(n, text_decode(number))) {
            const char *text = text_decode(node_text(n));

            vec_push(&lines, &text, sizeof text);
        }
    } else {
        for (n = stateNode; n; n = xml_find_next(n, text_decode((char *)stateName))) {
            const char *text = text_decode(node_text(n));

            vec_push(&lines, &text, sizeof text);
        }
    }
    count = VEC_COUNT(&lines, const char *);
    if (count) {
        sprintf(line, ((const char **)lines.first)[rand() % (int)count]);
        session_format_chat_line(&g_session1, line, out, aid, 0);
    }
    vec_free(&lines);
}

void session_get_pet_speak(struct Session *self, int petJob, int hunger, int action,
                                    char *out)
{
    char key[0x100];
    const char *hungerName, *actionName;
    struct XmlElement *node, *n;
    struct Vec lines;
    size_t count;

    out[0] = 0;
    strcpy(key, "monster_talk_table");
    node = xml_find_child(&self->petTalkTable.root, text_decode(key));
    if (!node)
        return;
    strcpy(key, session_get_pet_name(petJob));
    node = xml_find_child(node, text_decode(key));
    if (!node)
        return;
    switch (hunger) {
    case 1:  hungerName = "bit_hungry"; break;
    case 2:  hungerName = "noting"; break;
    case 3:  hungerName = "full"; break;
    case 4:  hungerName = "so_full"; break;
    default: hungerName = "hungry"; break;
    }
    strcpy(key, hungerName);
    node = xml_find_child(node, text_decode(key));
    if (!node)
        return;
    actionName = session_get_pet_act_text(action);
    strcpy(key, actionName);
    n = xml_find_child(node, text_decode(key));
    if (!n)
        return;

    lines.first = lines.last = lines.end = NULL;
    do {
        const char *text = string_cstr(xml_node_text(n));

        vec_push(&lines, &text, sizeof text);
        strcpy(key, actionName);
        n = xml_find_next(n, text_decode(key));
    } while (n);
    count = VEC_COUNT(&lines, const char *);
    if (count) {
        strcpy(out, ((const char **)lines.first)[rand() % (int)count]);
        text_decode(out);
    }
    vec_free(&lines);
}

void session_format_chat_line(struct Session *self, char *text, char *out, int aid,
                                       int replaceWords)
{
    char *source = text[0] == '\t' ? text + 1 : text;
    char *at = out;
    int offset = 0, replacedLength = 0;
    struct String actorName;
    const char *name;
    size_t n;

    sprintf(out, source);
    while (replaceWords) {
        int i;

        for (i = 0; i < g_talkWordCount; i++) {
            at = strstr(at, g_talkWords[i]);
            if (at)
                break;
            at = out + offset + replacedLength;
        }
        if (i >= g_talkWordCount)
            break;
        {
            const char *replacement = g_talkWords[g_talkWordCount + i];
            int length = (int)strlen(replacement);

            strcpy(at, replacement);
            offset = (int)(at - out);
            strcat(at, source + offset + length);
            at += length;
            replacedLength = length;
        }
    }

    game_mode_get_actor_name(game_current_mode(), &actorName, aid);
    if (actorName.size != 0)
        name = string_cstr(&actorName);
    else if (aid == g_session1.aid)
        name = g_session1.petName;
    else {
        out[0] = 0;
        string_free(&actorName);
        return;
    }
    sprintf(source, "%s : ", name);
    strcat(source, out);
    strcpy(out, source);
    n = strlen(out);
    if (out[n - 1] == '\t')
        out[n - 1] = 0;
    string_free(&actorName);
}
