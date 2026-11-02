#include "ui/window_manager_state.h"
#include "session/csession.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "ui/ui_std.h"
#include "core/cstr.h"

static char *node_text(struct XmlElement *node)
{
    struct String *text = xml_node_text(node);

    return text->size ? text->data : "";
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    snprintf(number, sizeof number, "%d", skillId);
    sprintf(skillKey, "_%s", number);
    strcpy(name, monsterName);
    cstr_lower(name);
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
