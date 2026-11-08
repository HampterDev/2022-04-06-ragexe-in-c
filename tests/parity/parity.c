
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <d3d9.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <intrin.h>

#include "parity_proto.h"

#define FRAME_MS 16

#define EPOCH_FILETIME 0x01DC7B1627B5E000ULL
#define CLOCK_START_MS 3600000u

static HANDLE g_pipe = INVALID_HANDLE_VALUE;
static FILE *g_log;
static int g_trace;     
static CRITICAL_SECTION g_lock;
static DWORD g_main_tid;



static void plog(const char *fmt, ...)
{
    va_list ap;
    if (!g_log)
        return;
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fputc('\n', g_log);
    fflush(g_log);
}


static void coverage_dump(void)
{
    void (__cdecl *dump)(void) =
        (void (__cdecl *)(void))(void *)GetProcAddress(GetModuleHandleA(NULL), "ragexe_coverage_dump");
    if (dump)
        dump();
}

static void fatal(const char *what)
{
    plog("fatal: %s (error %lu)", what, GetLastError());
    coverage_dump();
    TerminateProcess(GetCurrentProcess(), 3);
}



static void pipe_write(const void *p, DWORD n)
{
    DWORD done;
    while (n) {
        if (!WriteFile(g_pipe, p, n, &done, NULL))
            fatal("pipe write");
        p = (const char *)p + done;
        n -= done;
    }
}

static void pipe_read(void *p, DWORD n)
{
    DWORD done;
    while (n) {
        if (!ReadFile(g_pipe, p, n, &done, NULL) || !done)
            fatal("pipe read");
        p = (char *)p + done;
        n -= done;
    }
}

static void send_msg(DWORD type, const void *a, DWORD an, const void *b, DWORD bn)
{
    DWORD hdr[2] = { type, an + bn };
    pipe_write(hdr, sizeof hdr);
    if (an)
        pipe_write(a, an);
    if (bn)
        pipe_write(b, bn);
}



static volatile DWORD g_frame_base = CLOCK_START_MS;
static volatile DWORD g_intra;
static DWORD g_frame;
static DWORD g_sync_index;


#define SPIN_READS 20000
static DWORD g_spin_reads;
static int g_spin_noted;

static DWORD clock_ms(void)
{
    if (GetCurrentThreadId() == g_main_tid && ++g_spin_reads >= SPIN_READS) {
        g_spin_reads = 0;
        g_intra++;
        if (!g_spin_noted) {
            g_spin_noted = 1;
            plog("the main thread spins on the clock (first at frame %lu)", g_frame);
        }
    }
    return g_frame_base + g_intra;
}

static ULONGLONG clock_filetime(void)
{
    return EPOCH_FILETIME + (ULONGLONG)(clock_ms() - CLOCK_START_MS) * 10000u;
}

static DWORD WINAPI my_timeGetTime(void) { return clock_ms(); }
static DWORD WINAPI my_GetTickCount(void) { return clock_ms(); }

static BOOL WINAPI my_QueryPerformanceFrequency(LARGE_INTEGER *f)
{
    f->QuadPart = 10000000;
    return TRUE;
}

static BOOL WINAPI my_QueryPerformanceCounter(LARGE_INTEGER *c)
{
    c->QuadPart = (LONGLONG)clock_ms() * 10000;
    return TRUE;
}

static void WINAPI my_GetSystemTimeAsFileTime(FILETIME *ft)
{
    ULONGLONG t = clock_filetime();
    ft->dwLowDateTime = (DWORD)t;
    ft->dwHighDateTime = (DWORD)(t >> 32);
}

static void WINAPI my_GetSystemTime(SYSTEMTIME *st)
{
    FILETIME ft;
    my_GetSystemTimeAsFileTime(&ft);
    FileTimeToSystemTime(&ft, st);
}


static void WINAPI my_GetLocalTime(SYSTEMTIME *st) { my_GetSystemTime(st); }


#define CRT_TIME_ZONE "EST5EDT"
#define LOCAL_BIAS_100NS (4ULL * 3600 * 10000000)

static BOOL WINAPI my_FileTimeToLocalFileTime(const FILETIME *utc, FILETIME *local)
{
    ULONGLONG t = (ULONGLONG)utc->dwHighDateTime << 32 | utc->dwLowDateTime;
    if (t < LOCAL_BIAS_100NS) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    t -= LOCAL_BIAS_100NS;
    local->dwLowDateTime = (DWORD)t;
    local->dwHighDateTime = (DWORD)(t >> 32);
    return TRUE;
}

static __time64_t calendar_seconds(void)
{
    return (__time64_t)((clock_filetime() - 116444736000000000ULL) / 10000000u);
}

static __time64_t __cdecl my_time64(__time64_t *out)
{
    __time64_t t = calendar_seconds();
    if (out)
        *out = t;
    return t;
}

static __time32_t __cdecl my_time32(__time32_t *out)
{
    __time32_t t = (__time32_t)calendar_seconds();
    if (out)
        *out = t;
    return t;
}

static clock_t __cdecl my_clock(void) { return (clock_t)(clock_ms() - CLOCK_START_MS); }


static long long __cdecl my_Xtime_get_ticks(void)
{
    return (long long)(clock_filetime() - 116444736000000000ULL);
}

static void thread_park(int delta);


typedef struct {
    unsigned char bytes[16];
} GrannyClock;

static GrannyClock __stdcall my_GrannyGetSystemSeconds(void)
{
    GrannyClock c = { 0 };
    ULONGLONG t = (ULONGLONG)clock_ms() * 10000u;
    memcpy(c.bytes, &t, sizeof t);
    return c;
}

static float __stdcall my_GrannyGetSecondsElapsed(const GrannyClock *start, const GrannyClock *end)
{
    ULONGLONG a, b;
    memcpy(&a, start->bytes, sizeof a);
    memcpy(&b, end->bytes, sizeof b);
    return (float)((double)(LONGLONG)(b - a) / 1e7);
}


static void WINAPI my_Sleep(DWORD ms)
{
    if (g_trace && ms >= 10 && GetCurrentThreadId() == g_main_tid)
        plog("frame %lu Sleep(%lu) from %p", g_frame, ms, _ReturnAddress());
    if (GetCurrentThreadId() == g_main_tid && ms != INFINITE) {
        g_intra += ms;
        if (!ms)
            SwitchToThread();
        return;
    }
    thread_park(ms != 0);
    Sleep(ms);
    thread_park(-(ms != 0));
}



#define FAKE_SOCKET_BASE 0x7FFF0000u
#define MAX_SOCKETS 32


static int g_split;
static DWORD g_split_seed, g_split_chunk = 16, g_split_frame = 64;
static DWORD g_connects;            

typedef struct {
    
    int used, connected, peer_closed, nonblocking, refused, reset;
    unsigned char *buf;
    DWORD len, cap, head;
    
    DWORD base, shown, settled, recv_cut, frame_cut;
    unsigned long long recv_rng, frame_rng;
} FakeSocket;

static FakeSocket g_socks[MAX_SOCKETS];


static unsigned long long split_next(unsigned long long *s)
{
    unsigned long long z = (*s += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static DWORD split_gap(unsigned long long *s, DWORD max)
{
    return 1 + (DWORD)(split_next(s) % max);
}


static void split_start(FakeSocket *f)
{
    unsigned long long key = (unsigned long long)g_split_seed << 32 | g_connects++;
    f->base = 0u - f->head;
    f->shown = f->settled = f->base + f->len;
    f->recv_rng = key * 2;
    f->frame_rng = key * 2 + 1;
    f->recv_cut = split_gap(&f->recv_rng, g_split_chunk);
    f->frame_cut = g_split_frame ? split_gap(&f->frame_rng, g_split_frame) : 0;
}


static DWORD sock_shown(const FakeSocket *f)
{
    return g_split ? f->shown - f->base : f->len;
}


static int peer_gone(const FakeSocket *f)
{
    return f->peer_closed && sock_shown(f) == f->len;
}


static void split_arrive(void)
{
    int i;
    for (i = 0; i < MAX_SOCKETS; i++) {
        FakeSocket *f = &g_socks[i];
        DWORD delivered = f->base + f->len;
        if (!f->used || !f->connected)
            continue;
        if (!g_split_frame) {
            f->shown = delivered;
            continue;
        }
        if ((int)(f->settled - f->shown) > 0)
            f->shown = f->settled;
        while ((int)(delivered - f->frame_cut) >= 0) {
            f->shown = f->frame_cut;
            f->frame_cut += split_gap(&f->frame_rng, g_split_frame);
        }
        f->settled = delivered;
    }
}


static unsigned char *g_events;
static DWORD g_events_len, g_events_cap;

static void event_add(DWORD type, const void *a, DWORD an, const void *b, DWORD bn)
{
    DWORD need = 8 + an + bn, hdr[2] = { type, an + bn };
    if (g_events_len + need > g_events_cap) {
        g_events_cap = (g_events_len + need) * 2;
        g_events = realloc(g_events, g_events_cap);
        if (!g_events)
            fatal("event buffer");
    }
    memcpy(g_events + g_events_len, hdr, 8);
    if (an)
        memcpy(g_events + g_events_len + 8, a, an);
    if (bn)
        memcpy(g_events + g_events_len + 8 + an, b, bn);
    g_events_len += need;
}

static FakeSocket *fake(SOCKET s)
{
    DWORD i = (DWORD)(s - FAKE_SOCKET_BASE) / 4;
    if (s < FAKE_SOCKET_BASE || (s - FAKE_SOCKET_BASE) % 4 || i >= MAX_SOCKETS || !g_socks[i].used)
        return NULL;
    return &g_socks[i];
}

static DWORD sock_id(FakeSocket *f) { return FAKE_SOCKET_BASE + (DWORD)(f - g_socks) * 4; }

static void sock_append(FakeSocket *f, const unsigned char *p, DWORD n)
{
    if (f->head) {
        memmove(f->buf, f->buf + f->head, f->len - f->head);
        f->base += f->head;
        f->len -= f->head;
        f->head = 0;
    }
    if (f->len + n > f->cap) {
        f->cap = (f->len + n) * 2;
        f->buf = realloc(f->buf, f->cap);
        if (!f->buf)
            fatal("socket buffer");
    }
    memcpy(f->buf + f->len, p, n);
    f->len += n;
}



static HWND g_main_wnd, g_focus_wnd;
static POINT g_cursor;
static unsigned char g_keys[256];
static int g_quit;



static void capture_frame(IDirect3DDevice9 *dev, const char *path);
static void dump_registry(const char *path);
static void (*g_probe)(const char *path);
static int (__cdecl *g_call)(const DWORD *words, DWORD count);
static IDirect3DDevice9 *g_cur_dev;


static void write_window_state(const char *state_path);
static void write_counters(const char *state_path);

static void apply_command(DWORD type, unsigned char *p, DWORD n)
{
    DWORD *w = (DWORD *)p;
    FakeSocket *f;
    switch (type) {
    case PARITY_CMD_DELIVER:
        if ((f = fake(w[0])) != NULL)
            sock_append(f, p + 4, n - 4);
        break;
    case PARITY_CMD_PEER_CLOSE:
        if ((f = fake(w[0])) != NULL)
            f->peer_closed = 1;
        break;
    case PARITY_CMD_PEER_RESET:
        if ((f = fake(w[0])) != NULL) {
            f->reset = n >= 8 && w[1] ? 2 : 1;
            f->head = f->len;
            if (g_split)
                f->shown = f->settled = f->base + f->len;
        }
        break;
    case PARITY_CMD_CONNECTED:
        if ((f = fake(w[0])) != NULL) {
            f->connected = w[1] != 0;
            f->refused = w[1] == 0;
        }
        break;
    case PARITY_CMD_POST: {
        HWND target = w[0] == PARITY_TARGET_FOCUS && g_focus_wnd ? g_focus_wnd : g_main_wnd;
        UINT msg = w[1];
        LPARAM lp = (LPARAM)w[3];
        if (!target)
            break;
        
        if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) {
            POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
            ClientToScreen(g_main_wnd, &pt);
            lp = MAKELPARAM(pt.x, pt.y);
        }
        
        if ((msg == WM_LBUTTONDBLCLK || msg == WM_RBUTTONDBLCLK) &&
            !(GetClassLongA(target, GCL_STYLE) & CS_DBLCLKS))
            msg = msg == WM_LBUTTONDBLCLK ? WM_LBUTTONDOWN : WM_RBUTTONDOWN;
        PostMessageA(target, msg, w[2], lp);
        break;
    }
    case PARITY_CMD_KEY:
        g_keys[w[0] & 0xff] = (unsigned char)(w[1] != 0);
        break;
    case PARITY_CMD_CURSOR:
        g_cursor.x = (LONG)w[0];
        g_cursor.y = (LONG)w[1];
        break;
    case PARITY_CMD_CAPTURE:
        if (g_cur_dev)
            capture_frame(g_cur_dev, (char *)p);
        break;
    case PARITY_CMD_STATE:
        if (g_probe)
            g_probe((char *)p);
        write_window_state((const char *)p);
        write_counters((const char *)p);
        break;
    case PARITY_CMD_REGDUMP:
        dump_registry((char *)p);
        break;
    case PARITY_CMD_CALL:
        
        plog("call %lu %lu at frame %lu", n >= 4 ? w[0] : 0, n >= 8 ? w[1] : 0, g_frame);
        if (g_call && n >= 4)
            plog("call -> %d", g_call(w, n / 4));
        break;
    case PARITY_CMD_QUIT:
        g_quit = 1;
        break;
    }
}


static void write_window_state(const char *state_path)
{
    char path[MAX_PATH + 8];
    FILE *f;
    RECT rc = { 0, 0, 0, 0 };
    LONG style, ex;
    if (!g_main_wnd || _snprintf(path, sizeof path, "%s.window", state_path) < 0)
        return;
    path[sizeof path - 1] = 0;
    style = GetWindowLongA(g_main_wnd, GWL_STYLE) & ~(LONG)WS_VISIBLE;
    ex = GetWindowLongA(g_main_wnd, GWL_EXSTYLE) & ~(LONG)WS_EX_NOACTIVATE;
    GetClientRect(g_main_wnd, &rc);
    if ((f = fopen(path, "w")) != NULL) {
        fprintf(f, "style=%08lx exstyle=%08lx client=%ldx%ld\n", (unsigned long)style, (unsigned long)ex,
                rc.right - rc.left, rc.bottom - rc.top);
        fclose(f);
    }
}


static void sync_point(DWORD reason)
{
    DWORD body[4] = { reason, g_frame, clock_ms(), g_sync_index++ };
    if (g_events_len)
        pipe_write(g_events, g_events_len);
    g_events_len = 0;
    send_msg(PARITY_SYNC, body, sizeof body, NULL, 0);
    for (;;) {
        DWORD hdr[2];
        unsigned char *p;
        pipe_read(hdr, sizeof hdr);
        if (hdr[0] == PARITY_CMD_GO) {
            if (hdr[1])
                fatal("GO with a payload");
            break;
        }
        p = malloc(hdr[1] + 1);
        if (!p)
            fatal("command buffer");
        pipe_read(p, hdr[1]);
        p[hdr[1]] = 0;
        apply_command(hdr[0], p, hdr[1]);
        free(p);
    }
    if (g_split)
        split_arrive();
    if (g_quit) {
        plog("quit at frame %lu", g_frame);
        coverage_dump();
        TerminateProcess(GetCurrentProcess(), 0);
    }
}



static SOCKET WSAAPI my_socket(int af, int type, int proto)
{
    int i;
    (void)proto;
    if (af != AF_INET || type != SOCK_STREAM) {
        plog("socket(%d, %d) refused", af, type);
        WSASetLastError(WSAEAFNOSUPPORT);
        return INVALID_SOCKET;
    }
    EnterCriticalSection(&g_lock);
    for (i = 0; i < MAX_SOCKETS && g_socks[i].used; i++)
        ;
    if (i == MAX_SOCKETS) {
        LeaveCriticalSection(&g_lock);
        WSASetLastError(WSAEMFILE);
        return INVALID_SOCKET;
    }
    memset(&g_socks[i], 0, sizeof g_socks[i]);
    g_socks[i].used = 1;
    LeaveCriticalSection(&g_lock);
    return FAKE_SOCKET_BASE + (DWORD)i * 4;
}

static int WSAAPI my_connect(SOCKET s, const struct sockaddr *sa, int len)
{
    FakeSocket *f;
    const struct sockaddr_in *in = (const struct sockaddr_in *)sa;
    DWORD body[3];
    int ok;
    (void)len;
    EnterCriticalSection(&g_lock);
    if ((f = fake(s)) == NULL || sa->sa_family != AF_INET) {
        LeaveCriticalSection(&g_lock);
        WSASetLastError(WSAENOTSOCK);
        return SOCKET_ERROR;
    }
    body[0] = sock_id(f);
    body[1] = in->sin_addr.s_addr;
    body[2] = ntohs(in->sin_port);
    event_add(PARITY_EV_CONNECT, body, sizeof body, NULL, 0);
    f->connected = f->refused = 0;
    if (g_split)
        split_start(f);
    while (!f->connected && !f->refused)
        sync_point(PARITY_SYNC_CONNECT);
    ok = f->connected;
    LeaveCriticalSection(&g_lock);
    if (!ok) {
        WSASetLastError(WSAECONNREFUSED);
        return SOCKET_ERROR;
    }
    return 0;
}

static int WSAAPI my_send(SOCKET s, const char *p, int n, int flags)
{
    FakeSocket *f;
    DWORD id;
    (void)flags;
    EnterCriticalSection(&g_lock);
    if ((f = fake(s)) == NULL || !f->connected || f->peer_closed || f->reset) {
        if (f && f->reset)
            f->reset = 1;
        LeaveCriticalSection(&g_lock);
        WSASetLastError(f ? WSAECONNRESET : WSAENOTSOCK);
        return SOCKET_ERROR;
    }
    id = sock_id(f);
    if (n > 0)
        event_add(PARITY_EV_SEND, &id, 4, p, (DWORD)n);
    LeaveCriticalSection(&g_lock);
    return n;
}

static int WSAAPI my_WSASend(SOCKET s, LPWSABUF bufs, DWORD count, LPDWORD sent, DWORD flags,
                             LPWSAOVERLAPPED ov, LPWSAOVERLAPPED_COMPLETION_ROUTINE done)
{
    DWORD i, total = 0;
    (void)flags;
    if (ov || done) {
        WSASetLastError(WSAEOPNOTSUPP);
        return SOCKET_ERROR;
    }
    for (i = 0; i < count; i++) {
        int r = my_send(s, bufs[i].buf, (int)bufs[i].len, 0);
        if (r == SOCKET_ERROR)
            return SOCKET_ERROR;
        total += (DWORD)r;
    }
    if (sent)
        *sent = total;
    return 0;
}

static int WSAAPI my_recv(SOCKET s, char *p, int n, int flags)
{
    FakeSocket *f;
    DWORD avail;
    EnterCriticalSection(&g_lock);
    if (g_trace)
        plog("frame %lu recv(%x, %d, %d)", g_frame, (unsigned)s, n, flags);
    if ((f = fake(s)) == NULL) {
        LeaveCriticalSection(&g_lock);
        WSASetLastError(WSAENOTSOCK);
        return SOCKET_ERROR;
    }
    while (sock_shown(f) == f->head && !peer_gone(f) && f->reset != 1 && !f->nonblocking)
        sync_point(PARITY_SYNC_NETWAIT);
    if (f->reset == 1) {
        LeaveCriticalSection(&g_lock);
        WSASetLastError(WSAECONNRESET);
        return SOCKET_ERROR;
    }
    avail = sock_shown(f) - f->head;
    if (!avail) {
        int closed = peer_gone(f);
        LeaveCriticalSection(&g_lock);
        if (closed)
            return 0;
        WSASetLastError(WSAEWOULDBLOCK);
        return SOCKET_ERROR;
    }
    if (g_split) {
        DWORD at = f->base + f->head;
        while ((int)(f->recv_cut - at) <= 0)
            f->recv_cut += split_gap(&f->recv_rng, g_split_chunk);
        if (avail > f->recv_cut - at)
            avail = f->recv_cut - at;
    }
    if ((DWORD)n > avail)
        n = (int)avail;
    memcpy(p, f->buf + f->head, (size_t)n);
    if (!(flags & MSG_PEEK))
        f->head += (DWORD)n;
    if (g_trace)
        plog("frame %lu recv -> %d bytes, %lu left", g_frame, n, sock_shown(f) - f->head);
    LeaveCriticalSection(&g_lock);
    return n;
}

static int WSAAPI my_ioctlsocket(SOCKET s, long cmd, u_long *arg)
{
    FakeSocket *f;
    int r = 0;
    EnterCriticalSection(&g_lock);
    if ((f = fake(s)) == NULL) {
        r = SOCKET_ERROR;
        WSASetLastError(WSAENOTSOCK);
    } else if (g_trace && (plog("frame %lu ioctlsocket(%x, 0x%lx)", g_frame, (unsigned)s, (unsigned long)cmd), 0)) {
    } else if (cmd == FIONBIO) {
        f->nonblocking = *arg != 0;
    } else if (cmd == FIONREAD) {
        *arg = sock_shown(f) - f->head;
    }
    LeaveCriticalSection(&g_lock);
    return r;
}

static int set_ready(fd_set *set, int (*ready)(FakeSocket *))
{
    u_int i, keep = 0;
    if (!set)
        return 0;
    for (i = 0; i < set->fd_count; i++) {
        FakeSocket *f = fake(set->fd_array[i]);
        if (f && ready(f))
            set->fd_array[keep++] = set->fd_array[i];
    }
    set->fd_count = keep;
    return (int)keep;
}

static int readable(FakeSocket *f) { return sock_shown(f) > f->head || peer_gone(f) || f->reset == 1; }
static int writable(FakeSocket *f) { return f->connected && !f->peer_closed && f->reset != 1; }
static int never(FakeSocket *f) { (void)f; return 0; }


static int WSAAPI my_select(int nfds, fd_set *r, fd_set *w, fd_set *e, const struct timeval *t)
{
    fd_set r0 = { 0 }, w0 = { 0 };
    int n;
    (void)nfds;
    EnterCriticalSection(&g_lock);
    if (r)
        r0 = *r;
    if (w)
        w0 = *w;
    n = set_ready(r, readable) + set_ready(w, writable);
    if (g_trace)
        plog("frame %lu select(r %u, w %u, timeout %ld) -> %d", g_frame, r ? r0.fd_count : 0, w ? w0.fd_count : 0,
             t ? (long)(t->tv_sec * 1000 + t->tv_usec / 1000) : -1L, n);
    while (!n && (!t || t->tv_sec || t->tv_usec)) {
        if (r)
            *r = r0;
        if (w)
            *w = w0;
        sync_point(PARITY_SYNC_NETWAIT);
        n = set_ready(r, readable) + set_ready(w, writable);
        if (!n && t) {
            if (GetCurrentThreadId() == g_main_tid)
                g_intra += (DWORD)(t->tv_sec * 1000 + t->tv_usec / 1000);
            break;
        }
    }
    set_ready(e, never);
    LeaveCriticalSection(&g_lock);
    return n;
}

static int WSAAPI my_closesocket(SOCKET s)
{
    FakeSocket *f;
    DWORD id;
    EnterCriticalSection(&g_lock);
    if ((f = fake(s)) == NULL) {
        LeaveCriticalSection(&g_lock);
        WSASetLastError(WSAENOTSOCK);
        return SOCKET_ERROR;
    }
    id = sock_id(f);
    event_add(PARITY_EV_CLOSE, &id, 4, NULL, 0);
    free(f->buf);
    memset(f, 0, sizeof *f);
    LeaveCriticalSection(&g_lock);
    return 0;
}

static int WSAAPI my_setsockopt(SOCKET s, int level, int name, const char *v, int len)
{
    (void)level; (void)name; (void)v; (void)len;
    if (!fake(s)) {
        WSASetLastError(WSAENOTSOCK);
        return SOCKET_ERROR;
    }
    return 0;
}




#define ARENA_BASE ((void *)0x40000000)
#define ARENA_SIZE (1024u << 20)
#define ARENA_COMMIT (4u << 20)
#define ARENA_MIN_SIZE (64u << 20)
static DWORD g_arena_size;
#define BLOCK_MARK 0x59544950u
#define MAX_CLASSES 128

typedef struct {
    DWORD cls, mark, size, pad;
} BlockHeader;

typedef void *(__cdecl *calloc_fn)(size_t, size_t);
typedef void *(__cdecl *realloc_fn)(void *, size_t);
typedef void(__cdecl *free_fn)(void *);
typedef size_t(__cdecl *msize_fn)(void *);
static calloc_fn g_real_calloc;
static realloc_fn g_real_realloc;
static free_fn g_real_free;
static msize_fn g_real_msize;

static CRITICAL_SECTION g_heap_lock;
static unsigned char *g_arena, *g_arena_end, *g_bump, *g_committed;
static DWORD g_class_size[MAX_CLASSES];
static int g_class_count;
static void *g_free_list[MAX_CLASSES];
static LONGLONG g_arena_live, g_arena_peak, g_arena_allocs;   
static DWORD g_heap_trace = 2166136261u;   
static int g_asan;                          


static void heap_trace(const void *p, DWORD tag)
{
    if (g_trace)
        g_heap_trace = ((g_heap_trace ^ (DWORD)(UINT_PTR)p ^ tag) * 16777619u);
}

static void arena_init(void)
{
    DWORD size = 16;
    InitializeCriticalSection(&g_heap_lock);
    while (g_class_count < MAX_CLASSES && size <= (256u << 20)) {
        g_class_size[g_class_count++] = size;
        size = size < 256 ? size + 16 : ((DWORD)((double)size * 1.1892) + 15) & ~15u;
    }
    if (g_asan)
        return;
    g_arena_size = ARENA_SIZE;
    g_arena = VirtualAlloc(ARENA_BASE, g_arena_size, MEM_RESERVE, PAGE_READWRITE);
    while (!g_arena && g_arena_size >= ARENA_MIN_SIZE) {
        g_arena = VirtualAlloc(NULL, g_arena_size, MEM_RESERVE, PAGE_READWRITE);
        if (!g_arena)
            g_arena_size /= 2;
    }
    if (!g_arena)
        fatal("arena reservation");
    if (g_arena != ARENA_BASE || g_arena_size != ARENA_SIZE)
        plog("the arena is %lu MB at %p, not %u MB at %p", g_arena_size >> 20, g_arena, ARENA_SIZE >> 20, ARENA_BASE);
    g_arena_end = g_arena + g_arena_size;
    g_bump = g_committed = g_arena;
}

static int size_class(size_t n)
{
    int lo = 0, hi = g_class_count - 1;
    if (n > g_class_size[hi])
        return -1;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (g_class_size[mid] >= n)
            hi = mid;
        else
            lo = mid + 1;
    }
    return lo;
}

static BlockHeader *header_of(void *p)
{
    unsigned char *c = p;
    if (c < g_arena + sizeof(BlockHeader) || c >= g_arena_end)
        return NULL;
    return (BlockHeader *)(c - sizeof(BlockHeader));
}

static void *arena_alloc(size_t n)
{
    int cls = size_class(n ? n : 1);
    BlockHeader *h;
    unsigned char *block;
    if (cls < 0)
        return g_real_calloc(1, n);
    EnterCriticalSection(&g_heap_lock);
    if (g_free_list[cls]) {
        block = g_free_list[cls];
        g_free_list[cls] = *(void **)block;
        memset(block, 0, g_class_size[cls]);
    } else {
        size_t need = sizeof(BlockHeader) + g_class_size[cls];
        if ((size_t)(g_arena_end - g_bump) < need) {
            LeaveCriticalSection(&g_heap_lock);
            plog("the arena is full; the CRT serves the rest");
            return g_real_calloc(1, n);
        }
        while (g_bump + need > g_committed) {
            if (!VirtualAlloc(g_committed, ARENA_COMMIT, MEM_COMMIT, PAGE_READWRITE))
                fatal("arena commit");
            g_committed += ARENA_COMMIT;
        }
        block = g_bump + sizeof(BlockHeader);
        g_bump += need;
    }
    h = (BlockHeader *)(block - sizeof(BlockHeader));
    h->cls = (DWORD)cls;
    h->mark = BLOCK_MARK;
    h->size = (DWORD)n;
    heap_trace(block, 0);
    g_arena_allocs++;
    g_arena_live += g_class_size[cls];
    if (g_arena_live > g_arena_peak)
        g_arena_peak = g_arena_live;
    LeaveCriticalSection(&g_heap_lock);
    return block;
}


static void *g_free_caller;

static void arena_free(void *p)
{
    BlockHeader *h;
    if (!p)
        return;
    h = header_of(p);
    if (!h) {
        g_real_free(p);
        return;
    }
    if (h->mark != BLOCK_MARK) {
        HMODULE mod = NULL;
        char name[MAX_PATH] = "?";
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)g_free_caller, &mod);
        if (mod)
            GetModuleFileNameA(mod, name, sizeof(name));
        plog("a free of %p from %p (%s + 0x%lx): its block's mark is %08lx (freed before, or overrun)", p,
             g_free_caller, name, (unsigned long)((BYTE *)g_free_caller - (BYTE *)mod), (unsigned long)h->mark);
        fatal("free of a damaged or foreign arena block");
    }
    EnterCriticalSection(&g_heap_lock);
    h->mark = 0;
    heap_trace(p, 1);
    g_arena_live -= g_class_size[h->cls];
    *(void **)p = g_free_list[h->cls];
    g_free_list[h->cls] = p;
    LeaveCriticalSection(&g_heap_lock);
}

static void *__cdecl my_malloc(size_t n) { return arena_alloc(n); }

static void *__cdecl my_calloc(size_t count, size_t size)
{
    if (size && count > (size_t)-1 / size)
        return NULL;
    return arena_alloc(count * size);
}

static void __cdecl my_free(void *p)
{
    g_free_caller = _ReturnAddress();
    arena_free(p);
}

static void *__cdecl my_realloc(void *p, size_t n)
{
    BlockHeader *h;
    void *q;
    if (!p)
        return arena_alloc(n);
    h = header_of(p);
    if (!h) {
        size_t old = g_real_msize ? g_real_msize(p) : 0;
        unsigned char *r = g_real_realloc(p, n);
        if (r && n > old)
            memset(r + old, 0, n - old);
        return r;
    }
    if (!n) {
        arena_free(p);
        return NULL;
    }
    if (size_class(n) == (int)h->cls) {
        if (n > h->size)
            memset((unsigned char *)p + h->size, 0, n - h->size);
        h->size = (DWORD)n;
        return p;
    }
    q = arena_alloc(n);
    if (!q)
        return NULL;
    memcpy(q, p, h->size < n ? h->size : n);
    arena_free(p);
    return q;
}

static void *__cdecl my_recalloc(void *p, size_t count, size_t size)
{
    if (size && count > (size_t)-1 / size)
        return NULL;
    return my_realloc(p, count * size);
}

static char *__cdecl my_strdup(const char *s)
{
    size_t n;
    char *d;
    if (!s)
        return NULL;
    n = strlen(s) + 1;
    d = arena_alloc(n);
    if (d)
        memcpy(d, s, n);
    return d;
}

static wchar_t *__cdecl my_wcsdup(const wchar_t *s)
{
    size_t n;
    wchar_t *d;
    if (!s)
        return NULL;
    n = (wcslen(s) + 1) * sizeof(wchar_t);
    d = arena_alloc(n);
    if (d)
        memcpy(d, s, n);
    return d;
}


static size_t __cdecl my_msize(void *p)
{
    BlockHeader *h = header_of(p);
    if (h && h->mark == BLOCK_MARK)
        return h->size;
    return g_real_msize ? g_real_msize(p) : 0;
}

static LPVOID WINAPI my_HeapAlloc(HANDLE heap, DWORD flags, SIZE_T n)
{
    return HeapAlloc(heap, flags | HEAP_ZERO_MEMORY, n);
}

static LPVOID WINAPI my_HeapReAlloc(HANDLE heap, DWORD flags, LPVOID p, SIZE_T n)
{
    return HeapReAlloc(heap, flags | HEAP_ZERO_MEMORY, p, n);
}

static HGLOBAL WINAPI my_GlobalAlloc(UINT flags, SIZE_T n)
{
    return GlobalAlloc(flags | GMEM_ZEROINIT, n);
}




#define MAX_THREADS 64
#define BARRIER_MS 2000

typedef struct {
    LPTHREAD_START_ROUTINE start;
    void *arg;
    HANDLE handle;
    volatile LONG parked;
    volatile LONG started;
    DWORD tid;
} Tracked;

static Tracked g_threads[MAX_THREADS];
static volatile LONG g_thread_count;
static DWORD g_tls = TLS_OUT_OF_INDEXES;

static void settle_new(Tracked *t, DWORD flags);

static DWORD WINAPI tracked_start(void *p)
{
    Tracked *t = p;
    TlsSetValue(g_tls, t);
    InterlockedExchange(&t->started, 1);
    return t->start(t->arg);
}

static Tracked *track_new(LPTHREAD_START_ROUTINE start, void *arg)
{
    LONG i = InterlockedIncrement(&g_thread_count) - 1;
    if (i >= MAX_THREADS) {
        plog("more than %d client threads; the rest run untracked", MAX_THREADS);
        return NULL;
    }
    g_threads[i].start = start;
    g_threads[i].arg = arg;
    return &g_threads[i];
}

static HANDLE WINAPI my_CreateThread(LPSECURITY_ATTRIBUTES sa, SIZE_T stack, LPTHREAD_START_ROUTINE start, LPVOID arg,
                                     DWORD flags, LPDWORD tid)
{
    Tracked *t = track_new(start, arg);
    HANDLE h;
    DWORD id;
    if (!t)
        return CreateThread(sa, stack, start, arg, flags, tid);
    h = CreateThread(sa, stack, tracked_start, t, flags, &id);
    if (h) {
        DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &t->handle, SYNCHRONIZE, FALSE, 0);
        t->tid = id;
    }
    if (tid)
        *tid = id;
    settle_new(t, flags);
    return h;
}

typedef unsigned(__stdcall *ThreadEx_fn)(void *);
typedef uintptr_t(__cdecl *beginthreadex_fn)(void *, unsigned, ThreadEx_fn, void *, unsigned, unsigned *);
static beginthreadex_fn g_real_beginthreadex;

static uintptr_t __cdecl my_beginthreadex(void *sa, unsigned stack, ThreadEx_fn start, void *arg, unsigned flags,
                                          unsigned *tid)
{
    Tracked *t = track_new((LPTHREAD_START_ROUTINE)(void *)start, arg);
    uintptr_t h;
    unsigned id;
    if (!t)
        return g_real_beginthreadex(sa, stack, start, arg, flags, tid);
    h = g_real_beginthreadex(sa, stack, (ThreadEx_fn)(void *)tracked_start, t, flags, &id);
    if (h) {
        DuplicateHandle(GetCurrentProcess(), (HANDLE)h, GetCurrentProcess(), &t->handle, SYNCHRONIZE, FALSE, 0);
        t->tid = id;
    }
    if (tid)
        *tid = id;
    settle_new(t, flags);
    return h;
}

static Tracked *tracked_self(void)
{
    return g_tls == TLS_OUT_OF_INDEXES ? NULL : (Tracked *)TlsGetValue(g_tls);
}

static DWORD WINAPI my_WaitForSingleObject(HANDLE h, DWORD ms)
{
    Tracked *t = tracked_self();
    DWORD r;
    if (t)
        InterlockedIncrement(&t->parked);
    r = WaitForSingleObject(h, ms);
    if (t)
        InterlockedDecrement(&t->parked);
    return r;
}

static DWORD WINAPI my_WaitForSingleObjectEx(HANDLE h, DWORD ms, BOOL alertable)
{
    Tracked *t = tracked_self();
    DWORD r;
    if (t)
        InterlockedIncrement(&t->parked);
    r = WaitForSingleObjectEx(h, ms, alertable);
    if (t)
        InterlockedDecrement(&t->parked);
    return r;
}

static DWORD WINAPI my_WaitForMultipleObjects(DWORD n, const HANDLE *hs, BOOL all, DWORD ms)
{
    Tracked *t = tracked_self();
    DWORD r;
    if (t)
        InterlockedIncrement(&t->parked);
    r = WaitForMultipleObjects(n, hs, all, ms);
    if (t)
        InterlockedDecrement(&t->parked);
    return r;
}


static void thread_park(int delta)
{
    Tracked *t = tracked_self();
    if (t && delta > 0)
        InterlockedIncrement(&t->parked);
    else if (t && delta < 0)
        InterlockedDecrement(&t->parked);
}


static int thread_settle(Tracked *t, DWORD *waited)
{
    for (;;) {
        if (!t->handle || WaitForSingleObject(t->handle, 0) == WAIT_OBJECT_0)
            return 1;
        if (t->started && t->parked > 0)
            return 1;
        if (*waited >= BARRIER_MS) {
            plog("frame %lu: client thread %lu neither ended nor parked within %d ms", g_frame, t->tid, BARRIER_MS);
            return 0;
        }
        Sleep(*waited < 20 ? 0 : 1);
        ++*waited;
    }
}


static void thread_barrier(void)
{
    LONG n = g_thread_count < MAX_THREADS ? g_thread_count : MAX_THREADS, i;
    DWORD waited = 0;
    for (i = 0; i < n; i++)
        if (!thread_settle(&g_threads[i], &waited))
            return;
}


static void settle_new(Tracked *t, DWORD flags)
{
    DWORD waited = 0;
    if (t && t->handle && !(flags & CREATE_SUSPENDED) && GetCurrentThreadId() == g_main_tid)
        thread_settle(t, &waited);
}



static int offscreen_x(void)
{
    return GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN) + 64;
}

static HWND WINAPI my_CreateWindowExA(DWORD ex, LPCSTR cls, LPCSTR name, DWORD style, int x, int y,
                                      int w, int h, HWND parent, HMENU menu, HINSTANCE inst, LPVOID param)
{
    int top = !(style & WS_CHILD);
    HWND wnd;
    if (top) {
        ex |= WS_EX_NOACTIVATE;
        ex &= ~(DWORD)WS_EX_TOPMOST;
        x = offscreen_x();
        y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    }
    wnd = CreateWindowExA(ex, cls, name, top ? style & ~(DWORD)WS_VISIBLE : style, x, y, w, h, parent, menu,
                          inst, param);
    if (wnd && top && !g_main_wnd && !parent) {
        g_main_wnd = g_focus_wnd = wnd;
        plog("main window %p (%dx%d)", (void *)wnd, w, h);
    }
    if (wnd && top && (style & WS_VISIBLE))
        ShowWindow(wnd, SW_SHOWNOACTIVATE);
    return wnd;
}

static BOOL WINAPI my_ShowWindow(HWND wnd, int cmd)
{
    if (cmd == SW_HIDE || cmd == SW_MINIMIZE || cmd == SW_SHOWMINNOACTIVE || cmd == SW_FORCEMINIMIZE)
        return ShowWindow(wnd, cmd == SW_HIDE ? SW_HIDE : SW_SHOWMINNOACTIVE);
    return ShowWindow(wnd, IsWindowVisible(wnd) ? SW_SHOWNA : SW_SHOWNOACTIVATE);
}

static BOOL WINAPI my_SetWindowPos(HWND wnd, HWND after, int x, int y, int w, int h, UINT flags)
{
    if (wnd == g_main_wnd) {
        x = offscreen_x();
        y = GetSystemMetrics(SM_YVIRTUALSCREEN);
        after = NULL;
        flags |= SWP_NOZORDER;
    }
    return SetWindowPos(wnd, after, x, y, w, h, flags | SWP_NOACTIVATE);
}

static BOOL WINAPI my_MoveWindow(HWND wnd, int x, int y, int w, int h, BOOL repaint)
{
    return my_SetWindowPos(wnd, NULL, x, y, w, h, SWP_NOZORDER | (repaint ? 0 : SWP_NOREDRAW));
}

static HWND WINAPI my_SetFocus(HWND wnd)
{
    HWND prev = g_focus_wnd;
    g_focus_wnd = wnd ? wnd : g_main_wnd;
    return prev;
}

static BOOL WINAPI my_SetForegroundWindow(HWND wnd) { (void)wnd; return TRUE; }
static HWND WINAPI my_SetActiveWindow(HWND wnd) { (void)wnd; return g_main_wnd; }
static HWND WINAPI my_GetForegroundWindow(void) { return g_main_wnd; }
static HWND WINAPI my_GetActiveWindow(void) { return g_main_wnd; }
static HWND WINAPI my_SetCapture(HWND wnd) { (void)wnd; return NULL; }
static BOOL WINAPI my_ReleaseCapture(void) { return TRUE; }
static BOOL WINAPI my_ClipCursor(const RECT *r) { (void)r; return TRUE; }

static BOOL WINAPI my_GetCursorPos(POINT *p)
{
    *p = g_cursor;
    if (g_main_wnd)
        ClientToScreen(g_main_wnd, p);
    return TRUE;
}

static BOOL WINAPI my_SetCursorPos(int x, int y)
{
    POINT p = { x, y };
    if (g_main_wnd)
        ScreenToClient(g_main_wnd, &p);
    g_cursor = p;
    return TRUE;
}


static SHORT WINAPI my_GetAsyncKeyState(int vk) { return g_keys[vk & 0xff] ? SHRT_MIN : 0; }
static SHORT WINAPI my_GetKeyState(int vk) { return g_keys[vk & 0xff] ? (SHORT)-128 : 0; }


static int us_key_char(UINT vk, int shift, int ctrl)
{
    static const char digits[] = ")!@#$%^&*(";
    static const struct { UINT vk; char plain, shifted; } oem[] = {
        { 0xBA, ';', ':' }, { 0xBB, '=', '+' }, { 0xBC, ',', '<' }, { 0xBD, '-', '_' }, { 0xBE, '.', '>' },
        { 0xBF, '/', '?' }, { 0xC0, '`', '~' }, { 0xDB, '[', '{' }, { 0xDC, '\\', '|' }, { 0xDD, ']', '}' },
        { 0xDE, '\'', '"' },
    };
    int i;
    if (vk >= 'A' && vk <= 'Z')
        return ctrl ? (int)(vk - 'A' + 1) : (int)(shift ? vk : vk + 32);
    if (vk == VK_BACK)
        return ctrl ? 0x7f : 0x08;
    if (vk == VK_RETURN)
        return ctrl ? 0x0a : 0x0d;
    if (vk == VK_ESCAPE)
        return 0x1b;
    if (vk == VK_TAB)
        return ctrl ? -1 : 0x09;
    if (ctrl)
        return -1;
    if (vk == VK_SPACE)
        return ' ';
    if (vk >= '0' && vk <= '9')
        return shift ? digits[vk - '0'] : (int)vk;
    for (i = 0; i < (int)(sizeof oem / sizeof oem[0]); i++)
        if (oem[i].vk == vk)
            return shift ? oem[i].shifted : oem[i].plain;
    return -1;
}


static BOOL WINAPI my_TranslateMessage(const MSG *m)
{
    int ch;
    if (!m)
        return FALSE;
    if (m->message == WM_KEYUP || m->message == WM_SYSKEYUP)
        return TRUE;
    if (m->message != WM_KEYDOWN && m->message != WM_SYSKEYDOWN)
        return FALSE;
    ch = us_key_char((UINT)m->wParam, g_keys[VK_SHIFT] != 0, g_keys[VK_CONTROL] != 0);
    if (ch >= 0)
        PostMessageA(m->hwnd, m->message == WM_SYSKEYDOWN ? WM_SYSCHAR : WM_CHAR, (WPARAM)ch, m->lParam);
    return TRUE;
}





static volatile LONG g_n_draws, g_n_states, g_n_set_textures, g_n_textures, g_n_vbuffers, g_n_file_opens, g_n_texts;

static void write_counters(const char *state_path)
{
    char path[MAX_PATH + 16];
    FILE *f;
    if (_snprintf(path, sizeof path, "%s.counters", state_path) < 0)
        return;
    path[sizeof path - 1] = 0;
    if ((f = fopen(path, "w")) == NULL)
        return;
    EnterCriticalSection(&g_heap_lock);
    fprintf(f, "draws=%ld render_states=%ld set_textures=%ld textures_made=%ld vertex_buffers_made=%ld "
            "files_opened=%ld texts_drawn=%ld arena_allocs=%lld arena_live_kb=%lld arena_peak_kb=%lld\n",
            g_n_draws, g_n_states, g_n_set_textures, g_n_textures, g_n_vbuffers, g_n_file_opens, g_n_texts,
            g_arena_allocs, g_arena_live >> 10, g_arena_peak >> 10);
    LeaveCriticalSection(&g_heap_lock);
    fclose(f);
}


#define SHARE_RETRY_MS     25
#define SHARE_RETRY_TRIES  200

static HANDLE WINAPI my_CreateFileA(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa, DWORD disp,
                                    DWORD flags, HANDLE templ)
{
    HANDLE h;
    int tries = 0;
    InterlockedIncrement(&g_n_file_opens);
    while ((h = CreateFileA(name, access, share, sa, disp, flags, templ)) == INVALID_HANDLE_VALUE
           && GetLastError() == ERROR_SHARING_VIOLATION && ++tries < SHARE_RETRY_TRIES)
        Sleep(SHARE_RETRY_MS);
    return h;
}

static HANDLE WINAPI my_CreateFileW(LPCWSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa, DWORD disp,
                                    DWORD flags, HANDLE templ)
{
    HANDLE h;
    int tries = 0;
    InterlockedIncrement(&g_n_file_opens);
    while ((h = CreateFileW(name, access, share, sa, disp, flags, templ)) == INVALID_HANDLE_VALUE
           && GetLastError() == ERROR_SHARING_VIOLATION && ++tries < SHARE_RETRY_TRIES)
        Sleep(SHARE_RETRY_MS);
    return h;
}

static BOOL WINAPI my_TextOutA(HDC dc, int x, int y, LPCSTR s, int n)
{
    InterlockedIncrement(&g_n_texts);
    return TextOutA(dc, x, y, s, n);
}

static BOOL WINAPI my_TextOutW(HDC dc, int x, int y, LPCWSTR s, int n)
{
    InterlockedIncrement(&g_n_texts);
    return TextOutW(dc, x, y, s, n);
}

static BOOL WINAPI my_ExtTextOutW(HDC dc, int x, int y, UINT opt, const RECT *rc, LPCWSTR s, UINT n, const INT *dx)
{
    InterlockedIncrement(&g_n_texts);
    return ExtTextOutW(dc, x, y, opt, rc, s, n, dx);
}



static void report_dialog(const char *kind, const char *text)
{
    char buf[1024];
    _snprintf(buf, sizeof buf - 1, "%s: %s", kind, text ? text : "");
    buf[sizeof buf - 1] = 0;
    plog("%s", buf);
    EnterCriticalSection(&g_lock);
    event_add(PARITY_EV_DIALOG, buf, (DWORD)strlen(buf), NULL, 0);
    LeaveCriticalSection(&g_lock);
}

static int WINAPI my_MessageBoxA(HWND wnd, LPCSTR text, LPCSTR caption, UINT type)
{
    char kind[160];
    (void)wnd;
    _snprintf(kind, sizeof kind - 1, "MessageBox [%.120s]", caption ? caption : "");
    kind[sizeof kind - 1] = 0;
    report_dialog(kind, text);
    switch (type & 0xf) {
    case MB_YESNO: case MB_YESNOCANCEL: return IDYES;
    case MB_RETRYCANCEL: return IDCANCEL;
    case MB_ABORTRETRYIGNORE: return IDABORT;
    default: return IDOK;
    }
}

static INT_PTR WINAPI my_DialogBoxParamA(HINSTANCE inst, LPCSTR tmpl, HWND parent, DLGPROC proc, LPARAM lp)
{
    char id[32];
    (void)inst; (void)parent; (void)proc; (void)lp;
    if (IS_INTRESOURCE(tmpl))
        _snprintf(id, sizeof id, "#%u", (unsigned)(UINT_PTR)tmpl);
    else
        _snprintf(id, sizeof id, "%.24s", tmpl);
    id[sizeof id - 1] = 0;
    report_dialog("DialogBoxParam", id);
    return IDCANCEL;
}

static HINSTANCE WINAPI my_ShellExecuteA(HWND wnd, LPCSTR op, LPCSTR file, LPCSTR params, LPCSTR dir, INT show)
{
    (void)wnd; (void)op; (void)params; (void)dir; (void)show;
    report_dialog("ShellExecute", file);
    return (HINSTANCE)(INT_PTR)33;
}

static BOOL WINAPI my_CreateProcessA(LPCSTR app, LPSTR cmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta,
                                     BOOL inherit, DWORD flags, LPVOID env, LPCSTR dir,
                                     LPSTARTUPINFOA si, LPPROCESS_INFORMATION pi)
{
    (void)pa; (void)ta; (void)inherit; (void)flags; (void)env; (void)dir; (void)si; (void)pi;
    report_dialog("CreateProcess", cmd ? cmd : app);
    SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
}


static BOOL WINAPI my_SystemParametersInfoA(UINT action, UINT ui, PVOID pv, UINT win_ini)
{
    switch (action) {
    case SPI_GETWORKAREA: case SPI_GETSCREENSAVEACTIVE: case SPI_GETSTICKYKEYS: case SPI_GETTOGGLEKEYS:
    case SPI_GETFILTERKEYS: case SPI_GETKEYBOARDDELAY: case SPI_GETKEYBOARDSPEED: case SPI_GETMOUSESPEED:
    case SPI_GETWHEELSCROLLLINES: case SPI_GETNONCLIENTMETRICS: case SPI_GETMOUSE:
        return SystemParametersInfoA(action, ui, pv, win_ini);
    }
    plog("SystemParametersInfoA(0x%x) not passed on", action);
    return TRUE;
}

static LRESULT WINAPI my_SendMessageTimeoutA(HWND wnd, UINT msg, WPARAM wp, LPARAM lp, UINT flags, UINT ms, PDWORD_PTR res)
{
    if (wnd == HWND_BROADCAST) {
        plog("broadcast message 0x%x not sent", msg);
        if (res)
            *res = 0;
        return 1;
    }
    return SendMessageTimeoutA(wnd, msg, wp, lp, flags, ms, res);
}


static BOOL WINAPI my_K32GetProcessMemoryInfo(HANDLE proc, PPROCESS_MEMORY_COUNTERS pmc, DWORD cb)
{
    (void)proc;
    if (!pmc || cb < sizeof *pmc)
        return FALSE;
    memset(pmc, 0, cb);
    pmc->cb = cb;
    pmc->WorkingSetSize = pmc->PeakWorkingSetSize = 256u << 20;
    pmc->PagefileUsage = pmc->PeakPagefileUsage = 320u << 20;
    return TRUE;
}


static LONG WINAPI my_ChangeDisplaySettingsA(DEVMODEA *mode, DWORD flags)
{
    (void)mode; (void)flags;
    report_dialog("ChangeDisplaySettings", "refused");
    return DISP_CHANGE_SUCCESSFUL;
}

static LONG WINAPI my_ChangeDisplaySettingsExA(LPCSTR device, DEVMODEA *mode, HWND wnd, DWORD flags, LPVOID param)
{
    (void)device; (void)mode; (void)wnd; (void)flags; (void)param;
    report_dialog("ChangeDisplaySettingsEx", "refused");
    return DISP_CHANGE_SUCCESSFUL;
}


static HRESULT WINAPI my_CreateDXGIFactory1(REFIID riid, void **out)
{
    (void)riid;
    if (out)
        *out = NULL;
    return E_NOINTERFACE;
}


static void *WINAPI my_AIL_open_digital_driver(unsigned rate, int bits, int channels, unsigned flags)
{
    (void)rate; (void)bits; (void)channels; (void)flags;
    return NULL;
}



static UINT g_clip_format;
static HGLOBAL g_clip_data;

static BOOL WINAPI my_OpenClipboard(HWND wnd) { (void)wnd; return TRUE; }
static BOOL WINAPI my_CloseClipboard(void) { return TRUE; }

static BOOL WINAPI my_EmptyClipboard(void)
{
    if (g_clip_data)
        GlobalFree(g_clip_data);
    g_clip_data = NULL;
    g_clip_format = 0;
    return TRUE;
}

static HANDLE WINAPI my_SetClipboardData(UINT format, HANDLE mem)
{
    my_EmptyClipboard();
    g_clip_format = format;
    g_clip_data = mem;
    return mem;
}

static HANDLE WINAPI my_GetClipboardData(UINT format)
{
    return format == g_clip_format ? g_clip_data : NULL;
}



typedef struct {
    char *path, *name;
    DWORD type, size;
    unsigned char *data;
} RegValue;

static RegValue *g_reg;
static int g_reg_count, g_reg_cap;
static char *g_reg_keys[64];

#define FAKE_HKEY_BASE 0x7FFE0000u


static int virtual_path(const char *p)
{
    return _strnicmp(p, "Software\\Gravity Soft", 21) == 0 || _strnicmp(p, "Software\\Gravity\\", 17) == 0;
}

static int dumped_path(const char *p) { return _strnicmp(p, "Software\\Gravity Soft", 21) == 0; }

static char *key_path(HKEY parent, const char *sub)
{
    DWORD i = (DWORD)(UINT_PTR)parent - FAKE_HKEY_BASE;
    const char *base = (DWORD)(UINT_PTR)parent >= FAKE_HKEY_BASE && i < 64 && g_reg_keys[i] ? g_reg_keys[i] : NULL;
    char *out;
    size_t n;
    if (!base) {
        if (!sub || !virtual_path(sub))
            return NULL;
        return _strdup(sub);
    }
    if (!sub || !*sub)
        return _strdup(base);
    n = strlen(base) + strlen(sub) + 2;
    out = malloc(n);
    _snprintf(out, n, "%s\\%s", base, sub);
    return out;
}

static HKEY open_fake(char *path)
{
    int i;
    for (i = 0; i < 64 && g_reg_keys[i]; i++)
        ;
    if (i == 64) {
        free(path);
        return NULL;
    }
    g_reg_keys[i] = path;
    return (HKEY)(UINT_PTR)(FAKE_HKEY_BASE + (DWORD)i);
}

static const char *fake_path(HKEY k)
{
    DWORD i = (DWORD)(UINT_PTR)k - FAKE_HKEY_BASE;
    return (DWORD)(UINT_PTR)k >= FAKE_HKEY_BASE && i < 64 ? g_reg_keys[i] : NULL;
}

static RegValue *reg_find(const char *path, const char *name)
{
    int i;
    for (i = 0; i < g_reg_count; i++)
        if (!_stricmp(g_reg[i].path, path) && !_stricmp(g_reg[i].name, name ? name : ""))
            return &g_reg[i];
    return NULL;
}

static void reg_set(const char *path, const char *name, DWORD type, const void *data, DWORD size)
{
    RegValue *v = reg_find(path, name);
    if (!v) {
        if (g_reg_count == g_reg_cap) {
            g_reg_cap = g_reg_cap ? g_reg_cap * 2 : 32;
            g_reg = realloc(g_reg, sizeof *g_reg * (size_t)g_reg_cap);
        }
        v = &g_reg[g_reg_count++];
        v->path = _strdup(path);
        v->name = _strdup(name ? name : "");
    } else {
        free(v->data);
    }
    v->type = type;
    v->size = size;
    v->data = malloc(size ? size : 1);
    memcpy(v->data, data, size);
}

static int hex_bytes(const char *text, unsigned char *out, int max)
{
    int n = 0;
    unsigned v;
    while (*text && n < max) {
        while (*text == ' ' || *text == '\t')
            text++;
        if (!*text || *text == '-' || *text == '#')
            break;
        if (sscanf(text, "%2x", &v) != 1)
            return -1;
        out[n++] = (unsigned char)v;
        text += 2;
    }
    return n;
}

static void load_patches(const char *file)
{
    char line[1024];
    int applied = 0, refused = 0;
    FILE *f = fopen(file, "r");
    if (!f) {
        plog("no patch file %s", file);
        return;
    }
    while (fgets(line, sizeof line, f)) {
        unsigned char was[64], now[64];
        char *arrow, *end;
        unsigned va;
        int n_was, n_now;
        DWORD old;
        if ((end = strpbrk(line, "\r\n")) != NULL)
            *end = 0;
        if (!*line || *line == '#')
            continue;
        arrow = strstr(line, "->");
        if (!arrow || sscanf(line, "%x", &va) != 1) {
            plog("patch line not understood: %s", line);
            refused++;
            continue;
        }
        n_was = hex_bytes(strchr(line, ' '), was, sizeof was);
        n_now = hex_bytes(arrow + 2, now, sizeof now);
        if (n_was <= 0 || n_was != n_now) {
            plog("patch line not understood: %s", line);
            refused++;
            continue;
        }
        if (memcmp((void *)(UINT_PTR)va, was, n_was) != 0) {
            plog("patch at %08x refused: the bytes there are not the ones it expects", va);
            refused++;
            continue;
        }
        VirtualProtect((void *)(UINT_PTR)va, n_was, PAGE_EXECUTE_READWRITE, &old);
        memcpy((void *)(UINT_PTR)va, now, n_was);
        VirtualProtect((void *)(UINT_PTR)va, n_was, old, &old);
        FlushInstructionCache(GetCurrentProcess(), (void *)(UINT_PTR)va, n_was);
        applied++;
    }
    fclose(f);
    plog("patches: %d applied, %d refused (%s)", applied, refused, file);
}

static void load_registry(const char *file)
{
    char line[1024];
    FILE *f = fopen(file, "r");
    if (!f) {
        plog("no registry file %s", file);
        return;
    }
    while (fgets(line, sizeof line, f)) {
        char *path = line, *name, *type, *val, *end;
        if ((end = strpbrk(line, "\r\n")) != NULL)
            *end = 0;
        if (!*line || *line == '#')
            continue;
        name = strchr(path, '|');
        type = name ? strchr(name + 1, '|') : NULL;
        val = type ? strchr(type + 1, '|') : NULL;
        if (!val)
            continue;
        *name++ = 0;
        *type++ = 0;
        *val++ = 0;
        if (!_stricmp(type, "dword")) {
            DWORD d = strtoul(val, NULL, 0);
            reg_set(path, name, REG_DWORD, &d, 4);
        } else if (!_strnicmp(type, "bin", 3)) {
            
            unsigned char data[512];
            DWORD n = 0;
            while (n < sizeof data && isxdigit((unsigned char)val[0]) && isxdigit((unsigned char)val[1])) {
                char byte[3] = { val[0], val[1], 0 };
                data[n++] = (unsigned char)strtoul(byte, NULL, 16);
                val += 2;
            }
            reg_set(path, name, strtoul(type + 3, NULL, 10), data, n);
        } else {
            reg_set(path, name, REG_SZ, val, (DWORD)strlen(val) + 1);
        }
    }
    fclose(f);
}

static void dump_registry(const char *path)
{
    int i;
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    for (i = 0; i < g_reg_count; i++) {
        RegValue *v = &g_reg[i];
        if (!dumped_path(v->path))
            continue;
        if (v->type == REG_DWORD && v->size == 4)
            fprintf(f, "%s|%s|dword|%lu\n", v->path, v->name, *(DWORD *)v->data);
        else if (v->type == REG_SZ || v->type == REG_EXPAND_SZ)
            fprintf(f, "%s|%s|sz|%.*s\n", v->path, v->name, (int)(v->size ? v->size - 1 : 0), v->data);
        else {
            DWORD k;
            fprintf(f, "%s|%s|bin%lu|", v->path, v->name, v->type);
            for (k = 0; k < v->size; k++)
                fprintf(f, "%02x", ((unsigned char *)v->data)[k]);
            fputc('\n', f);
        }
    }
    fclose(f);
}

static LSTATUS WINAPI my_RegOpenKeyExA(HKEY root, LPCSTR sub, DWORD opt, REGSAM sam, PHKEY out)
{
    char *p = key_path(root, sub);
    if (!p)
        return RegOpenKeyExA(root, sub, opt, sam, out);
    *out = open_fake(p);
    return *out ? ERROR_SUCCESS : ERROR_TOO_MANY_OPEN_FILES;
}

static LSTATUS WINAPI my_RegOpenKeyExW(HKEY root, LPCWSTR sub, DWORD opt, REGSAM sam, PHKEY out)
{
    char a[512];
    char *p;
    if (!WideCharToMultiByte(CP_ACP, 0, sub ? sub : L"", -1, a, sizeof a, NULL, NULL))
        return RegOpenKeyExW(root, sub, opt, sam, out);
    p = key_path(root, a);
    if (!p)
        return RegOpenKeyExW(root, sub, opt, sam, out);
    *out = open_fake(p);
    return *out ? ERROR_SUCCESS : ERROR_TOO_MANY_OPEN_FILES;
}

static LSTATUS WINAPI my_RegCreateKeyExA(HKEY root, LPCSTR sub, DWORD res, LPSTR cls, DWORD opt, REGSAM sam,
                                         const LPSECURITY_ATTRIBUTES sa, PHKEY out, LPDWORD disp)
{
    char *p = key_path(root, sub);
    if (!p)
        return RegCreateKeyExA(root, sub, res, cls, opt, sam, sa, out, disp);
    if (disp)
        *disp = REG_OPENED_EXISTING_KEY;
    *out = open_fake(p);
    return *out ? ERROR_SUCCESS : ERROR_TOO_MANY_OPEN_FILES;
}

static LSTATUS WINAPI my_RegCreateKeyA(HKEY root, LPCSTR sub, PHKEY out)
{
    return my_RegCreateKeyExA(root, sub, 0, NULL, 0, KEY_ALL_ACCESS, NULL, out, NULL);
}

static LSTATUS WINAPI my_RegCloseKey(HKEY k)
{
    DWORD i = (DWORD)(UINT_PTR)k - FAKE_HKEY_BASE;
    if (fake_path(k)) {
        free(g_reg_keys[i]);
        g_reg_keys[i] = NULL;
        return ERROR_SUCCESS;
    }
    return RegCloseKey(k);
}

static LSTATUS WINAPI my_RegQueryValueExA(HKEY k, LPCSTR name, LPDWORD res, LPDWORD type, LPBYTE data, LPDWORD size)
{
    const char *path = fake_path(k);
    RegValue *v;
    if (!path)
        return RegQueryValueExA(k, name, res, type, data, size);
    if ((v = reg_find(path, name)) == NULL) {
        plog("registry read of an unset value %s|%s", path, name ? name : "");
        return ERROR_FILE_NOT_FOUND;
    }
    if (type)
        *type = v->type;
    if (data && size && *size < v->size) {
        *size = v->size;
        return ERROR_MORE_DATA;
    }
    if (data)
        memcpy(data, v->data, v->size);
    if (size)
        *size = v->size;
    return ERROR_SUCCESS;
}

static LSTATUS WINAPI my_RegQueryValueExW(HKEY k, LPCWSTR name, LPDWORD res, LPDWORD type, LPBYTE data, LPDWORD size)
{
    const char *path = fake_path(k);
    char a[256];
    RegValue *v;
    DWORD need;
    if (!path)
        return RegQueryValueExW(k, name, res, type, data, size);
    WideCharToMultiByte(CP_ACP, 0, name ? name : L"", -1, a, sizeof a, NULL, NULL);
    if ((v = reg_find(path, a)) == NULL) {
        plog("registry read of an unset value %s|%s", path, a);
        return ERROR_FILE_NOT_FOUND;
    }
    if (type)
        *type = v->type;
    if (v->type == REG_SZ || v->type == REG_EXPAND_SZ) {
        int wn = MultiByteToWideChar(CP_ACP, 0, (char *)v->data, (int)v->size, NULL, 0);
        need = (DWORD)wn * 2;
        if (data && size && *size < need) {
            *size = need;
            return ERROR_MORE_DATA;
        }
        if (data)
            MultiByteToWideChar(CP_ACP, 0, (char *)v->data, (int)v->size, (WCHAR *)data, wn);
    } else {
        need = v->size;
        if (data && size && *size < need) {
            *size = need;
            return ERROR_MORE_DATA;
        }
        if (data)
            memcpy(data, v->data, need);
    }
    if (size)
        *size = need;
    return ERROR_SUCCESS;
}


static char g_reg_end[MAX_PATH];

static void reg_changed(void)
{
    if (g_reg_end[0])
        dump_registry(g_reg_end);
}

static LSTATUS WINAPI my_RegSetValueExA(HKEY k, LPCSTR name, DWORD res, DWORD type, const BYTE *data, DWORD size)
{
    const char *path = fake_path(k);
    if (!path)
        return RegSetValueExA(k, name, res, type, data, size);
    reg_set(path, name, type, data, size);
    reg_changed();
    return ERROR_SUCCESS;
}

static LSTATUS WINAPI my_RegDeleteValueA(HKEY k, LPCSTR name)
{
    const char *path = fake_path(k);
    RegValue *v;
    if (!path)
        return RegDeleteValueA(k, name);
    if ((v = reg_find(path, name)) == NULL)
        return ERROR_FILE_NOT_FOUND;
    free(v->path);
    free(v->name);
    free(v->data);
    *v = g_reg[--g_reg_count];
    reg_changed();
    return ERROR_SUCCESS;
}

static LSTATUS WINAPI my_RegEnumValueA(HKEY k, DWORD index, LPSTR name, LPDWORD name_len, LPDWORD res,
                                       LPDWORD type, LPBYTE data, LPDWORD size)
{
    const char *path = fake_path(k);
    int i;
    DWORD seen = 0;
    if (!path)
        return RegEnumValueA(k, index, name, name_len, res, type, data, size);
    for (i = 0; i < g_reg_count; i++) {
        RegValue *v = &g_reg[i];
        DWORD n;
        if (_stricmp(v->path, path) || seen++ != index)
            continue;
        n = (DWORD)strlen(v->name);
        if (*name_len <= n)
            return ERROR_MORE_DATA;
        memcpy(name, v->name, n + 1);
        *name_len = n;
        if (type)
            *type = v->type;
        if (data && size && *size < v->size)
            return ERROR_MORE_DATA;
        if (data)
            memcpy(data, v->data, v->size);
        if (size)
            *size = v->size;
        return ERROR_SUCCESS;
    }
    return ERROR_NO_MORE_ITEMS;
}




typedef struct {
    void **vtable;
    int slot;
    void *orig;
} VtPatch;

static VtPatch g_vt[64];
static int g_vt_count;

static void patch_vt(void *obj, int slot, void *hook)
{
    void **vt = *(void ***)obj;
    DWORD old;
    if (vt[slot] == hook)
        return;
    if (g_vt_count == (int)(sizeof g_vt / sizeof g_vt[0]))
        fatal("too many vtable patches");
    g_vt[g_vt_count].vtable = vt;
    g_vt[g_vt_count].slot = slot;
    g_vt[g_vt_count].orig = vt[slot];
    g_vt_count++;
    VirtualProtect(&vt[slot], sizeof vt[slot], PAGE_READWRITE, &old);
    vt[slot] = hook;
    VirtualProtect(&vt[slot], sizeof vt[slot], old, &old);
}

static void *vt_orig(void *obj, int slot)
{
    void **vt = *(void ***)obj;
    int i;
    for (i = 0; i < g_vt_count; i++)
        if (g_vt[i].vtable == vt && g_vt[i].slot == slot)
            return g_vt[i].orig;
    fatal("a hooked method without its original");
    return NULL;
}

enum {
    SLOT_D3D_CREATEDEVICE = 16,
    SLOT_D3DEX_CREATEDEVICEEX = 20,
    SLOT_DEV_RESET = 16,
    SLOT_DEV_PRESENT = 17,
    SLOT_DEVEX_RESETEX = 132,
    SLOT_DEV_CREATEQUERY = 118,
    SLOT_DEVEX_PRESENTEX = 121,
    SLOT_DEV_CREATETEXTURE = 23,
    SLOT_DEV_CREATEVERTEXBUFFER = 26,
    SLOT_DEV_SETRENDERSTATE = 57,
    SLOT_DEV_SETTEXTURE = 65,
    SLOT_DEV_SETTRANSFORM = 44,
    SLOT_DEV_UPDATETEXTURE = 31,
    SLOT_DEV_SETVIEWPORT = 47,
    SLOT_DEV_SETTEXTURESTAGESTATE = 67,
    SLOT_DEV_SETSAMPLERSTATE = 69,
    SLOT_DEV_SETFVF = 89,
    SLOT_DEV_SETVERTEXSHADER = 92,
    SLOT_DEV_SETSTREAMSOURCE = 100,
    SLOT_DEV_SETPIXELSHADER = 107,
    SLOT_DEV_DRAWPRIMITIVE = 81,
    SLOT_DEV_DRAWINDEXEDPRIMITIVE = 82,
    SLOT_DEV_DRAWPRIMITIVEUP = 83,
    SLOT_DEV_DRAWINDEXEDPRIMITIVEUP = 84,
    SLOT_QUERY_GETDATA = 7
};

static void write_bmp(const char *path, const unsigned char *bits, int pitch, int w, int h)
{
    BITMAPFILEHEADER fh = { 0 };
    BITMAPINFOHEADER ih = { 0 };
    FILE *f = fopen(path, "wb");
    int y;
    if (!f) {
        plog("cannot write %s", path);
        return;
    }
    fh.bfType = 0x4d42;
    fh.bfOffBits = sizeof fh + sizeof ih;
    fh.bfSize = fh.bfOffBits + (DWORD)(w * h * 4);
    ih.biSize = sizeof ih;
    ih.biWidth = w;
    ih.biHeight = -h;
    ih.biPlanes = 1;
    ih.biBitCount = 32;
    fwrite(&fh, sizeof fh, 1, f);
    fwrite(&ih, sizeof ih, 1, f);
    for (y = 0; y < h; y++)
        fwrite(bits + (size_t)y * (size_t)pitch, 4, (size_t)w, f);
    fclose(f);
}


static void capture_frame(IDirect3DDevice9 *dev, const char *path)
{
    IDirect3DSurface9 *bb = NULL, *rt = NULL, *mem = NULL, *src;
    D3DSURFACE_DESC d;
    D3DLOCKED_RECT lr;
    if (FAILED(IDirect3DDevice9_GetBackBuffer(dev, 0, 0, D3DBACKBUFFER_TYPE_MONO, &bb))) {
        plog("capture: no back buffer");
        return;
    }
    IDirect3DSurface9_GetDesc(bb, &d);
    src = bb;
    if (d.MultiSampleType != D3DMULTISAMPLE_NONE) {
        if (FAILED(IDirect3DDevice9_CreateRenderTarget(dev, d.Width, d.Height, d.Format, D3DMULTISAMPLE_NONE, 0,
                                                       FALSE, &rt, NULL)) ||
            FAILED(IDirect3DDevice9_StretchRect(dev, bb, NULL, rt, NULL, D3DTEXF_NONE))) {
            plog("capture: resolve failed");
            goto done;
        }
        src = rt;
    }
    if (d.Format != D3DFMT_X8R8G8B8 && d.Format != D3DFMT_A8R8G8B8) {
        plog("capture: back buffer format %d not supported", d.Format);
        goto done;
    }
    if (FAILED(IDirect3DDevice9_CreateOffscreenPlainSurface(dev, d.Width, d.Height, d.Format, D3DPOOL_SYSTEMMEM,
                                                            &mem, NULL)) ||
        FAILED(IDirect3DDevice9_GetRenderTargetData(dev, src, mem)) ||
        FAILED(IDirect3DSurface9_LockRect(mem, &lr, NULL, D3DLOCK_READONLY))) {
        plog("capture: read back failed");
        goto done;
    }
    write_bmp(path, lr.pBits, lr.Pitch, (int)d.Width, (int)d.Height);
    IDirect3DSurface9_UnlockRect(mem);
done:
    if (mem)
        IDirect3DSurface9_Release(mem);
    if (rt)
        IDirect3DSurface9_Release(rt);
    IDirect3DSurface9_Release(bb);
}


static void frame_sync(IDirect3DDevice9 *dev)
{
    thread_barrier();
    if (g_trace)
        plog("frame %lu heap %08lx", g_frame, g_heap_trace);
    EnterCriticalSection(&g_lock);
    g_cur_dev = dev;
    sync_point(PARITY_SYNC_FRAME);
    g_cur_dev = NULL;
    LeaveCriticalSection(&g_lock);
}

static void frame_advance(void)
{
    if (g_trace && g_intra > FRAME_MS)
        plog("frame %lu took %lu ms", g_frame, g_intra);
    g_frame_base += g_intra > FRAME_MS ? g_intra : FRAME_MS;
    g_intra = 0;
    g_spin_reads = 0;
    g_frame++;
}

static void force_windowed(D3DPRESENT_PARAMETERS *pp)
{
    if (!pp)
        return;
    pp->Windowed = TRUE;
    pp->FullScreen_RefreshRateInHz = 0;
    pp->PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
}

typedef HRESULT(STDMETHODCALLTYPE *Present_fn)(IDirect3DDevice9 *, const RECT *, const RECT *, HWND, const RGNDATA *);
typedef HRESULT(STDMETHODCALLTYPE *PresentEx_fn)(IDirect3DDevice9Ex *, const RECT *, const RECT *, HWND,
                                                  const RGNDATA *, DWORD);
typedef HRESULT(STDMETHODCALLTYPE *Reset_fn)(IDirect3DDevice9 *, D3DPRESENT_PARAMETERS *);
typedef HRESULT(STDMETHODCALLTYPE *ResetEx_fn)(IDirect3DDevice9Ex *, D3DPRESENT_PARAMETERS *, D3DDISPLAYMODEEX *);
typedef HRESULT(STDMETHODCALLTYPE *CreateQuery_fn)(IDirect3DDevice9 *, D3DQUERYTYPE, IDirect3DQuery9 **);
typedef HRESULT(STDMETHODCALLTYPE *GetData_fn)(IDirect3DQuery9 *, void *, DWORD, DWORD);
typedef HRESULT(STDMETHODCALLTYPE *CreateDevice_fn)(IDirect3D9 *, UINT, D3DDEVTYPE, HWND, DWORD,
                                                     D3DPRESENT_PARAMETERS *, IDirect3DDevice9 **);
typedef HRESULT(STDMETHODCALLTYPE *CreateDeviceEx_fn)(IDirect3D9Ex *, UINT, D3DDEVTYPE, HWND, DWORD,
                                                       D3DPRESENT_PARAMETERS *, D3DDISPLAYMODEEX *,
                                                       IDirect3DDevice9Ex **);
typedef IDirect3D9 *(WINAPI *Direct3DCreate9_fn)(UINT);
typedef HRESULT(WINAPI *Direct3DCreate9Ex_fn)(UINT, IDirect3D9Ex **);

static Direct3DCreate9_fn g_real_Direct3DCreate9;
static Direct3DCreate9Ex_fn g_real_Direct3DCreate9Ex;


static HRESULT seen(HRESULT hr)
{
    static int noted;
    if (hr != S_PRESENT_OCCLUDED)
        return hr;
    if (!noted) {
        noted = 1;
        plog("a present was occluded (first at frame %lu)", g_frame);
    }
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE my_Present(IDirect3DDevice9 *dev, const RECT *src, const RECT *dst, HWND wnd,
                                            const RGNDATA *dirty)
{
    Present_fn orig = (Present_fn)vt_orig(dev, SLOT_DEV_PRESENT);
    HRESULT hr;
    frame_sync(dev);
    hr = orig(dev, src, dst, wnd, dirty);
    frame_advance();
    return seen(hr);
}


static HRESULT STDMETHODCALLTYPE my_PresentEx(IDirect3DDevice9Ex *dev, const RECT *src, const RECT *dst, HWND wnd,
                                              const RGNDATA *dirty, DWORD flags)
{
    PresentEx_fn orig = (PresentEx_fn)vt_orig(dev, SLOT_DEVEX_PRESENTEX);
    HRESULT hr;
    frame_sync((IDirect3DDevice9 *)dev);
    hr = orig(dev, src, dst, wnd, dirty, flags & ~(DWORD)D3DPRESENT_DONOTWAIT);
    frame_advance();
    return seen(hr);
}


static HRESULT STDMETHODCALLTYPE my_GetData(IDirect3DQuery9 *q, void *data, DWORD size, DWORD flags)
{
    GetData_fn orig = (GetData_fn)vt_orig(q, SLOT_QUERY_GETDATA);
    switch (IDirect3DQuery9_GetType(q)) {
    case D3DQUERYTYPE_EVENT:
        if (data && size >= sizeof(BOOL))
            *(BOOL *)data = TRUE;
        return S_OK;
    case D3DQUERYTYPE_TIMESTAMP:
        if (data && size >= sizeof(UINT64))
            *(UINT64 *)data = (UINT64)clock_ms() * 1000u;
        return S_OK;
    case D3DQUERYTYPE_TIMESTAMPFREQ:
        if (data && size >= sizeof(UINT64))
            *(UINT64 *)data = 1000000u;
        return S_OK;
    case D3DQUERYTYPE_TIMESTAMPDISJOINT:
        if (data && size >= sizeof(BOOL))
            *(BOOL *)data = FALSE;
        return S_OK;
    default:
        return orig(q, data, size, flags);
    }
}

static HRESULT STDMETHODCALLTYPE my_CreateQuery(IDirect3DDevice9 *dev, D3DQUERYTYPE type, IDirect3DQuery9 **out)
{
    CreateQuery_fn orig = (CreateQuery_fn)vt_orig(dev, SLOT_DEV_CREATEQUERY);
    HRESULT hr = orig(dev, type, out);
    if (SUCCEEDED(hr) && out && *out)
        patch_vt(*out, SLOT_QUERY_GETDATA, (void *)my_GetData);
    return hr;
}


static HRESULT STDMETHODCALLTYPE my_Reset(IDirect3DDevice9 *dev, D3DPRESENT_PARAMETERS *pp)
{
    Reset_fn orig = (Reset_fn)vt_orig(dev, SLOT_DEV_RESET);
    force_windowed(pp);
    return orig(dev, pp);
}

static HRESULT STDMETHODCALLTYPE my_ResetEx(IDirect3DDevice9Ex *dev, D3DPRESENT_PARAMETERS *pp, D3DDISPLAYMODEEX *mode)
{
    ResetEx_fn orig = (ResetEx_fn)vt_orig(dev, SLOT_DEVEX_RESETEX);
    (void)mode;
    force_windowed(pp);
    return orig(dev, pp, NULL);
}


static DWORD g_dx_frame;
static DWORD g_dx_dumped;

static int dx_on(void)
{
    return g_dx_frame && g_frame == g_dx_frame && GetCurrentThreadId() == g_main_tid;
}

static void dx_dump_state(IDirect3DDevice9 *dev)
{
    DWORD i, s, v;
    if (g_dx_dumped == g_frame)
        return;
    g_dx_dumped = g_frame;
    plog("dx frame %lu begins", g_frame);
    for (i = 7; i <= 209; i++)
        if (SUCCEEDED(IDirect3DDevice9_GetRenderState(dev, (D3DRENDERSTATETYPE)i, &v)))
            plog("dx init rs %lu = 0x%lx", i, v);
    for (s = 0; s < 4; s++) {
        for (i = 1; i <= 32; i++)
            if (SUCCEEDED(IDirect3DDevice9_GetTextureStageState(dev, s, (D3DTEXTURESTAGESTATETYPE)i, &v)))
                plog("dx init tss %lu.%lu = 0x%lx", s, i, v);
        for (i = 1; i <= 13; i++)
            if (SUCCEEDED(IDirect3DDevice9_GetSamplerState(dev, s, (D3DSAMPLERSTATETYPE)i, &v)))
                plog("dx init ss %lu.%lu = 0x%lx", s, i, v);
    }
    if (SUCCEEDED(IDirect3DDevice9_GetFVF(dev, &v)))
        plog("dx init fvf 0x%lx", v);
}

static char g_dx_dir[MAX_PATH];


#define DX_TWINS 4096
static struct {
    void *gpu, *sys;
} g_dx_twin[DX_TWINS];
static int g_dx_twins;

static IDirect3DTexture9 *dx_twin_of(void *gpu)
{
    int i;
    for (i = 0; i < g_dx_twins; i++)
        if (g_dx_twin[i].gpu == gpu)
            return (IDirect3DTexture9 *)g_dx_twin[i].sys;
    return NULL;
}


static DWORD dx_texels(IDirect3DDevice9 *dev, IDirect3DTexture9 *t, const D3DSURFACE_DESC *d)
{
    IDirect3DSurface9 *src = NULL, *rt = NULL, *mem = NULL, *lockable;
    IDirect3DTexture9 *twin = d->Pool == D3DPOOL_DEFAULT ? dx_twin_of(t) : NULL;
    D3DLOCKED_RECT lr;
    DWORD hash = 2166136261u, row, x, bpp;
    char path[MAX_PATH];
    FILE *f;
    if (d->Format == D3DFMT_A8R8G8B8 || d->Format == D3DFMT_X8R8G8B8)
        bpp = 4;
    else if (d->Format == D3DFMT_A1R5G5B5 || d->Format == D3DFMT_X1R5G5B5 || d->Format == D3DFMT_R5G6B5 ||
             d->Format == D3DFMT_A4R4G4B4)
        bpp = 2;
    else
        return 0;
    if (twin)
        t = twin;
    if (FAILED(IDirect3DTexture9_GetSurfaceLevel(t, 0, &src)))
        return 0;
    lockable = src;
    if (d->Pool == D3DPOOL_DEFAULT && !twin) {
        if (FAILED(IDirect3DDevice9_CreateRenderTarget(dev, d->Width, d->Height, d->Format, D3DMULTISAMPLE_NONE, 0,
                                                       FALSE, &rt, NULL)) ||
            FAILED(IDirect3DDevice9_StretchRect(dev, src, NULL, rt, NULL, D3DTEXF_NONE)) ||
            FAILED(IDirect3DDevice9_CreateOffscreenPlainSurface(dev, d->Width, d->Height, d->Format, D3DPOOL_SYSTEMMEM,
                                                                &mem, NULL)) ||
            FAILED(IDirect3DDevice9_GetRenderTargetData(dev, rt, mem))) {
            hash = 0;
            goto done;
        }
        lockable = mem;
    }
    if (FAILED(IDirect3DSurface9_LockRect(lockable, &lr, NULL, D3DLOCK_READONLY))) {
        hash = 0;
        goto done;
    }
    for (row = 0; row < d->Height; row++)
        for (x = 0; x < d->Width * bpp; x++)
            hash = (hash ^ ((const unsigned char *)lr.pBits)[row * lr.Pitch + x]) * 16777619u;
    _snprintf(path, sizeof path, "%stex_%08lx.raw", g_dx_dir, hash);
    if ((f = fopen(path, "wb")) != NULL) {
        DWORD head[4] = { d->Width, d->Height, (DWORD)d->Format, d->Width * bpp };
        fwrite(head, sizeof head, 1, f);
        for (row = 0; row < d->Height; row++)
            fwrite((const unsigned char *)lr.pBits + row * lr.Pitch, d->Width * bpp, 1, f);
        fclose(f);
    }
    IDirect3DSurface9_UnlockRect(lockable);
done:
    if (mem)
        IDirect3DSurface9_Release(mem);
    if (rt)
        IDirect3DSurface9_Release(rt);
    IDirect3DSurface9_Release(src);
    return hash;
}

static DWORD g_dx_bound[2]; 

static void dx_texture(IDirect3DDevice9 *dev, DWORD stage, IDirect3DBaseTexture9 *t)
{
    D3DSURFACE_DESC d;
    DWORD hash = 0;
    if (!t)
        plog("dx tex %lu none", stage);
    else if (IDirect3DBaseTexture9_GetType(t) == D3DRTYPE_TEXTURE &&
             SUCCEEDED(IDirect3DTexture9_GetLevelDesc((IDirect3DTexture9 *)t, 0, &d))) {
        hash = dx_texels(dev, (IDirect3DTexture9 *)t, &d);
        plog("dx tex %lu %ux%u fmt %d levels %lu pool %d usage 0x%lx texels %08lx", stage, d.Width, d.Height,
             (int)d.Format, IDirect3DBaseTexture9_GetLevelCount(t), (int)d.Pool, d.Usage, hash);
    } else
        plog("dx tex %lu type %d", stage, (int)IDirect3DBaseTexture9_GetType(t));
    if (stage < 2)
        g_dx_bound[stage] = hash;
}


static void dx_tris(D3DPRIMITIVETYPE t, UINT prims, const void *idx, D3DFORMAT ifmt, const void *v, UINT stride)
{
    char path[MAX_PATH];
    FILE *f;
    UINT i;
    DWORD head[4];
    if (t != D3DPT_TRIANGLELIST || !v)
        return;
    _snprintf(path, sizeof path, "%sdx_tris.bin", g_dx_dir);
    if ((f = fopen(path, "ab")) == NULL)
        return;
    head[0] = g_dx_bound[0];
    head[1] = g_dx_bound[1];
    head[2] = stride;
    head[3] = prims;
    fwrite(head, sizeof head, 1, f);
    for (i = 0; i < prims * 3; i++) {
        UINT k = !idx ? i : ifmt == D3DFMT_INDEX32 ? ((const DWORD *)idx)[i] : ((const WORD *)idx)[i];
        fwrite((const char *)v + (size_t)k * stride, stride, 1, f);
    }
    fclose(f);
}

static void dx_draw(const char *what, D3DPRIMITIVETYPE t, UINT n, const void *v, UINT stride)
{
    char line[256];
    int at = 0;
    UINT i;
    at += _snprintf(line + at, sizeof line - at, "dx %s prim %d n %u stride %u", what, (int)t, n, stride);
    if (v)
        for (i = 0; i < stride / 4 && i < 10; i++)
            at += _snprintf(line + at, sizeof line - at, " %08lx", ((const DWORD *)v)[i]);
    plog("%s", line);
}


typedef HRESULT(STDMETHODCALLTYPE *DrawPrimitive_fn)(IDirect3DDevice9 *, D3DPRIMITIVETYPE, UINT, UINT);
typedef HRESULT(STDMETHODCALLTYPE *DrawIndexedPrimitive_fn)(IDirect3DDevice9 *, D3DPRIMITIVETYPE, INT, UINT, UINT,
                                                            UINT, UINT);
typedef HRESULT(STDMETHODCALLTYPE *DrawPrimitiveUP_fn)(IDirect3DDevice9 *, D3DPRIMITIVETYPE, UINT, const void *, UINT);
typedef HRESULT(STDMETHODCALLTYPE *DrawIndexedPrimitiveUP_fn)(IDirect3DDevice9 *, D3DPRIMITIVETYPE, UINT, UINT, UINT,
                                                              const void *, D3DFORMAT, const void *, UINT);
typedef HRESULT(STDMETHODCALLTYPE *CreateTexture_fn)(IDirect3DDevice9 *, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL,
                                                     IDirect3DTexture9 **, HANDLE *);
typedef HRESULT(STDMETHODCALLTYPE *CreateVertexBuffer_fn)(IDirect3DDevice9 *, UINT, DWORD, DWORD, D3DPOOL,
                                                          IDirect3DVertexBuffer9 **, HANDLE *);
typedef HRESULT(STDMETHODCALLTYPE *SetRenderState_fn)(IDirect3DDevice9 *, D3DRENDERSTATETYPE, DWORD);
typedef HRESULT(STDMETHODCALLTYPE *SetTexture_fn)(IDirect3DDevice9 *, DWORD, IDirect3DBaseTexture9 *);
typedef HRESULT(STDMETHODCALLTYPE *SetTransform_fn)(IDirect3DDevice9 *, D3DTRANSFORMSTATETYPE, const D3DMATRIX *);
typedef HRESULT(STDMETHODCALLTYPE *SetViewport_fn)(IDirect3DDevice9 *, const D3DVIEWPORT9 *);
typedef HRESULT(STDMETHODCALLTYPE *UpdateTexture_fn)(IDirect3DDevice9 *, IDirect3DBaseTexture9 *,
                                                     IDirect3DBaseTexture9 *);
typedef HRESULT(STDMETHODCALLTYPE *SetStageState_fn)(IDirect3DDevice9 *, DWORD, DWORD, DWORD);
typedef HRESULT(STDMETHODCALLTYPE *SetFVF_fn)(IDirect3DDevice9 *, DWORD);
typedef HRESULT(STDMETHODCALLTYPE *SetShader_fn)(IDirect3DDevice9 *, void *);
typedef HRESULT(STDMETHODCALLTYPE *SetStreamSource_fn)(IDirect3DDevice9 *, UINT, IDirect3DVertexBuffer9 *, UINT, UINT);

static HRESULT STDMETHODCALLTYPE my_DrawPrimitive(IDirect3DDevice9 *dev, D3DPRIMITIVETYPE t, UINT a, UINT b)
{
    InterlockedIncrement(&g_n_draws);
    if (dx_on()) {
        dx_dump_state(dev);
        plog("dx draw prim %d start %u n %u", (int)t, a, b);
    }
    return ((DrawPrimitive_fn)vt_orig(dev, SLOT_DEV_DRAWPRIMITIVE))(dev, t, a, b);
}

static HRESULT STDMETHODCALLTYPE my_DrawIndexedPrimitive(IDirect3DDevice9 *dev, D3DPRIMITIVETYPE t, INT a, UINT b,
                                                         UINT c, UINT d, UINT e)
{
    InterlockedIncrement(&g_n_draws);
    if (dx_on()) {
        dx_dump_state(dev);
        plog("dx drawi prim %d base %d min %u verts %u start %u n %u", (int)t, a, b, c, d, e);
    }
    return ((DrawIndexedPrimitive_fn)vt_orig(dev, SLOT_DEV_DRAWINDEXEDPRIMITIVE))(dev, t, a, b, c, d, e);
}

static HRESULT STDMETHODCALLTYPE my_DrawPrimitiveUP(IDirect3DDevice9 *dev, D3DPRIMITIVETYPE t, UINT n, const void *v,
                                                    UINT stride)
{
    InterlockedIncrement(&g_n_draws);
    if (dx_on()) {
        dx_dump_state(dev);
        dx_draw("drawup", t, n, v, stride);
        dx_tris(t, n, NULL, D3DFMT_UNKNOWN, v, stride);
    }
    return ((DrawPrimitiveUP_fn)vt_orig(dev, SLOT_DEV_DRAWPRIMITIVEUP))(dev, t, n, v, stride);
}

static HRESULT STDMETHODCALLTYPE my_DrawIndexedPrimitiveUP(IDirect3DDevice9 *dev, D3DPRIMITIVETYPE t, UINT a, UINT b,
                                                           UINT c, const void *i, D3DFORMAT f, const void *v,
                                                           UINT stride)
{
    InterlockedIncrement(&g_n_draws);
    if (dx_on()) {
        dx_dump_state(dev);
        plog("dx drawiup verts %u", b);
        dx_draw("drawiup", t, c, v, stride);
        dx_tris(t, c, i, f, v, stride);
    }
    return ((DrawIndexedPrimitiveUP_fn)vt_orig(dev, SLOT_DEV_DRAWINDEXEDPRIMITIVEUP))(dev, t, a, b, c, i, f, v, stride);
}

static HRESULT STDMETHODCALLTYPE my_CreateTexture(IDirect3DDevice9 *dev, UINT w, UINT h, UINT levels, DWORD usage,
                                                  D3DFORMAT f, D3DPOOL pool, IDirect3DTexture9 **out, HANDLE *share)
{
    InterlockedIncrement(&g_n_textures);
    return ((CreateTexture_fn)vt_orig(dev, SLOT_DEV_CREATETEXTURE))(dev, w, h, levels, usage, f, pool, out, share);
}

static HRESULT STDMETHODCALLTYPE my_CreateVertexBuffer(IDirect3DDevice9 *dev, UINT len, DWORD usage, DWORD fvf,
                                                       D3DPOOL pool, IDirect3DVertexBuffer9 **out, HANDLE *share)
{
    InterlockedIncrement(&g_n_vbuffers);
    return ((CreateVertexBuffer_fn)vt_orig(dev, SLOT_DEV_CREATEVERTEXBUFFER))(dev, len, usage, fvf, pool, out, share);
}

static HRESULT STDMETHODCALLTYPE my_SetRenderState(IDirect3DDevice9 *dev, D3DRENDERSTATETYPE st, DWORD v)
{
    InterlockedIncrement(&g_n_states);
    if (dx_on()) {
        dx_dump_state(dev);
        plog("dx rs %d = 0x%lx", (int)st, v);
    }
    return ((SetRenderState_fn)vt_orig(dev, SLOT_DEV_SETRENDERSTATE))(dev, st, v);
}

static HRESULT STDMETHODCALLTYPE my_SetTexture(IDirect3DDevice9 *dev, DWORD stage, IDirect3DBaseTexture9 *t)
{
    InterlockedIncrement(&g_n_set_textures);
    if (dx_on()) {
        dx_dump_state(dev);
        dx_texture(dev, stage, t);
    }
    return ((SetTexture_fn)vt_orig(dev, SLOT_DEV_SETTEXTURE))(dev, stage, t);
}


static HRESULT STDMETHODCALLTYPE my_UpdateTexture(IDirect3DDevice9 *dev, IDirect3DBaseTexture9 *sys,
                                                  IDirect3DBaseTexture9 *gpu)
{
    int i;
    EnterCriticalSection(&g_lock);
    for (i = 0; i < g_dx_twins && g_dx_twin[i].gpu != gpu; i++)
        ;
    if (i < DX_TWINS) {
        g_dx_twin[i].gpu = gpu;
        g_dx_twin[i].sys = sys;
        if (i == g_dx_twins)
            g_dx_twins++;
    }
    LeaveCriticalSection(&g_lock);
    return ((UpdateTexture_fn)vt_orig(dev, SLOT_DEV_UPDATETEXTURE))(dev, sys, gpu);
}

static HRESULT STDMETHODCALLTYPE my_SetTransform(IDirect3DDevice9 *dev, D3DTRANSFORMSTATETYPE st, const D3DMATRIX *m)
{
    if (dx_on() && m) {
        const float *f = &m->_11;
        dx_dump_state(dev);
        plog("dx xform %d %.6g %.6g %.6g %.6g | %.6g %.6g %.6g %.6g | %.6g %.6g %.6g %.6g | %.6g %.6g %.6g %.6g", (int)st,
             f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9], f[10], f[11], f[12], f[13], f[14], f[15]);
    }
    return ((SetTransform_fn)vt_orig(dev, SLOT_DEV_SETTRANSFORM))(dev, st, m);
}

static HRESULT STDMETHODCALLTYPE my_SetViewport(IDirect3DDevice9 *dev, const D3DVIEWPORT9 *vp)
{
    if (dx_on() && vp) {
        dx_dump_state(dev);
        plog("dx viewport %lu %lu %lu %lu %g %g", vp->X, vp->Y, vp->Width, vp->Height, vp->MinZ, vp->MaxZ);
    }
    return ((SetViewport_fn)vt_orig(dev, SLOT_DEV_SETVIEWPORT))(dev, vp);
}

static HRESULT STDMETHODCALLTYPE my_SetTextureStageState(IDirect3DDevice9 *dev, DWORD stage, DWORD st, DWORD v)
{
    if (dx_on()) {
        dx_dump_state(dev);
        plog("dx tss %lu.%lu = 0x%lx", stage, st, v);
    }
    return ((SetStageState_fn)vt_orig(dev, SLOT_DEV_SETTEXTURESTAGESTATE))(dev, stage, st, v);
}

static HRESULT STDMETHODCALLTYPE my_SetSamplerState(IDirect3DDevice9 *dev, DWORD stage, DWORD st, DWORD v)
{
    if (dx_on()) {
        dx_dump_state(dev);
        plog("dx ss %lu.%lu = 0x%lx", stage, st, v);
    }
    return ((SetStageState_fn)vt_orig(dev, SLOT_DEV_SETSAMPLERSTATE))(dev, stage, st, v);
}

static HRESULT STDMETHODCALLTYPE my_SetFVF(IDirect3DDevice9 *dev, DWORD fvf)
{
    if (dx_on()) {
        dx_dump_state(dev);
        plog("dx fvf 0x%lx", fvf);
    }
    return ((SetFVF_fn)vt_orig(dev, SLOT_DEV_SETFVF))(dev, fvf);
}

static HRESULT STDMETHODCALLTYPE my_SetVertexShader(IDirect3DDevice9 *dev, void *sh)
{
    if (dx_on()) {
        dx_dump_state(dev);
        plog("dx vs %s", sh ? "set" : "none");
    }
    return ((SetShader_fn)vt_orig(dev, SLOT_DEV_SETVERTEXSHADER))(dev, sh);
}

static HRESULT STDMETHODCALLTYPE my_SetPixelShader(IDirect3DDevice9 *dev, void *sh)
{
    if (dx_on()) {
        dx_dump_state(dev);
        plog("dx ps %s", sh ? "set" : "none");
    }
    return ((SetShader_fn)vt_orig(dev, SLOT_DEV_SETPIXELSHADER))(dev, sh);
}

static HRESULT STDMETHODCALLTYPE my_SetStreamSource(IDirect3DDevice9 *dev, UINT n, IDirect3DVertexBuffer9 *vb, UINT off,
                                                    UINT stride)
{
    if (dx_on()) {
        dx_dump_state(dev);
        plog("dx stream %u %s off %u stride %u", n, vb ? "vb" : "none", off, stride);
    }
    return ((SetStreamSource_fn)vt_orig(dev, SLOT_DEV_SETSTREAMSOURCE))(dev, n, vb, off, stride);
}

static void hook_device(IDirect3DDevice9 *dev, int ex)
{
    patch_vt(dev, SLOT_DEV_DRAWPRIMITIVE, (void *)my_DrawPrimitive);
    patch_vt(dev, SLOT_DEV_DRAWINDEXEDPRIMITIVE, (void *)my_DrawIndexedPrimitive);
    patch_vt(dev, SLOT_DEV_DRAWPRIMITIVEUP, (void *)my_DrawPrimitiveUP);
    patch_vt(dev, SLOT_DEV_DRAWINDEXEDPRIMITIVEUP, (void *)my_DrawIndexedPrimitiveUP);
    patch_vt(dev, SLOT_DEV_CREATETEXTURE, (void *)my_CreateTexture);
    patch_vt(dev, SLOT_DEV_CREATEVERTEXBUFFER, (void *)my_CreateVertexBuffer);
    patch_vt(dev, SLOT_DEV_SETRENDERSTATE, (void *)my_SetRenderState);
    patch_vt(dev, SLOT_DEV_SETTEXTURE, (void *)my_SetTexture);
    patch_vt(dev, SLOT_DEV_RESET, (void *)my_Reset);
    patch_vt(dev, SLOT_DEV_PRESENT, (void *)my_Present);
    patch_vt(dev, SLOT_DEV_CREATEQUERY, (void *)my_CreateQuery);
    if (g_dx_frame) {
        patch_vt(dev, SLOT_DEV_SETTRANSFORM, (void *)my_SetTransform);
        patch_vt(dev, SLOT_DEV_UPDATETEXTURE, (void *)my_UpdateTexture);
        patch_vt(dev, SLOT_DEV_SETVIEWPORT, (void *)my_SetViewport);
        patch_vt(dev, SLOT_DEV_SETTEXTURESTAGESTATE, (void *)my_SetTextureStageState);
        patch_vt(dev, SLOT_DEV_SETSAMPLERSTATE, (void *)my_SetSamplerState);
        patch_vt(dev, SLOT_DEV_SETFVF, (void *)my_SetFVF);
        patch_vt(dev, SLOT_DEV_SETVERTEXSHADER, (void *)my_SetVertexShader);
        patch_vt(dev, SLOT_DEV_SETPIXELSHADER, (void *)my_SetPixelShader);
        patch_vt(dev, SLOT_DEV_SETSTREAMSOURCE, (void *)my_SetStreamSource);
    }
    if (ex) {
        patch_vt(dev, SLOT_DEVEX_PRESENTEX, (void *)my_PresentEx);
        patch_vt(dev, SLOT_DEVEX_RESETEX, (void *)my_ResetEx);
    }
}

static void log_device(const char *what, HRESULT hr, const D3DPRESENT_PARAMETERS *pp)
{
    plog("%s -> 0x%08lx (%ux%u, multisample %d)", what, hr, pp ? pp->BackBufferWidth : 0,
         pp ? pp->BackBufferHeight : 0, pp ? (int)pp->MultiSampleType : -1);
}

static HRESULT STDMETHODCALLTYPE my_CreateDevice(IDirect3D9 *d3d, UINT adapter, D3DDEVTYPE type, HWND focus,
                                                 DWORD flags, D3DPRESENT_PARAMETERS *pp, IDirect3DDevice9 **out)
{
    CreateDevice_fn orig = (CreateDevice_fn)vt_orig(d3d, SLOT_D3D_CREATEDEVICE);
    HRESULT hr;
    force_windowed(pp);
    hr = orig(d3d, adapter, type, focus, flags, pp, out);
    log_device("CreateDevice", hr, pp);
    if (SUCCEEDED(hr))
        hook_device(*out, 0);
    return hr;
}

static HRESULT STDMETHODCALLTYPE my_CreateDeviceEx(IDirect3D9Ex *d3d, UINT adapter, D3DDEVTYPE type, HWND focus,
                                                   DWORD flags, D3DPRESENT_PARAMETERS *pp, D3DDISPLAYMODEEX *mode,
                                                   IDirect3DDevice9Ex **out)
{
    CreateDeviceEx_fn orig = (CreateDeviceEx_fn)vt_orig(d3d, SLOT_D3DEX_CREATEDEVICEEX);
    HRESULT hr;
    (void)mode;
    force_windowed(pp);
    hr = orig(d3d, adapter, type, focus, flags, pp, NULL, out);
    log_device("CreateDeviceEx", hr, pp);
    if (SUCCEEDED(hr))
        hook_device((IDirect3DDevice9 *)*out, 1);
    return hr;
}

static IDirect3D9 *WINAPI my_Direct3DCreate9(UINT sdk)
{
    IDirect3D9 *d3d = g_real_Direct3DCreate9(sdk);
    if (d3d)
        patch_vt(d3d, SLOT_D3D_CREATEDEVICE, (void *)my_CreateDevice);
    return d3d;
}

static HRESULT WINAPI my_Direct3DCreate9Ex(UINT sdk, IDirect3D9Ex **out)
{
    HRESULT hr = g_real_Direct3DCreate9Ex(sdk, out);
    if (SUCCEEDED(hr)) {
        patch_vt(*out, SLOT_D3D_CREATEDEVICE, (void *)my_CreateDevice);
        patch_vt(*out, SLOT_D3DEX_CREATEDEVICEEX, (void *)my_CreateDeviceEx);
    }
    return hr;
}



typedef struct {
    const char *dll, *name;
    void *hook;
    void *real;
} Hook;

static Hook g_hooks[] = {
    { "winmm.dll", "timeGetTime", (void *)my_timeGetTime },
    { "kernel32.dll", "GetTickCount", (void *)my_GetTickCount },
    { "kernel32.dll", "QueryPerformanceCounter", (void *)my_QueryPerformanceCounter },
    { "kernel32.dll", "QueryPerformanceFrequency", (void *)my_QueryPerformanceFrequency },
    { "kernel32.dll", "GetSystemTimeAsFileTime", (void *)my_GetSystemTimeAsFileTime },
    { "kernel32.dll", "GetSystemTime", (void *)my_GetSystemTime },
    { "kernel32.dll", "GetLocalTime", (void *)my_GetLocalTime },
    { "kernel32.dll", "FileTimeToLocalFileTime", (void *)my_FileTimeToLocalFileTime },
    { "kernel32.dll", "Sleep", (void *)my_Sleep },
    { "kernel32.dll", "CreateProcessA", (void *)my_CreateProcessA },
    { "kernel32.dll", "CreateThread", (void *)my_CreateThread },
    { "kernel32.dll", "K32GetProcessMemoryInfo", (void *)my_K32GetProcessMemoryInfo },
    { "kernel32.dll", "WaitForSingleObject", (void *)my_WaitForSingleObject },
    { "kernel32.dll", "WaitForSingleObjectEx", (void *)my_WaitForSingleObjectEx },
    { "kernel32.dll", "WaitForMultipleObjects", (void *)my_WaitForMultipleObjects },
    { "ucrtbase.dll", "_beginthreadex", (void *)my_beginthreadex },
    { "ucrtbase.dll", "malloc", (void *)my_malloc },
    { "ucrtbase.dll", "calloc", (void *)my_calloc },
    { "ucrtbase.dll", "realloc", (void *)my_realloc },
    { "ucrtbase.dll", "free", (void *)my_free },
    { "ucrtbase.dll", "_recalloc", (void *)my_recalloc },
    { "ucrtbase.dll", "_strdup", (void *)my_strdup },
    { "ucrtbase.dll", "_wcsdup", (void *)my_wcsdup },
    { "ucrtbase.dll", "_msize", (void *)my_msize },
    { "kernel32.dll", "HeapAlloc", (void *)my_HeapAlloc },
    { "kernel32.dll", "HeapReAlloc", (void *)my_HeapReAlloc },
    { "kernel32.dll", "GlobalAlloc", (void *)my_GlobalAlloc },
    { "ucrtbase.dll", "_time64", (void *)my_time64 },
    { "ucrtbase.dll", "_time32", (void *)my_time32 },
    { "ucrtbase.dll", "clock", (void *)my_clock },
    { "msvcp140.dll", "_Xtime_get_ticks", (void *)my_Xtime_get_ticks },
    { "ws2_32.dll", "socket", (void *)my_socket },
    { "ws2_32.dll", "connect", (void *)my_connect },
    { "ws2_32.dll", "send", (void *)my_send },
    { "ws2_32.dll", "WSASend", (void *)my_WSASend },
    { "ws2_32.dll", "recv", (void *)my_recv },
    { "ws2_32.dll", "select", (void *)my_select },
    { "ws2_32.dll", "ioctlsocket", (void *)my_ioctlsocket },
    { "ws2_32.dll", "closesocket", (void *)my_closesocket },
    { "ws2_32.dll", "setsockopt", (void *)my_setsockopt },
    { "user32.dll", "CreateWindowExA", (void *)my_CreateWindowExA },
    { "user32.dll", "ShowWindow", (void *)my_ShowWindow },
    { "user32.dll", "SetWindowPos", (void *)my_SetWindowPos },
    { "user32.dll", "MoveWindow", (void *)my_MoveWindow },
    { "user32.dll", "SetFocus", (void *)my_SetFocus },
    { "user32.dll", "SetForegroundWindow", (void *)my_SetForegroundWindow },
    { "user32.dll", "SetActiveWindow", (void *)my_SetActiveWindow },
    { "user32.dll", "GetForegroundWindow", (void *)my_GetForegroundWindow },
    { "user32.dll", "GetActiveWindow", (void *)my_GetActiveWindow },
    { "user32.dll", "SetCapture", (void *)my_SetCapture },
    { "user32.dll", "ReleaseCapture", (void *)my_ReleaseCapture },
    { "user32.dll", "ClipCursor", (void *)my_ClipCursor },
    { "user32.dll", "GetCursorPos", (void *)my_GetCursorPos },
    { "user32.dll", "SetCursorPos", (void *)my_SetCursorPos },
    { "user32.dll", "GetAsyncKeyState", (void *)my_GetAsyncKeyState },
    { "user32.dll", "GetKeyState", (void *)my_GetKeyState },
    { "user32.dll", "TranslateMessage", (void *)my_TranslateMessage },
    { "kernel32.dll", "CreateFileA", (void *)my_CreateFileA },
    { "kernel32.dll", "CreateFileW", (void *)my_CreateFileW },
    { "gdi32.dll", "TextOutA", (void *)my_TextOutA },
    { "gdi32.dll", "TextOutW", (void *)my_TextOutW },
    { "gdi32.dll", "ExtTextOutW", (void *)my_ExtTextOutW },
    { "user32.dll", "MessageBoxA", (void *)my_MessageBoxA },
    { "user32.dll", "DialogBoxParamA", (void *)my_DialogBoxParamA },
    { "user32.dll", "SystemParametersInfoA", (void *)my_SystemParametersInfoA },
    { "user32.dll", "SendMessageTimeoutA", (void *)my_SendMessageTimeoutA },
    { "user32.dll", "ChangeDisplaySettingsA", (void *)my_ChangeDisplaySettingsA },
    { "user32.dll", "ChangeDisplaySettingsExA", (void *)my_ChangeDisplaySettingsExA },
    { "user32.dll", "OpenClipboard", (void *)my_OpenClipboard },
    { "user32.dll", "CloseClipboard", (void *)my_CloseClipboard },
    { "user32.dll", "EmptyClipboard", (void *)my_EmptyClipboard },
    { "user32.dll", "SetClipboardData", (void *)my_SetClipboardData },
    { "user32.dll", "GetClipboardData", (void *)my_GetClipboardData },
    { "shell32.dll", "ShellExecuteA", (void *)my_ShellExecuteA },
    { "advapi32.dll", "RegOpenKeyExA", (void *)my_RegOpenKeyExA },
    { "advapi32.dll", "RegOpenKeyExW", (void *)my_RegOpenKeyExW },
    { "advapi32.dll", "RegCreateKeyA", (void *)my_RegCreateKeyA },
    { "advapi32.dll", "RegCreateKeyExA", (void *)my_RegCreateKeyExA },
    { "advapi32.dll", "RegCloseKey", (void *)my_RegCloseKey },
    { "advapi32.dll", "RegQueryValueExA", (void *)my_RegQueryValueExA },
    { "advapi32.dll", "RegQueryValueExW", (void *)my_RegQueryValueExW },
    { "advapi32.dll", "RegSetValueExA", (void *)my_RegSetValueExA },
    { "advapi32.dll", "RegDeleteValueA", (void *)my_RegDeleteValueA },
    { "advapi32.dll", "RegEnumValueA", (void *)my_RegEnumValueA },
    { "mss32.dll", "_AIL_open_digital_driver@16", (void *)my_AIL_open_digital_driver },
    { "granny2.dll", "_GrannyGetSystemSeconds@0", (void *)my_GrannyGetSystemSeconds },
    { "granny2.dll", "_GrannyGetSecondsElapsed@8", (void *)my_GrannyGetSecondsElapsed },
    { "d3d9.dll", "Direct3DCreate9", (void *)my_Direct3DCreate9 },
    { "d3d9.dll", "Direct3DCreate9Ex", (void *)my_Direct3DCreate9Ex },
};

#define HOOK_COUNT (sizeof g_hooks / sizeof g_hooks[0])


static int is_arena_hook(const void *hook)
{
    return hook == (void *)my_malloc || hook == (void *)my_calloc || hook == (void *)my_realloc ||
           hook == (void *)my_free || hook == (void *)my_recalloc || hook == (void *)my_strdup ||
           hook == (void *)my_wcsdup || hook == (void *)my_msize;
}


static FARPROC WINAPI my_GetProcAddress(HMODULE mod, LPCSTR name)
{
    FARPROC p = GetProcAddress(mod, name);
    size_t i;
    for (i = 0; p && i < HOOK_COUNT; i++)
        if (g_hooks[i].real == (void *)p)
            return (FARPROC)g_hooks[i].hook;
    if (p && !IS_INTRESOURCE(name) && !strcmp(name, "CreateDXGIFactory1"))
        return (FARPROC)my_CreateDXGIFactory1;
    if (p && !IS_INTRESOURCE(name) && !strcmp(name, "ChangeDisplaySettingsA"))
        return (FARPROC)my_ChangeDisplaySettingsA;
    if (p && !IS_INTRESOURCE(name) && !strcmp(name, "ChangeDisplaySettingsExA"))
        return (FARPROC)my_ChangeDisplaySettingsExA;
    return p;
}

static void patch_slot(void **slot, void *hook)
{
    DWORD old;
    VirtualProtect(slot, sizeof *slot, PAGE_READWRITE, &old);
    *slot = hook;
    VirtualProtect(slot, sizeof *slot, old, &old);
}


static int patch_imports(HMODULE mod)
{
    BYTE *base = (BYTE *)mod;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    IMAGE_IMPORT_DESCRIPTOR *d = (IMAGE_IMPORT_DESCRIPTOR *)(base + dir.VirtualAddress);
    void *real_gpa = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetProcAddress");
    int patched = 0;
    for (; d->Name; d++) {
        void **slot = (void **)(base + d->FirstThunk);
        for (; *slot; slot++) {
            size_t i;
            if (*slot == real_gpa) {
                patch_slot(slot, (void *)my_GetProcAddress);
                patched++;
                continue;
            }
            for (i = 0; i < HOOK_COUNT; i++) {
                if (g_hooks[i].real && *slot == g_hooks[i].real) {
                    patch_slot(slot, g_hooks[i].hook);
                    patched++;
                    break;
                }
            }
        }
    }
    return patched;
}




__declspec(dllexport) void parity_register_probe(void (*dump)(const char *path))
{
    g_probe = dump;
}


#pragma comment(linker, "/EXPORT:parity_init=_parity_init@4")
DWORD WINAPI parity_init(LPVOID main_tid)
{
    char buf[MAX_PATH * 2];
    DWORD body[2];
    size_t i;
    int patched;
    g_main_tid = (DWORD)(UINT_PTR)main_tid;
    SetEnvironmentVariableA("TZ", CRT_TIME_ZONE);
    InitializeCriticalSection(&g_lock);
    g_tls = TlsAlloc();
    {
        HMODULE ucrt = GetModuleHandleA("ucrtbase.dll");
        g_real_calloc = (calloc_fn)(void *)GetProcAddress(ucrt, "calloc");
        g_real_msize = (msize_fn)(void *)GetProcAddress(ucrt, "_msize");
    }
    if (GetEnvironmentVariableA("PARITY_LOG", buf, sizeof buf)) {
        char *slash;
        g_log = fopen(buf, "w");
        strcpy(g_dx_dir, buf);
        slash = strrchr(g_dx_dir, '\\');
        if (slash)
            slash[1] = 0;
        else
            g_dx_dir[0] = 0;
    }
    g_asan = GetModuleHandleA("clang_rt.asan_dynamic-i386.dll") != NULL;
    arena_init();
    if (!GetEnvironmentVariableA("PARITY_PIPE", buf, sizeof buf))
        fatal("PARITY_PIPE is not set");
    g_pipe = CreateFileA(buf, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (g_pipe == INVALID_HANDLE_VALUE)
        fatal("cannot open the runner's pipe");
    g_trace = GetEnvironmentVariableA("PARITY_TRACE", buf, sizeof buf) > 0;
    if (GetEnvironmentVariableA("PARITY_RECV_SPLIT", buf, sizeof buf)) {
        char *p = buf;
        g_split = 1;
        g_split_seed = strtoul(p, &p, 10);
        if (*p == ':')
            g_split_chunk = strtoul(p + 1, &p, 10);
        if (*p == ':')
            g_split_frame = strtoul(p + 1, &p, 10);
        if (!g_split_chunk)
            g_split_chunk = 1;
        plog("recv split: seed %lu, recv cuts up to %lu bytes apart, frame cuts up to %lu", g_split_seed,
             g_split_chunk, g_split_frame);
    }
    if (GetEnvironmentVariableA("PARITY_TRACE_FRAME", buf, sizeof buf))
        g_dx_frame = strtoul(buf, NULL, 10);
    
    g_probe = (void (*)(const char *))(void *)GetProcAddress(GetModuleHandleA(NULL), "parity_probe");
    g_call = (int (__cdecl *)(const DWORD *, DWORD))(void *)GetProcAddress(GetModuleHandleA(NULL), "parity_call");
    if (GetEnvironmentVariableA("PARITY_PATCHES", buf, sizeof buf))
        load_patches(buf);
    if (GetEnvironmentVariableA("PARITY_REG", buf, sizeof buf))
        load_registry(buf);
    if (GetEnvironmentVariableA("PARITY_REG_END", g_reg_end, sizeof g_reg_end))
        reg_changed();
    for (i = 0; i < HOOK_COUNT; i++) {
        HMODULE m = GetModuleHandleA(g_hooks[i].dll);
        g_hooks[i].real = m && !(g_asan && is_arena_hook(g_hooks[i].hook)) ? (void *)GetProcAddress(m, g_hooks[i].name) : NULL;
    }
    for (i = 0; i < HOOK_COUNT; i++) {
        if (g_hooks[i].hook == (void *)my_Direct3DCreate9)
            g_real_Direct3DCreate9 = (Direct3DCreate9_fn)g_hooks[i].real;
        else if (g_hooks[i].hook == (void *)my_Direct3DCreate9Ex)
            g_real_Direct3DCreate9Ex = (Direct3DCreate9Ex_fn)g_hooks[i].real;
        else if (g_hooks[i].hook == (void *)my_beginthreadex)
            g_real_beginthreadex = (beginthreadex_fn)g_hooks[i].real;
        else if (g_hooks[i].hook == (void *)my_realloc)
            g_real_realloc = (realloc_fn)g_hooks[i].real;
        else if (g_hooks[i].hook == (void *)my_free)
            g_real_free = (free_fn)g_hooks[i].real;
    }
    patched = patch_imports(GetModuleHandleA(NULL));
    plog("parity_init: main thread %lu, %d import slots patched", g_main_tid, patched);
    if (g_asan)
        plog("parity_init: an AddressSanitizer client; its CRT allocations stay with ASan, no arena");
    GetModuleFileNameA(NULL, buf, sizeof buf);
    body[0] = GetCurrentProcessId();
    body[1] = g_main_tid;
    send_msg(PARITY_HELLO, body, sizeof body, buf, (DWORD)strlen(buf));
    return 1;
}


BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    DWORD done;
    (void)inst; (void)reserved;
    if (reason == DLL_PROCESS_DETACH && g_pipe != INVALID_HANDLE_VALUE && g_events_len)
        WriteFile(g_pipe, g_events, g_events_len, &done, NULL);
    return TRUE;
}
