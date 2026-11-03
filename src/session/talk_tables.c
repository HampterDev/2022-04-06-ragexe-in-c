#include "ui/window_manager_state.h"
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

void session_get_monster_skill_info(struct Session *self, const char *monsterName,
                                            int skillId, int *action, int *loopType)
{
    char number[8], actionText[8], loopText[8], skillKey[8];
    char name[0x100];
    struct XmlElement *node, *actionNode, *loopNode;

    *action = -1;
    node = xml_find_child(&self->monsterSkillTable.root, "Monster_Action_Table");
    if (!node)
        return;
    _itoa(skillId, number, 10);
    sprintf(skillKey, "_%s", number);
    strcpy(name, monsterName);
    _strlwr(name);
    node = xml_find_child(node, name);
    if (!node)
        return;
    node = xml_find_child(node, skillKey);
    if (!node)
        return;
    actionNode = xml_find_child(node, "action");
    loopNode = xml_find_child(node, "loopType");

    if (session_is_admin_option(&g_session1, 1) && (!actionNode || !loopNode)) {
        *action = 0;
        *loopType = 0;
        ui_window_mgr_message_box(&g_windowMgr,
                   "\xec\x97\x90\xeb\x9f\xac\xeb\xb0\x9c\xec\x83\x9d. \xed\x81\xb4\xeb\x9d\xbc\xec\x9d\xb4\xec\x96\xb8\xed\x8a\xb8 \xeb\x8b\xb4\xeb\x8b\xb9\xec\x9e\x90 " /* "에러발생. 클라이언트 담당자 " */
                   "\xec\x97\x90\xea\xb2\x8c \xec\x97\x90\xeb\x9f\xac \xec\x82\xac\xec\x8b\xa4\xec\x9d\x84 \xed\x86\xb5\xeb\xb3\xb4 \xed\x95\x98\xec\x84\xb8\xec\x9a\x94. " /* "에게 에러 사실을 통보 하세요. " */
                   "\xec\x9c\x84\xec\xb9\x98 - CPlayer :: GetMonsterSkillInfo" /* "위치 - CPlayer :: GetMonsterSkillInfo" */,
                   0, 1, 0, 0, UI_MSGBOX_TITLE, 0x118, 0x78, 0);
        ui_window_mgr_message_box(&g_windowMgr,
                   "action \xeb\x98\x90\xeb\x8a\x94 loopType \xec\x9d\xb4 \xec\xa1\xb4\xec\x9e\xac\xed\x95\x98\xec\xa7\x80 \xec\x95\x8a\xec\x9d\x8c." /* "action 또는 loopType 이 존재하지 않음." */,
                   0, 1, 0, 0, UI_MSGBOX_TITLE, 0x118, 0x78, 0);
        return;
    }
    sprintf(actionText, node_text(actionNode));
    sprintf(loopText, node_text(loopNode));
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
