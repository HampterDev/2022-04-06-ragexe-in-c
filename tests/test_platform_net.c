#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "platform/clock.h"
#include "platform/net.h"
#include "platform/thread.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET NativeSocket;
#define close_native closesocket
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int NativeSocket;
#define close_native close
#endif

static NativeSocket listen_loopback(uint16_t *port)
{
    struct sockaddr_in address;
    socklen_t size = sizeof address;
    NativeSocket listener = socket(AF_INET, SOCK_STREAM, 0);
    memset(&address, 0, sizeof address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = platform_net_parse_ipv4("127.0.0.1");
    assert(bind(listener, (struct sockaddr *)&address, sizeof address) == 0);
    assert(listen(listener, 1) == 0);
    assert(getsockname(listener, (struct sockaddr *)&address, &size) == 0);
    *port = ntohs(address.sin_port);
    return listener;
}

static int wait_ready(struct PlatformSocket *sock, int writing, uint32_t milliseconds)
{
    uint32_t started = platform_ticks_ms();
    do {
        int ready = writing ? platform_socket_writable(sock) : platform_socket_readable(sock);
        assert(ready >= 0);
        if (ready)
            return 1;
        platform_thread_yield();
    } while ((uint32_t)(platform_ticks_ms() - started) < milliseconds);
    return 0;
}

static struct PlatformSocket *open_client(uint16_t port)
{
    struct PlatformSocket *client = platform_socket_open();
    assert(client);
    assert(platform_socket_set_nonblocking(client));
    assert(platform_socket_connect(client, platform_net_parse_ipv4("127.0.0.1"), port));
    return client;
}

static void test_addresses(void)
{
    uint32_t address = 0;
    assert(platform_net_to_host32(platform_net_parse_ipv4("1.2.3.4")) == 0x01020304u);
    assert(strcmp(platform_net_ipv4_text(platform_net_parse_ipv4("10.20.30.40")), "10.20.30.40") == 0);
    assert(platform_net_parse_ipv4("300.1.1.1") == 0xffffffffu);
    assert(platform_net_resolve_ipv4("127.0.0.1", &address));
    assert(address == platform_net_parse_ipv4("127.0.0.1"));
    assert(platform_net_resolve_ipv4("localhost", &address));
    assert(platform_net_to_host32(address) >> 24 == 127);
}

static void test_exchange(void)
{
    char buffer[64];
    uint16_t port;
    NativeSocket listener = listen_loopback(&port), server;
    struct PlatformSocket *client = open_client(port);
    int got;

    server = accept(listener, NULL, NULL);
    assert(wait_ready(client, 1, 5000));
    assert(!platform_socket_readable(client));
    assert(platform_socket_recv(client, buffer, (int)sizeof buffer) == PLATFORM_SOCKET_RETRY);
    assert(platform_socket_send(client, "ping", 4) == 4);
    assert(recv(server, buffer, (int)sizeof buffer, 0) == 4 && memcmp(buffer, "ping", 4) == 0);
    assert(send(server, "pong!", 5, 0) == 5);
    assert(wait_ready(client, 0, 5000));
    got = platform_socket_recv(client, buffer, (int)sizeof buffer);
    assert(got == 5 && memcmp(buffer, "pong!", 5) == 0);
    close_native(server);
    assert(wait_ready(client, 0, 5000));
    assert(platform_socket_recv(client, buffer, (int)sizeof buffer) == 0);
    platform_socket_close(client);
    close_native(listener);
}

static void test_broken_peer(void)
{
    char block[4096] = {0};
    uint16_t port;
    NativeSocket listener = listen_loopback(&port), server;
    struct PlatformSocket *client = open_client(port);
    int sent = 0, i;

    server = accept(listener, NULL, NULL);
    assert(wait_ready(client, 1, 5000));
    close_native(server);
    close_native(listener);
    for (i = 0; i < 1000 && sent != PLATFORM_SOCKET_FAILED; ++i) {
        if (wait_ready(client, 1, 100))
            sent = platform_socket_send(client, block, (int)sizeof block);
    }
    assert(sent == PLATFORM_SOCKET_FAILED);
    platform_socket_close(client);
}

static void test_refused(void)
{
    uint16_t port;
    NativeSocket listener = listen_loopback(&port);
    struct PlatformSocket *client;

    close_native(listener);
    client = platform_socket_open();
    assert(client && platform_socket_set_nonblocking(client));
    if (platform_socket_connect(client, platform_net_parse_ipv4("127.0.0.1"), port)) {
        assert(!wait_ready(client, 0, 300));
        assert(!wait_ready(client, 1, 300));
    }
    platform_socket_close(client);
}

int main(void)
{
    assert(platform_net_startup());
    test_addresses();
    test_exchange();
    test_broken_peer();
    test_refused();
    platform_net_shutdown();
    puts("Platform net: address text, resolution, loopback connect, send, receive, peer close, reset and refused connect passed");
    return 0;
}
