#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "../src/login/otp.c"

static struct LoginMode mode, before;
static int phase, last_size, size_queries, sends, trace_mode;
static BYTE last_packet[14], expected[14];
static char send_result = 1;
static struct RagConnection *const connection = (struct RagConnection *)1;
static struct UIWindow *const wait_window = (struct UIWindow *)2;
static char wait_text[] = "Please wait";

struct RagConnection *rag_connection_instance(void) { return connection; }
int rag_connection_get_packet_length(struct RagConnection *self, int id)
{
    assert(self == connection && id == 0xad0 && phase == 0);
    ++size_queries;
    return 11;
}
char rag_connection_send_packet(struct RagConnection *self, int size, const void *packet)
{
    assert(self == connection && phase++ == 0 && (size == 11 || size == 14));
    ++sends;
    last_size = size;
    memcpy(last_packet, packet, size);
    return send_result;
}
struct UIWindow *ui_window_open(int id)
{
    assert(id == 6 && phase++ == 1);
    return wait_window;
}
char *msg_str(int id)
{
    assert(id == 0x7a && phase++ == 2 && mode.waitWnd == wait_window);
    return wait_text;
}
void ui_wait_wnd_set_text(struct UIWindow *window, const char *text)
{
    assert(phase++ == 3 && window == wait_window && text == wait_text);
}

static void send_both(void)
{
    DWORD value = (DWORD)mode.mobile_otp_id;
    size_t i;
    int query_count = size_queries;
    before = mode;
    memset(expected, 0, sizeof(expected));
    expected[0] = 0xa3; expected[1] = 9; expected[2] = 14;
    expected[4] = (BYTE)value;
    expected[5] = (BYTE)(value / 256);
    expected[6] = (BYTE)(value / 65536);
    expected[7] = (BYTE)(value / 16777216);
    for (i = 0; i < 6 && mode.otp_code[i]; ++i)
        expected[8 + i] = mode.otp_code[i];
    phase = 0;
    assert(login_send_mobile_otp(&mode) == 0 && phase == 4 && last_size == 14 && size_queries == query_count);
    assert(!memcmp(last_packet, expected, 14));
    if (trace_mode) {
        for (i = 0; i < 14; ++i) printf("%02x", last_packet[i]);
        putchar(' ');
    }
    before.waitWnd = wait_window;
    assert(!memcmp(&before, &mode, sizeof(mode)));
    phase = 0;
    assert(login_send_otp(&mode) == 0 && phase == 4 && last_size == 11 && size_queries == query_count + 1);
    assert(last_packet[0] == 0xd0 && last_packet[1] == 10 && !memcmp(last_packet + 2, mode.otp_code, 9));
    assert(!memcmp(&before, &mode, sizeof(mode)));
    if (trace_mode) {
        for (i = 0; i < 11; ++i) printf("%02x", last_packet[i]);
        putchar('\n');
    }
}

static void check_set(const char *input)
{
    BYTE text[12] = {0};
    size_t i;
    before = mode;
    for (i = 0; i < 11 && input && input[i]; ++i)
        text[i] = (BYTE)input[i];
    login_set_otp_code(&mode, input);
    memcpy(before.otp_code, text, sizeof(text));
    assert(!memcmp(&before, &mode, sizeof(mode)));
}

static void bounded_inputs(void)
{
    SYSTEM_INFO info;
    BYTE *pages, *end;
    DWORD old;
    size_t length;
    GetSystemInfo(&info);
    pages = VirtualAlloc(NULL, 2 * info.dwPageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(pages);
    end = pages + info.dwPageSize;
    assert(VirtualProtect(end, info.dwPageSize, PAGE_NOACCESS, &old));
    for (length = 1; length <= 11; ++length) {
        memset(end - length, '3', length);
        end[-1] = 0;
        check_set((const char *)end - length);
        send_both();
    }
    memset(end - 11, '7', 11);
    check_set((const char *)end - 11);
    assert(mode.otp_code[10] == '7' && mode.otp_code[11] == 0);
    send_both();
    assert(VirtualFree(pages, 0, MEM_RELEASE));
}

int main(int argc, char **argv)
{
    unsigned random = 0x9a30ad0;
    int i, j;
    trace_mode = argc == 2 && !strcmp(argv[1], "--trace");
    memset(&mode, 0xa5, sizeof(mode));
    if (trace_mode) {
        for (i = 0; i < 2000; ++i) {
            memset(mode.otp_code, 0, sizeof(mode.otp_code));
            for (j = 0; j < (i % 2 ? 8 : 6); ++j) {
                random = random * 1664525u + 1013904223u;
                mode.otp_code[j] = '0' + (BYTE)(random % 10);
            }
            mode.mobile_otp_id = (int)random;
            send_both();
        }
        return 0;
    }
    check_set("12345678"); send_both();
    check_set("123"); send_both();
    assert(!memcmp(mode.otp_code, "123\0\0\0\0\0\0\0\0", 12));
    check_set(""); send_both();
    check_set(NULL); send_both();
    check_set("%s%n%%"); send_both();
    check_set("한국123"); send_both();
    check_set("123456789012345678901234"); send_both();
    assert(!memcmp(mode.otp_code, "12345678901", 12));
    check_set((const char *)mode.otp_code); send_both();
    check_set((const char *)mode.otp_code + 2); send_both();
    memset(mode.otp_code, '8', sizeof(mode.otp_code));
    mode.mobile_otp_id = -1; send_both();
    mode.mobile_otp_id = (int)0x80000000u; send_both();
    mode.mobile_otp_id = 0; send_both();
    bounded_inputs();
    for (i = 0; i < 5000; ++i) {
        char code[25];
        int length = i % 24;
        for (j = 0; j < length; ++j) {
            random = random * 1664525u + 1013904223u;
            code[j] = (char)(1 + random % 255);
        }
        code[length] = 0;
        mode.mobile_otp_id = (int)random;
        send_result = (char)(i % 2);
        check_set(code); send_both();
    }
    assert(sends > 10000);
    puts("Login OTP: exact packet bytes, literal bounded input, aliasing, zero padding, protected pages, wait ordering and 5000 generated cases passed.");
    return 0;
}
