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
#include "core/text_format.h"
#include "platform/code_page.h"

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

/* The number in an <action> or <loopType> node of monsterskillinfo.xml; a missing node
   counts as 0, as on the administrator path. atoi reads only blanks, a sign and digits,
   which text_decode leaves as they are, so the text is read as stored. */
static int node_number(struct XmlElement *node)
{
    return node ? atoi(node_text(node)) : 0;
}

void session_get_monster_skill_info(struct Session *self, const char *monsterName,
                                            int skillId, int *action, int *loopType)
{
    char number[8], skillKey[8];
    char key[0x100], name[0x100];
    struct XmlElement *node, *actionNode, *loopNode;

    *action = -1;
    strcpy(key, "Monster_Action_Table");
    node = xml_find_child(&self->monsterSkillTable.root, text_decode(key));
    if (!node)
        return;
    _itoa(skillId, number, 10);
    sprintf(skillKey, "_%s", number);
    _snprintf_s(name, sizeof name, _TRUNCATE, "%s", monsterName);
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
    *action = node_number(actionNode);
    *loopType = node_number(loopNode);
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

/* Copies length bytes of a talk line into out[size], cut by whole characters. */
static void talk_copy(char *out, size_t size, const char *text, size_t length)
{
    if (!size)
        return;
    length = text_prefix_length(text, length, size - 1, g_codePage);
    memcpy(out, text, length);
    out[length] = 0;
}

void session_get_monster_talk_info(struct Session *self, int job, int state, int value,
                                           char *out, size_t out_size, int aid)
{
    char number[12], key[0x100], name[0x100];
    struct XmlElement *node, *stateNode, *n;
    struct Vec lines;
    const char *stateName;
    size_t count;

    talk_copy(out, out_size, "", 0);
    strcpy(key, "enemy_monster_talk_table");
    node = xml_find_child(&self->monsterTalkTable.root, text_decode(key));
    if (!node)
        return;
    _snprintf_s(name, sizeof name, _TRUNCATE, "%s", ((const char **)g_session1.reqJobNames.first)[job]);
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
    if (count)
        session_format_chat_line(&g_session1, ((const char **)lines.first)[rand() % (int)count], out,
                                 out_size, aid, 0);
    vec_free(&lines);
}

void session_get_pet_speak(struct Session *self, int petJob, int hunger, int action,
                                    char *out, size_t out_size)
{
    char key[0x100];
    const char *hungerName, *actionName;
    struct XmlElement *node, *n;
    struct Vec lines;
    size_t count;

    talk_copy(out, out_size, "", 0);
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
        const char *line = ((const char **)lines.first)[rand() % (int)count];

        /* The Korean-service mirror maps lead and trail bytes to lead and trail bytes,
           so cutting before decoding keeps the same whole characters. */
        talk_copy(out, out_size, line, strlen(line));
        text_decode(out);
    }
    vec_free(&lines);
}

/* Builds "name : line" in a String and copies it into out[out_size], cut by whole
   characters; text is copied as text and not written. */
void session_format_chat_line(struct Session *self, const char *text, char *out, size_t out_size, int aid,
                                       int replaceWords)
{
    const char *source = text[0] == '\t' ? text + 1 : text;
    size_t source_size = strlen(source), from = 0;
    struct String line, said, actorName;
    const char *name;

    string_init(&line);
    string_assign(&line, source, source_size);
    while (replaceWords) {
        const char *at = NULL;
        int i;

        for (i = 0; i < g_talkWordCount; i++) {
            at = strstr(string_cstr(&line) + from, g_talkWords[i]);
            if (at)
                break;
        }
        if (i >= g_talkWordCount)
            break;
        {
            const char *replacement = g_talkWords[g_talkWordCount + i];
            size_t length = strlen(replacement), offset = (size_t)(at - string_cstr(&line)), tail;

            /* As before, the rest is taken from the source at the word's offset in the line
               plus the replacement's length, but never from past the source's end. */
            tail = offset + length < source_size ? offset + length : source_size;
            string_truncate(&line, offset);
            string_append(&line, replacement, length);
            string_append(&line, source + tail, source_size - tail);
            from = offset + length;
        }
    }

    game_mode_get_actor_name(game_current_mode(), &actorName, aid);
    if (actorName.size != 0)
        name = string_cstr(&actorName);
    else if (aid == g_session1.aid)
        name = g_session1.petName;
    else
        name = NULL;
    string_init(&said);
    if (name) {
        string_format(&said, "%s : %s", name, string_cstr(&line));
        if (string_cstr(&said)[said.size - 1] == '\t')
            string_truncate(&said, said.size - 1);
    }
    talk_copy(out, out_size, string_cstr(&said), said.size);
    string_free(&said);
    string_free(&line);
    string_free(&actorName);
}
