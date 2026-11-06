#ifndef CONTENTS_CONNECTION_API_H
#define CONTENTS_CONNECTION_API_H

struct Connection;
struct RagConnection;

struct ServerAddress {
    char ip[16];
    int port;
    char host[132];
};

char connection_startup(void);
void connection_resolve_server_address(void);
void connection_shutdown(void);
char connection_ws2_unhooked(void);
struct Connection *connection_construct(struct Connection *connection);
struct RagConnection *rag_connection_construct(struct RagConnection *connection);
void connection_destruct(struct Connection *connection);
char connection_connect(struct Connection *connection, const struct ServerAddress *address);
void connection_disconnect(struct Connection *connection);
char connection_flush(struct Connection *connection);
char connection_poll(struct Connection *connection);
int connection_recv_bytes(struct Connection *connection, void *dst, int count, char exact);
void connection_unread(struct Connection *connection, int count, const void *src);
void connection_set_hold_sends(struct Connection *connection, char hold);
void connection_clear_held(struct Connection *connection);
void connection_reset_queues(struct Connection *connection);
void connection_delay_sends(struct Connection *connection, unsigned milliseconds);
int connection_is_open(const struct Connection *connection);

struct RagConnection *rag_connection_instance(void);
struct Connection *rag_connection_transport(struct RagConnection *connection);
void rag_connection_drop_sends(struct RagConnection *connection, int drop);
int rag_connection_get_packet_length(struct RagConnection *connection, int packet_id);
unsigned short rag_connection_get_packet_type(const void *packet);
char rag_connection_get_packet(struct RagConnection *connection, void *buffer,
                             unsigned short *length);
char rag_connection_send_packet(struct RagConnection *connection, int length, const void *packet);

#endif
