#include "platform/win_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/drive_state.h"
#include "world/mode_manager.h"
#include "world/mode_types.h"
#include "world/state.h"
#include "game/gamemode.h"
#include "game/cells.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_types.h"
#include "ui/window_keyboard.h"
#include "world/pick_node.h"
#include "platform/clock.h"

#define DRIVE_STATE_INTERVAL_MS 250
#define CHAT_KEPT 8
#define CHAT_LINE_MAX 160

static char g_statePath[MAX_PATH];
static int g_stateChecked;
static DWORD g_stateLastWrite;
static char g_chat[CHAT_KEPT][CHAT_LINE_MAX];
static int g_chatNext;

static const char *state_path(void)
{
    if (!g_stateChecked) {
        DWORD n = GetEnvironmentVariableA("RAGEXE_DRIVE_STATE", g_statePath, sizeof g_statePath);

        if (n == 0 || n >= sizeof g_statePath)
            g_statePath[0] = '\0';
        g_stateChecked = 1;
    }
    return g_statePath;
}

void drive_state_note_chat(const char *text)
{
    char *line;
    size_t i;

    if (!text || !state_path()[0])
        return;
    line = g_chat[g_chatNext];
    strncpy(line, text, CHAT_LINE_MAX - 1);
    line[CHAT_LINE_MAX - 1] = '\0';
    for (i = 0; line[i]; i++)
        if (line[i] == '\r' || line[i] == '\n')
            line[i] = ' ';
    g_chatNext = (g_chatNext + 1) % CHAT_KEPT;
}

static void write_game(FILE *f)
{
    struct GameMode *mode = (struct GameMode *)g_modeMgr.curMode;
    struct GameActor *player;
    int x = 0, y = 0;

    fprintf(f, "map=%s\nchar=%s\n", g_session1.mapName, g_session1.charName);
    if (mode && mode->world && mode->world->player) {
        player = (struct GameActor *)mode->world->player;
        game_mode_world_to_cell(mode, player->pos.x, player->pos.z, &x, &y);
        fprintf(f, "cell=%d,%d\n", x, y);
    }
    fprintf(f, "hp=%d/%d\nsp=%d/%d\nlevel=%d/%d\n", g_session1.hp, g_session1.maxHp, g_session1.sp,
            g_session1.maxSp, g_session1.baseLevel, g_session1.jobLevel);
}

static void write_picks(FILE *f, const struct ActorPickNode *node, const char **sep)
{
    const struct ListNode *head = node->boxes.head, *n;
    int i;

    if (node->child[0]) {
        for (i = 0; i < 4; i++)
            write_picks(f, node->child[i], sep);
        return;
    }
    if (!head)
        return;
    for (n = head->next; n != head; n = n->next) {
        const struct PickInfo *box = *LIST_VALUE(n, struct PickInfo *);

        fprintf(f, "%s%d:%d:%d:%d,%d", *sep, box->aid, box->job, box->kind,
                (int)((box->left + box->right) / 2), (int)((box->top + box->bottom) / 2));
        *sep = ";";
    }
}

static void write_windows(FILE *f)
{
    struct ListNode *head = g_windowMgr.windows.head, *node;
    const char *sep = "";

    fputs("windows=", f);
    if (head)
        for (node = head->next; node != head; node = node->next) {
            struct UIWindow *window = *LIST_VALUE(node, struct UIWindow *);

            if (window && window->visible) {
                fprintf(f, "%s%d", sep, window->id);
                sep = ",";
            }
        }
    fprintf(f, "\nmodal=%d\n", g_windowMgr.modal_wnd ? g_windowMgr.modal_wnd->id : 0);
    fprintf(f, "chat_focus=%d\n", g_windowMgr.chat_bar_shown && ui_window_mgr_is_chat_edit_focused(&g_windowMgr));
}

void drive_state_tick(void)
{
    const char *path = state_path();
    char temp[MAX_PATH + 8];
    DWORD now;
    FILE *f;
    int i;

    if (!path[0])
        return;
    now = platform_coarse_ticks_ms();
    if (now - g_stateLastWrite < DRIVE_STATE_INTERVAL_MS)
        return;
    g_stateLastWrite = now;
    snprintf(temp, sizeof temp, "%s.tmp", path);
    f = fopen(temp, "w");
    if (!f)
        return;
    fprintf(f, "tick=%lu\n", (unsigned long)now);
    if (!g_modeMgr.curMode) {
        fputs("mode=none\n", f);
    } else if (g_modeMgr.curModeType == MODE_TYPE_GAME) {
        const char *sep = "";

        fputs("mode=game\n", f);
        write_game(f);
        fputs("actors=", f);
        write_picks(f, &g_actorPickNode, &sep);
        fputc('\n', f);
    } else {
        fprintf(f, "mode=login\nscreen=%d\n", g_modeMgr.curMode->sub_mode);
    }
    write_windows(f);
    for (i = 0; i < CHAT_KEPT; i++) {
        const char *line = g_chat[(g_chatNext + i) % CHAT_KEPT];

        if (line[0])
            fprintf(f, "chat%d=%s\n", i, line);
    }
    fclose(f);
    MoveFileExA(temp, path, MOVEFILE_REPLACE_EXISTING);
}
