#include "contents/sound.h"
#include "ui/window_manager_state.h"
#include "world/mode_dispatch.h"
#include "game/context.h"
#include "session/csession.h"
#include "session/session_names.h"
#include "session/session_talk.h"

#include <stdlib.h>
#include <string.h>
#include "contents/repute_wnd.h"
#include "ui/ui_std.h"
#include "ui/ui_items.h"
#include "ui/wndmgr/wndmgr.h"

static int tokens_count(const struct Vec *tokens)
{
    return (int)(((BYTE *)tokens->last - (BYTE *)tokens->first) / (int)sizeof(struct String));
}

struct TalkPrefix {
    const char *text;
    int len;
    int type;
};

static const struct TalkPrefix s_prefixes[] = {
    { "/ㅠ", 3, 0x1 },
    { "/b", 2, 0x1 },
    { "/lb", 3, 0x57 },
    { "/ㅣㅠ", 5, 0x57 },
    { "/nb", 3, 0x5a },
    { "/파란말", 7, 0x64 },
    { "/ㅜㅠ", 5, 0x5a },
    { "/nlb", 4, 0x5b },
    { "/ㅟㅠ", 5, 0x5b },
    { "/bb", 3, 0x64 },
    { "/지역파란말", 0xb, 0x65 },
    { "/mb", 3, 0x65 },
    { "/kill", 5, 0x4 },
};

struct StatCommand {
    const char *text;
    int msg;
    int stat;
};

static const struct StatCommand s_statCommands[] = {
    { "/str+", 0x1e, 0xd },
    { "/agi+", 0x1e, 0xe },
    { "/luk+", 0x1e, 0x12 },
    { "/int+", 0x1e, 0x10 },
    { "/vit+", 0x1e, 0xf },
    { "/dex+", 0x1e, 0x11 },
    { "/pow+", 0x13a, 0xdb },
    { "/sta+", 0x13a, 0xdc },
    { "/wis+", 0x13a, 0xdd },
    { "/spl+", 0x13a, 0xde },
    { "/con+", 0x13a, 0xdf },
    { "/crt+", 0x13a, 0xe0 },
};

#define TOK(i)  tokens_at(tokens, (i))
#define COUNT() tokens_count(tokens)

static void assign_token(struct String *out, struct Vec *tokens, int i)
{
    const struct String *token = TOK(i);

    string_assign(out, string_cstr(token), token->size);
}

static void append_token(struct String *out, struct Vec *tokens, int i)
{
    const struct String *token = TOK(i);

    string_append(out, string_cstr(token), token->size);
}

static int skip_blanks(const char *text, int at)
{
    while (text[at] == ' ')
        at++;
    return at;
}

static int first_arg(struct String *out, struct Vec *tokens)
{
    assign_token(out, tokens, 1);
    return -1;
}

static int arg_3(struct String *out, struct Vec *tokens, int at)
{
    assign_token(&out[2], tokens, 3);
    return at;
}

static int args_2_3(struct String *out, struct Vec *tokens, int at)
{
    assign_token(&out[1], tokens, 2);
    return arg_3(out, tokens, at);
}

static int invited(struct String *out, struct String *joined, size_t pos)
{
    string_erase(joined, 0, pos + 1);
    string_assign(out, string_cstr(joined), joined->size);
    string_free(joined);
    return -1;
}

static int talk_type_scan(struct Session *self, const char *text, int *type, struct String *out,
                          struct Vec *tokens)
{
    struct String joined;
    const struct String *t0;
    int at = 0;
    int i, n;
    size_t k, pos;

    if (COUNT() == 0) {
        *type = 3;
        return -1;
    }
    text_to_lower(TOK(0));
    if (text[0] != '/') {
        *type = 0;
        return at;
    }
    t0 = TOK(0);

    for (k = 0; k < sizeof s_prefixes / sizeof s_prefixes[0]; k++) {
        if (string_equal_cstr(t0, s_prefixes[k].text)) {
            *type = s_prefixes[k].type;
            at = s_prefixes[k].len;
            return skip_blanks(text, at);
        }
    }

    if (string_equal_cstr(t0, "/v") || string_equal_cstr(t0, "/ㅍ")) {
        char number[0x14];

        *type = 0xc;
        if ((BYTE *)tokens->last - (BYTE *)tokens->first >= 0x30)
            return first_arg(out, tokens);
        _itoa(sound_effect_volume(), number, 10);
        string_assign_cstr(out, number);
        return -1;
    }
    if (string_equal_cstr(t0, "/bv") || string_equal_cstr(t0, "/ㅠㅍ")) {
        char number[0x14];

        *type = 0xd;
        if ((BYTE *)tokens->last - (BYTE *)tokens->first >= 0x30)
            return first_arg(out, tokens);
        _itoa(sound_bgm_volume(), number, 10);
        string_assign_cstr(out, number);
        return -1;
    }
    if ((string_equal_cstr(t0, "/gc") || string_equal_cstr(t0, "/guildchat")) && COUNT() >= 2) {
        *type = 0x39;
        string_assign_cstr(out, string_equal_cstr(TOK(0), "/gc") ? text + 4 : text + 0xb);
        return -1;
    }
    if ((string_equal_cstr(t0, "/ex") || string_equal_cstr(t0, "/차단")) && COUNT() >= 2) {
        *type = 0x12;
        string_assign_cstr(out, string_equal_cstr(TOK(0), "/ex") ? text + 4 : text + 6);
        return -1;
    }
    if ((string_equal_cstr(t0, "/in") || string_equal_cstr(t0, "/해제")) && COUNT() >= 2) {
        *type = 0x13;
        string_assign_cstr(out, string_equal_cstr(TOK(0), "/in") ? text + 4 : text + 6);
        return -1;
    }
    if (string_equal_cstr(t0, "/red")) {
        *type = 0x14;
        at = -1;
        for (i = 1; i < COUNT(); i++) {
            append_token(out, tokens, i);
            string_append_cstr(out, " ");
        }
        return at;
    }
    if ((string_equal_cstr(t0, "/거래") || string_equal_cstr(t0, "/rjfo")
         || string_equal_cstr(t0, "/교환") || string_equal_cstr(t0, "/ryghks")
         || string_equal_cstr(t0, "/deal") || string_equal_cstr(t0, "/ㅇㄷ미"))
         && COUNT() == 2) {
        *type = 0x19;
        return first_arg(out, tokens);
    }
    if ((string_equal_cstr(t0, "/길드") || string_equal_cstr(t0, "/rlfem") || string_equal_cstr(t0, "/guild"))
        && COUNT() == 2) {
        *type = 0x37;
        return first_arg(out, tokens);
    }
    if ((string_equal_cstr(t0, "/해체") || string_equal_cstr(t0, "/gocp") || string_equal_cstr(t0, "/breakguild"))
        && COUNT() == 2) {
        *type = 0x38;
        return first_arg(out, tokens);
    }
    if ((string_equal_cstr(t0, "/결성") || string_equal_cstr(t0, "/ruftjd") || string_equal_cstr(t0, "/organize"))
        && COUNT() == 2) {
        *type = 0x1a;
        return first_arg(out, tokens);
    }
    if ((string_equal_cstr(t0, "/몬스터") || string_equal_cstr(t0, "/아이템")
         || string_equal_cstr(t0, "/item") || string_equal_cstr(t0, "/monster"))
         && COUNT() >= 2) {
        *type = 0x30;
        assign_token(out, tokens, 1);
        if (COUNT() < 3)
            return -1;
        string_init(&joined);
        n = COUNT();
        for (i = 1; i < n; n = COUNT()) {
            if (i + 1 != n) {
                struct String piece;

                string_copy(&piece, TOK(i));
                string_append_cstr(&piece, " ");
                string_append(&joined, string_cstr(&piece), piece.size);
                string_free(&piece);
            } else {
                append_token(&joined, tokens, i);
            }
            i++;
        }
        string_assign(out, string_cstr(&joined), joined.size);
        string_free(&joined);
        return -1;
    }
    if ((string_equal_cstr(t0, "/맵이동") || string_equal_cstr(t0, "/mapmove") || string_equal_cstr(t0, "/mm"))
        && COUNT() >= 2) {
        *type = 0x31;
        assign_token(out, tokens, 1);
        at = -1;
        if (COUNT() == 4)
            return args_2_3(out, tokens, at);
        string_assign_cstr(&out[2], "0");
        string_assign_cstr(&out[1], "0");
        return at;
    }
    if ((string_equal_cstr(t0, "/제명") || string_equal_cstr(t0, "/wpaud") || string_equal_cstr(t0, "/expel"))
        && COUNT() == 2) {
        *type = 0x1c;
        return first_arg(out, tokens);
    }
    if (string_equal_cstr(t0, "/sm") && COUNT() == 2) {
        *type = 0x3a;
        return first_arg(out, tokens);
    }
    if (string_equal_cstr(t0, "/resetstate")) {
        *type = 0x3d;
        return -1;
    }
    if (string_equal_cstr(t0, "/resetskill")) {
        *type = 0x3e;
        if (COUNT() >= 2)
            return first_arg(out, tokens);
        string_assign_cstr(out, "1");
        return -1;
    }
    if ((string_equal_cstr(t0, "/changemaptype") || string_equal_cstr(t0, "/cmt")) && COUNT() == 4) {
        *type = 0x3f;
        assign_token(out, tokens, 1);
        assign_token(&out[1], tokens, 2);
        assign_token(&out[2], tokens, 3);
        return -1;
    }
    if (string_equal_cstr(t0, "/emblem") || string_equal_cstr(t0, "/엠블렘")
        || string_equal_cstr(t0, "/앰블렘")
        || string_equal_cstr(t0, "/dpaqmffpa") || string_equal_cstr(t0, "/doaqmffpa")) {
        *type = 0x40;
        return -1;
    }
    if (string_equal_cstr(t0, "/hide")) {
        *type = 0x58;
        return -1;
    }
    if (string_equal_cstr(t0, "/폰트") || string_equal_cstr(t0, "/font")) {
        *type = 0x5c;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/mineffect") || string_equal_cstr(TOK(0), msg_str(0x2ba))) {
        *type = 0xe3;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/remove") && COUNT() == 2) {
        *type = 0x5d;
        return first_arg(out, tokens);
    }
    if (string_equal_cstr(TOK(0), "/summon") && COUNT() == 2) {
        *type = 0x60;
        return first_arg(out, tokens);
    }
    if (string_equal_cstr(TOK(0), "/shift")) {
        *type = 0x5e;
        for (i = 0; i < COUNT() - 1; out++) {
            i++;
            assign_token(out, tokens, i);
        }
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/recall") && COUNT() == 2) {
        *type = 0x5f;
        return first_arg(out, tokens);
    }
    if (string_equal_cstr(TOK(0), "/check") && COUNT() == 2) {
        *type = 0x8d;
        return first_arg(out, tokens);
    }
    if (string_equal_cstr(TOK(0), "/rc") && COUNT() == 2) {
        *type = 0x8e;
        return first_arg(out, tokens);
    }
    if (string_equal_cstr(TOK(0), "/요금")) {
        *type = 0x63;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/이름표시")
        || string_equal_cstr(TOK(0), "/showname")) {
        *type = 0x6a;
        return skip_blanks(text, at);
    }
    for (k = 0; k < sizeof s_statCommands / sizeof s_statCommands[0]; k++) {
        if (string_equal_cstr(TOK(0), s_statCommands[k].text) && COUNT() > 1) {
            void *mode;
            int points;

            *type = 0x87;
            mode = game_current_mode();
            points = atoi(string_cstr(TOK(1)));
            mode_send_message(mode, s_statCommands[k].msg,
                                                       s_statCommands[k].stat, points, 0, 0);
            return *type == 0 ? at : skip_blanks(text, at);
        }
    }
    if (string_equal_cstr(TOK(0), "/hi")) {
        string_init(&joined);
        *type = 0x95;
        if (COUNT() > 1) {
            assign_token(out, tokens, 1);
            for (i = 1; i < COUNT(); i++) {
                append_token(&joined, tokens, i);
                string_append_cstr(&joined, " ");
            }
            ui_window_mgr_set_chat_line(&g_windowMgr, string_cstr(&joined));
        }
        string_free(&joined);
        at = -1;
        return at;
    }
    if (string_equal_cstr(TOK(0), "/초대")
        || string_equal_cstr(TOK(0), "/cheo") || string_equal_cstr(TOK(0), "/invite")) {
        *type = 0xb6;
        string_init(&joined);
        string_assign_cstr(&joined, text);
        pos = string_find(&joined, " ", 0);
        if (pos == (size_t)-1) {
            ui_window_mgr_set_chat_line(&g_windowMgr, msg_str(0x9e4));
            string_free(&joined);
            at = -1;
            return at;
        }
        return invited(out, &joined, pos);
    }
    if (string_equal_cstr(TOK(0), "/길드초대")
        || string_equal_cstr(TOK(0), "/rlfemcheo") || string_equal_cstr(TOK(0), "/guildinvite")) {
        *type = 0xd1;
        string_init(&joined);
        string_assign_cstr(&joined, text);
        pos = string_find(&joined, " ", 0);
        if (pos == (size_t)-1) {
            ui_window_mgr_set_chat_line(&g_windowMgr, msg_str(0x9e5));
            string_free(&joined);
            at = -1;
            return at;
        }
        return invited(out, &joined, pos);
    }
    if (string_equal_cstr(TOK(0), "/web") && COUNT() >= 2) {
        *type = 0xce;
        assign_token(out, tokens, 1);
        return *type == 0 ? at : skip_blanks(text, at);
    }
    if ((string_equal_cstr(TOK(0), "/navi") || string_equal_cstr(TOK(0), "/navigation"))
        && COUNT() >= 3) {
        *type = 0xcf;
        at = -1;
        assign_token(out, tokens, 1);
        assign_token(&out[1], tokens, 2);
        return at;
    }
    if ((string_equal_cstr(TOK(0), "/navi2") || string_equal_cstr(TOK(0), "/navigation2"))
        && COUNT() >= 4) {
        *type = 0xd0;
        at = -1;
        assign_token(out, tokens, 1);
        return args_2_3(out, tokens, at);
    }
    if (string_equal_cstr(TOK(0), "/naviopen")) {
        *type = 0xd7;
        return -1;
    }
    if ((string_equal_cstr(TOK(0), "/cl") || string_equal_cstr(TOK(0), "/clanchat")) && COUNT() >= 2) {
        *type = 0xd4;
        string_assign_cstr(out, string_equal_cstr(TOK(0), "/cl") ? text + 4 : text + 0xa);
        return -1;
    }
    if (string_equal_cstr(TOK(0), msg_str(0x9df))) {
        *type = 0xd6;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/cashshop")) {
        *type = 0xd8;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/goldpc")) {
        *type = 0xd9;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/roulette")) {
        *type = 0xda;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/quake")) {
        *type = 0xdb;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/트윗")
        || string_equal_cstr(TOK(0), "/트위터")
        || string_equal_cstr(TOK(0), "/twitter")) {
        *type = 0xdc;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/매크로등록")
        || string_equal_cstr(TOK(0), "/macro_register") || string_equal_cstr(TOK(0), "/mr")) {
        *type = 0xde;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/매크로확인")
        || string_equal_cstr(TOK(0), "/macro_preview") || string_equal_cstr(TOK(0), "/mp")) {
        *type = 0xdf;
        at = -1;
        if (COUNT() >= 2)
            assign_token(out, tokens, 1);
        return at;
    }
    if (string_equal_cstr(TOK(0), "/매크로신고")
        || string_equal_cstr(TOK(0), "/macro_detector") || string_equal_cstr(TOK(0), "/md")) {
        *type = 0xe0;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/setcamera") && COUNT() > 2) {
        *type = 0xe2;
        assign_token(out, tokens, 1);
        assign_token(&out[1], tokens, 2);
        at = -1;
        if (COUNT() != 4)
            return at;
        return arg_3(out, tokens, at);
    }
    if (string_equal_cstr(TOK(0), "/resetcooltime") || string_equal_cstr(TOK(0), "/rct")) {
        *type = 0xe4;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/call") || string_equal_cstr(TOK(0), "/호출")) {
        *type = 0xe5;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/환복")
        || string_equal_cstr(TOK(0), "/changedress") || string_equal_cstr(TOK(0), "/nocosplay")) {
        *type = 0xed;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/노점보기")
        || string_equal_cstr(TOK(0), "/showshop")) {
        *type = 0xf8;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/prison")) {
        *type = 0xf9;
        if (COUNT() < 2)
            return -1;
        return first_arg(out, tokens);
    }
    if (string_equal_cstr(TOK(0), "/계정한정")
        || string_equal_cstr(TOK(0), "/accountlimitedtool")) {
        *type = 0xfa;
        return -1;
    }
    if (string_equal_cstr(TOK(0), "/모험가중개소")
        || string_equal_cstr(TOK(0), "/중개소")
        || string_equal_cstr(TOK(0), "/agency")) {
        *type = 0xeb;
        return -1;
    }
    *type = session_lookup_talk_type(self, string_cstr(TOK(0)));
    return -1;
}

int session_get_talk_type(struct Session *self, const char *text, int *type,
                                    struct String *out)
{
    struct Vec tokens;
    struct String line;
    int at;

    if (text == NULL) {
        *type = 3;
        return -1;
    }
    tokens.first = tokens.last = tokens.end = NULL;
    string_init(&line);
    string_assign_cstr(&line, text);
    tokens_split(&tokens, &line);
    string_free(&line);
    at = talk_type_scan(self, text, type, out, &tokens);
    string_vec_free(&tokens);
    return at;
}
