#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/xml.h"
#include "session/session_state.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_messages.h"

/* Run the production session_get_monster_skill_info over a parsed
   monsterskillinfo.xml. Values longer than 8 bytes, values with printf
   conversions, missing <action> or <loopType> nodes and monster names longer
   than its 0x100-byte buffer must be read without overrunning anything. */

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static int admin, boxes;

__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }

int session_is_admin_option(struct Session *self, int what)
{
    assert(self == &g_session1 && what == 1);
    return admin;
}

int ui_window_mgr_message_box(struct UIWindowMgr *self, const char *text, int type, int arg3, int arg4, int arg5,
                              const char *title, int width, int height, int arg9)
{
    assert(self == &g_windowMgr && text && title);
    (void)type; (void)arg3; (void)arg4; (void)arg5; (void)width; (void)height; (void)arg9;
    ++boxes;
    return 0;
}

#include "monster_skill_info.inc"

static void element(char *xml, const char *name, const char *value)
{
    sprintf(xml + strlen(xml), "<%s>%s</%s>", name, value, name);
}

static void skill(char *xml, const char *key, const char *action, const char *loop)
{
    sprintf(xml + strlen(xml), "<%s>", key);
    if (action)
        element(xml, "action", action);
    if (loop)
        element(xml, "loopType", loop);
    sprintf(xml + strlen(xml), "</%s>", key);
}

static void load(void)
{
    static char xml[2048];

    strcpy(xml, "<Monster_Action_Table><poring>");
    skill(xml, "_10", "3", "1");
    skill(xml, "_11", "000000000007", "  -2 and some text after it");
    skill(xml, "_12", "5%d%s%n", "%n9");
    skill(xml, "_13", "4", NULL);
    skill(xml, "_14", NULL, "1");
    skill(xml, "_15", NULL, NULL);
    skill(xml, "_16", "", "");
    strcat(xml, "</poring></Monster_Action_Table>");
    xml_element_construct(&g_session1.monsterSkillTable.root, "XMLDocument");
    g_session1.monsterSkillTable.copy = NULL;
    assert(xml_parse(&g_session1.monsterSkillTable, xml, xml + strlen(xml)));
}

static void unload(void)
{
    xml_document_release(&g_session1.monsterSkillTable);
    xml_element_destruct(&g_session1.monsterSkillTable.root);
}

static int cases;

static void check(const char *monster, int skill_id, int action, int loop, int message_boxes)
{
    int got_action = 0x55, got_loop = 0x66;

    boxes = 0;
    session_get_monster_skill_info(&g_session1, monster, skill_id, &got_action, &got_loop);
    assert(got_action == action && got_loop == loop && boxes == message_boxes);
    ++cases;
}

int main(void)
{
    char long_name[0x300];

    memset(long_name, 'p', sizeof long_name - 1);
    long_name[sizeof long_name - 1] = 0;
    load();
    admin = 0;
    check("poring", 10, 3, 1, 0);
    check("PORING", 10, 3, 1, 0);
    check("poring", 11, 7, -2, 0);
    check("poring", 12, 5, 0, 0);
    check("poring", 13, 4, 0, 0);
    check("poring", 14, 0, 1, 0);
    check("poring", 15, 0, 0, 0);
    check("poring", 16, 0, 0, 0);
    check("poring", 99, -1, 0x66, 0);
    check("lunatic", 10, -1, 0x66, 0);
    check(long_name, 10, -1, 0x66, 0);
    check("poring", 10, 3, 1, 0);
    admin = 1;
    check("poring", 10, 3, 1, 0);
    check("poring", 13, 0, 0, 2);
    check("poring", 14, 0, 0, 2);
    check("poring", 11, 7, -2, 0);
    unload();
    printf("Monster skill info: %d lookups; long, formatted and missing values and long names.\n", cases);
    return 0;
}
