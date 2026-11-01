#ifndef UI_QUESTS_QUEST_JOURNAL_LAYOUT_H
#define UI_QUESTS_QUEST_JOURNAL_LAYOUT_H

#define QUEST_TAB_WIDTH 0x1d
#define QUEST_TAB_HEIGHT 0x65
#define QUEST_TAB_Y 0x11

enum { QUEST_TAB_ACTIVE, QUEST_TAB_PAUSED, QUEST_TAB_OTHER, QUEST_TAB_COUNT };
struct UIWindow;
void quest_journal_draw_background(struct UIWindow *wnd, int tab);

#endif
