#include "ui/social/guild_requests.h"
#include "world/mode_manager.h"
#include "world/mode_dispatch.h"

void guild_request_page(enum GuildPage page)
{
    if ((unsigned)page >= GUILD_PAGE_COUNT)
        return;
    if (string_cstr(&g_guildInfo.master_name)[0] == '\0')
        mode_send_message(client_current_mode(), 0x6b, GUILD_PAGE_INFO, 0, 0, 0);
    g_guildInfo.page_asked[page] = 1;
    mode_send_message(client_current_mode(), 0x6b, page, 0, 0, 0);
}
