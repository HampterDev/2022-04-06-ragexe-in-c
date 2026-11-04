#include "core/memory.h"
#include "core/fatal.h"
#include "platform/win_types.h"
#include <string.h>
#include "std/std.h"
#include "core/app.h"
#include "core/client_managers.h"
#include "core/text_util.h"

HINSTANCE g_hInstance;
char g_clientDir[0x88];

/* Destroys the client's managers (g_clientManagers, core/client_managers.c) in destroy-rank
   order, which is not the reverse of the create order. */
void app_destroy_singletons(void)
{
    manager_table_destroy(&g_clientManagers);
}
