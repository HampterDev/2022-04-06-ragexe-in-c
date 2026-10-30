#include <stdlib.h>
#include "security/gameguard.h"
#include "core/owned_text.h"

struct GameGuard {
    unsigned connect_count;
    int error;
    struct OwnedText error_text;
    CRITICAL_SECTION cs;
};

struct GameGuard *g_gameGuard;

void gameguard_set_error(struct GameGuard *self, const char *text)
{
    if (!self) return;
    EnterCriticalSection(&self->cs);
    if (!self->error) {
        self->error = 1;
        owned_text_assign(&self->error_text, text_view_cstr(text ? text : ""));
    }
    LeaveCriticalSection(&self->cs);
}

int gameguard_startup(void)
{
    struct GameGuard *guard;
    if (g_gameGuard) return 0;
    guard = malloc(sizeof *guard);
    if (!guard) return 0;
    guard->connect_count = 0;
    guard->error = 0;
    owned_text_init(&guard->error_text);
    if (!InitializeCriticalSectionEx(&guard->cs, 0, 0)) {
        free(guard);
        return 0;
    }
    g_gameGuard = guard;
    return 1;
}

void gameguard_shutdown(void)
{
    struct GameGuard *guard = g_gameGuard;
    if (!guard) return;
    g_gameGuard = NULL;
    owned_text_clear(&guard->error_text);
    DeleteCriticalSection(&guard->cs);
    free(guard);
}

int gameguard_has_error(struct GameGuard *self)
{
    int error;
    if (!self) return 0;
    EnterCriticalSection(&self->cs);
    error = self->error;
    LeaveCriticalSection(&self->cs);
    return error;
}

void gameguard_check(struct GameGuard *self)
{
    gameguard_set_error(self, "");
}

void gameguard_on_connect(struct GameGuard *self)
{
    if (!self) return;
    EnterCriticalSection(&self->cs);
    ++self->connect_count;
    LeaveCriticalSection(&self->cs);
}

void gameguard_authenticate(struct GameGuard *self)
{
    if (!self) return;
    EnterCriticalSection(&self->cs);
    LeaveCriticalSection(&self->cs);
}

void gameguard_send_auth_answers(struct GameGuard *self)
{
    if (!self) return;
    EnterCriticalSection(&self->cs);
    LeaveCriticalSection(&self->cs);
}

void gameguard_set_window(struct GameGuard *self)
{
    if (!self) return;
    EnterCriticalSection(&self->cs);
    LeaveCriticalSection(&self->cs);
}

void gameguard_set_account(struct GameGuard *self)
{
    if (!self) return;
    EnterCriticalSection(&self->cs);
    LeaveCriticalSection(&self->cs);
}
