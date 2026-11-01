#include "core/text_markup.h"
#include "core/markup_tags.h"
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"

#define NPOS ((size_t)-1)

static size_t shown_length(const struct String *s)
{
    struct String copy;
    size_t n;

    string_copy(&copy, s);
    n = copy.size ? text_strip_info(copy.data, copy.size) : 0;
    string_free(&copy);
    return n;
}

void ui_window_parse_tag_links(struct UIWindow *self, struct String text,
                                       struct String open, struct String close, struct Vec *out)
{
    struct String s, prefix;
    size_t pos, base;

    string_copy(&s, &text);
    pos = text_view_find(string_view(&s), string_view(&open), 0);
    if (pos != NPOS) {
        string_init(&prefix);
        string_substring(&prefix, &s, 0, pos);
        base = shown_length(&prefix);
        string_free(&prefix);
        do {
            struct String rest;
            size_t cpos;

            pos += open.size;
            string_init(&rest);
            string_substring(&rest, &s, pos, NPOS);
            cpos = text_view_find(string_view(&rest), string_view(&close), 0);
            if (cpos != NPOS) {
                struct String inner, args;
                struct UiTagLink link, copy;
                size_t ipos;

                string_init(&inner);
                string_substring(&inner, &rest, 0, cpos);
                ipos = text_view_find(string_view(&inner), string_view(&text_tag_INFO), 0);
                string_init(&args);
                if (ipos != NPOS) {
                    size_t epos;

                    string_free(&inner);
                    string_substring(&inner, &rest, 0, ipos);
                    string_free(&args);
                    string_substring(&args, &rest, ipos + text_tag_INFO.size, NPOS);
                    epos = text_view_find(string_view(&args), string_view(&text_tag_INFO_END), 0);
                    if (epos != NPOS) {
                        struct String t;

                        string_init(&t);
                        string_substring(&t, &args, 0, epos);
                        string_free(&args);
                        args = t;
                    }
                }
                link.pos = (int)base;
                link.x = 0;
                link.y = 0;
                string_copy(&link.label, &inner);
                string_copy(&link.args, &args);
                ui_tag_link_copy(&copy, &link);
                vec_push(out, &copy, sizeof copy);
                base += inner.size;
                string_substring(&rest, &rest, close.size + cpos, STRING_NPOS);
                string_free(&s);
                s = rest;
                string_init(&rest);
                pos = text_view_find(string_view(&s), string_view(&open), 0);
                if (pos != NPOS) {
                    string_substring(&prefix, &s, 0, pos);
                    base += shown_length(&prefix);
                    string_free(&prefix);
                }
                string_free(&link.args);
                string_free(&link.label);
                string_free(&args);
                string_free(&inner);
            }
            string_free(&rest);
        } while (pos != NPOS);
    }
    string_free(&s);
    string_free(&text);
    string_free(&open);
    string_free(&close);
}

static void parse_pair(struct UIWindow *self, struct String text, const struct String *open,
                       const struct String *close, struct Vec *out)
{
    struct String a_open, a_close;

    string_copy(&a_close, close);
    string_copy(&a_open, open);
    ui_window_parse_tag_links(self, text, a_open, a_close, out);
}

void ui_window_parse_navi_links(struct UIWindow *self, struct String text,
                                        struct Vec *out)
{
    parse_pair(self, text, &text_tag_NAVI, &text_tag_NAVI_END, out);
}

void ui_window_parse_url_links(struct UIWindow *self, struct String text,
                                       struct Vec *out)
{
    parse_pair(self, text, &text_tag_URL, &text_tag_URL_END, out);
}
