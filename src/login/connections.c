#include "login/loginmode.h"
#include "ui/ui_std.h"

static void read_text(void *elem, const char *name, struct String *s)
{
    void *child = xml_find_child(elem, name);
    const struct String *t;

    if (!child)
        return;
    t = xml_node_text(child);
    string_assign(s, string_cstr(t), t->size);
}

static void push_copy(struct Vec *v, const struct LoginConnEntry *e)
{
    struct LoginConnEntry copy;

    login_conn_entry_copy(&copy, e);
    vec_push(v, &copy, sizeof copy);
}

BYTE login_mode_load_connections(struct LoginMode *self)
{
    struct Vec *list = &self->connections;
    struct LoginConnEntry e;
    void *info, *conn;

    if (list->first == list->last) {
        info = get_client_info();
        if (!info)
            return 0;
        conn = xml_find_child(info, "connection");
        if (!conn)
            return 0;
        do {
            string_init(&e.display);
            string_init(&e.desc);
            string_init(&e.balloon);
            string_init(&e.address);
            string_init(&e.port);
            read_text(conn, "display", &e.display);
            read_text(conn, "desc", &e.desc);
            read_text(conn, "balloon", &e.balloon);
            push_copy(list, &e);
            conn = xml_find_next(conn, "connection");
            string_free(&e.port);
            string_free(&e.address);
            string_free(&e.balloon);
            string_free(&e.desc);
            string_free(&e.display);
        } while (conn);
    }
    return g_hideAccountList == 0;
}
