#include "lossy_link/relay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <enet/enet.h>

/* lossy_link <listen port> <server port> [loss=N] [repeat=N] [delay=MS] [jitter=MS] [seed=N]
 *
 * A lossy link on this machine, to play or test over (plans/delivery.md, step 6). It takes what
 * a client sends to 127.0.0.1:<listen port>, passes it to the server on 127.0.0.1:<server port>
 * and the answers back, and loses `loss` of a hundred datagrams, repeats `repeat` of a hundred
 * and holds each for `delay` ms and up to `jitter` more, each way, by `seed`. A round trip is
 * twice the delay longer. Point the client at the listen port; the server needs no change.
 * It runs until it is stopped, and says every ten seconds what went over it. */

static int setting(const char *argument, const char *name, unsigned long *value)
{
    size_t length = strlen(name);
    char *end;

    if (strncmp(argument, name, length) != 0 || argument[length] != '=')
        return 0;
    *value = strtoul(argument + length + 1, &end, 10);
    return end != argument + length + 1 && *end == 0;
}

int main(int argc, char **argv)
{
    struct LossyWireConfig config = { 0, 0, 0, 0 };
    unsigned long value, listen_port, server_port, seed = 1;
    struct LossyRelay *relay;
    const struct LossyWire *wire;
    enet_uint32 said;
    int i;

    if (argc < 3 || (listen_port = strtoul(argv[1], NULL, 10)) - 1 >= 65535
        || (server_port = strtoul(argv[2], NULL, 10)) - 1 >= 65535) {
        printf("usage: lossy_link <listen port> <server port> [loss=N] [repeat=N] [delay=MS] [jitter=MS] [seed=N]\n");
        return 2;
    }
    for (i = 3; i < argc; i++) {
        if (setting(argv[i], "loss", &value) && value <= 100)
            config.loss_pct = (unsigned)value;
        else if (setting(argv[i], "repeat", &value) && value <= 100)
            config.repeat_pct = (unsigned)value;
        else if (setting(argv[i], "delay", &value) && value <= 10000)
            config.delay_ms = (uint32_t)value;
        else if (setting(argv[i], "jitter", &value) && value <= 10000)
            config.jitter_ms = (uint32_t)value;
        else if (setting(argv[i], "seed", &value))
            seed = value;
        else {
            printf("lossy_link: cannot use \"%s\"\n", argv[i]);
            return 2;
        }
    }
    if (enet_initialize() != 0
        || !(relay = lossy_relay_open((uint16_t)listen_port, (uint16_t)server_port, (uint32_t)seed, &config))) {
        printf("lossy_link: cannot listen on UDP port %lu\n", listen_port);
        return 1;
    }
    wire = lossy_relay_wire(relay);
    printf("lossy link: 127.0.0.1:%lu to 127.0.0.1:%lu, each way %u%% lost, %u%% repeated, %u ms and up to %u ms more, seed %lu\n",
           listen_port, server_port, config.loss_pct, config.repeat_pct, (unsigned)config.delay_ms,
           (unsigned)config.jitter_ms, seed);
    fflush(stdout);
    for (said = enet_time_get();;) {
        enet_uint32 now = enet_time_get();

        lossy_relay_pump(relay, now);
        if (now - said >= 10000) {
            said = now;
            printf("to the server: %u datagrams, %u lost; to the client: %u, %u lost\n", (unsigned)wire->put[0],
                   (unsigned)wire->lost[0], (unsigned)wire->put[1], (unsigned)wire->lost[1]);
            fflush(stdout);
        }
        lossy_relay_wait(relay, 1);
    }
}
