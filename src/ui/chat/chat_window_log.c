#include "session/state.h"
#include "ui/window_manager_state.h"
#include <io.h>
#include <direct.h>
#include <stdio.h>
#include <string.h>
#include "ui/chat/chat_window.h"
#include "ui/chat/sub_chat.h"
#include "core/app.h"

#define CHAT_SAVED_COLOUR 0xff00
#define CHAT_PATH_SIZE 0x800

static void save_box(struct UIChatHisBox *box, const struct String *name)
{
    char path[CHAT_PATH_SIZE];
    int number = 0;

    sprintf(path, "%s\\Chat\\Chat_%s.txt", g_clientDir, string_cstr(name));
    if (_access(path, 0) == 0) {
        do {
            number++;
            sprintf(path, "%s\\Chat\\Chat_%s_%03d.txt", g_clientDir, string_cstr(name), number);
        } while (_access(path, 0) == 0);
    }
    if (ui_chat_his_box_save_to_file(box, path)) {
        struct String line;

        string_init(&line);
        string_assign_cstr(&line, path);
        string_append_cstr(&line, " is Saved.");
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        ui_chat_line(string_cstr(&line), CHAT_SAVED_COLOUR);
        string_free(&line);
    }
}

void ui_chat_wnd_write_chat_to_file(struct UINewChatWnd *self)
{
    char path[CHAT_PATH_SIZE];
    struct String name;
    struct TreeNode *node;
    int i;

    sprintf(path, "%s\\Chat", g_clientDir);
    if (_access(path, 0) != 0 && _mkdir(path) != 0)
        return;
    string_init(&name);
    for (i = 0; i < CHAT_BOX_COUNT(self); i++) {
        struct ChatTab *tab;

        tab = chat_tabs_find(&g_session1.chatWndTabs, i);
        if (tab)
            string_assign(&name, owned_text_cstr(&tab->value.name), tab->value.name.size);
        if (name.size)
            save_box((struct UIChatHisBox *)CHAT_BOX(self, i), &name);
    }
    for (node = tree_first(&g_windowMgr.sub_chat_wnds); node != g_windowMgr.sub_chat_wnds.head;
         node = tree_next(node)) {
        struct UISubChatHisWnd *sub = (struct UISubChatHisWnd *)TREE_VALUE(node, struct SubChatWnd)->wnd;
        struct String title;

        string_copy(&title, &sub->name);
        string_free(&name);
        name = title;
        save_box(sub->box, &name);
    }
    string_free(&name);
}
