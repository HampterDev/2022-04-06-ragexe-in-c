#include <windows.h>
#include <shellapi.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Run both production <URL> link handlers with the shell stubbed. Link targets
   come from server text, so only absolute http:// and https:// URLs may reach
   ShellExecuteA; files, UNC paths and other protocol handlers must not. */
static HINSTANCE test_shell_open(HWND window, const char *operation, const char *file,
                                 const char *parameters, const char *directory, int show);
#define ShellExecuteA test_shell_open
#include "url_link_helper.inc"
#include "url_link_handlers.inc"
/* The links open through the platform's desktop call, which is ShellExecuteA on Windows. */
#include "../src/platform/desktop_win32.c"

struct UIHooks g_uiHooks;
#undef ShellExecuteA

HWND g_hMainWnd = (HWND)0x1234;
static char opened[128];
static int opens, cases;

static HINSTANCE test_shell_open(HWND window, const char *operation, const char *file,
                                 const char *parameters, const char *directory, int show)
{
    assert(window == g_hMainWnd && !strcmp(operation, "open"));
    assert(!parameters && !directory && show == SW_SHOWNORMAL);
    assert(strlen(file) < sizeof opened);
    strcpy(opened, file);
    ++opens;
    return (HINSTANCE)33;
}

/* Text-button lines, as the production button stores them. */
struct TextView ui_text_button_get_line(const struct UITextButton *self, size_t i)
{
    return text_array_at(&self->lines, i);
}

size_t ui_text_button_line_count(const struct UITextButton *self)
{
    return self->lines.size;
}

static void check(struct TextView target, const char *expected)
{
    int handler;

    for (handler = 0; handler < 2; ++handler) {
        struct UITextButton link;
        int before = opens;

        memset(&link, 0, sizeof link);
        assert(text_array_append(&link.lines, target));
        opened[0] = 0;
        if (handler)
            ui_tag_text_viewer_on_url_link(NULL, &link);
        else
            on_link(&link, LINK_URL);
        if (expected) {
            assert(opens == before + 1 && !strcmp(opened, expected));
        } else {
            assert(opens == before);
        }
        text_array_clear(&link.lines);
        ++cases;
    }
}

static void opens_as(const char *target, const char *expected) { check(text_view_cstr(target), expected); }
static void stays_closed(const char *target) { check(text_view_cstr(target), NULL); }

int main(void)
{
    static const char embedded[] = "http://\0x";
    static const char slice[] = "http://xC:\\Windows\\System32\\cmd.exe";
    struct UITextButton empty;

    opens_as("http://x", "http://x");
    opens_as("HTTPS://example.com/path?q", "HTTPS://example.com/path?q");
    opens_as("hTtP://example.com", "hTtP://example.com");
    stays_closed("file:///C:/x");
    stays_closed("C:\\Windows\\System32\\cmd.exe");
    stays_closed("\\\\host\\share\\a.exe");
    stays_closed("ms-msdt:/id");
    stays_closed("search-ms:query=x");
    stays_closed("javascript:alert(1)");
    stays_closed("http:/x");
    stays_closed("http://");
    stays_closed("https://");
    stays_closed("");
    stays_closed(" http://x");
    stays_closed("xhttp://x");
    check(text_view(embedded, sizeof embedded - 1), NULL);
    /* A line taken from part of a longer text is stored with its own NUL. */
    check(text_view(slice, 8), "http://x");

    memset(&empty, 0, sizeof empty);
    ui_tag_text_viewer_on_url_link(NULL, &empty);
    on_link(&empty, LINK_URL);
    on_link(NULL, LINK_URL);
    assert(opens == 8);
    printf("URL links: %d handler cases passed (only http:// and https:// targets open).\n", cases);
    return 0;
}
