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

BYTE login_mode_load_sub_connections(struct LoginMode *self, int index)
{
    struct Vec *list = &self->subConnections;
    struct LoginConnEntry e;
    const struct String *t;
    void *info, *conn, *sub, *child;

    if (list->first == list->last) {
        info = get_client_info();
        if (!info)
            return 0;
        conn = xml_find_child(info, "connection");
        if (!conn)
            goto done;
        if (index) {
            do {
                if (!conn)
                    goto done;
                conn = xml_find_next(conn, "connection");
            } while (--index);
        }
        if (!conn)
            goto done;

        sub = xml_find_child(conn, "subconnection");
        if (sub) {
            do {
                login_conn_entry_construct(&e);
                read_text(sub, "display", &e.display);
                read_text(sub, "desc", &e.desc);
                read_text(sub, "balloon", &e.balloon);
                push_copy(list, &e);
                sub = xml_find_next(sub, "subconnection");
                login_conn_entry_destruct(&e);
            } while (sub);
        } else {
            login_conn_entry_construct(&e);
            if ((child = xml_find_child(conn, "display")) != NULL) {
                t = xml_node_text(child);
                string_assign(&e.display, string_cstr(t), t->size);
            }
            if ((child = xml_find_child(conn, "desc")) != NULL) {
                t = xml_node_text(child);
                string_assign(&e.desc, string_cstr(t), t->size);
            }
            if ((child = xml_find_child(conn, "balloon")) != NULL) {
                t = xml_node_text(child);
                string_assign(&e.balloon, string_cstr(t), t->size);
            }
            push_copy(list, &e);
            login_conn_entry_destruct(&e);
        }
    }
done:
    return g_hideAccountList == 0;
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
