"""Checks that the delivery piece here is the server's (plans/delivery.md).

    python tools/check_delivery_copy.py [server checkout]

The piece's source lives in the server's repository under net/delivery and this client holds a
copy: the piece under src/delivery, its tests under tests. The client and the gateway are the
two ends of one link, so a rule changed on one side only would be two ends that no longer
understand each other. This compares the files as check_body_copy.py compares the body core's,
and fails on the first that differs.

Without a server checkout at the given place, or with one that has no net/delivery, it says so
and passes: the check is for a machine that has both.
"""
import os
import sys

from check_body_copy import CLIENT, DEFAULT_SERVER, text

# the server's file under net/delivery -> this client's
COPIES = [
    ("delivery.c", "src/delivery/delivery.c"),
    ("delivery.h", "src/delivery/delivery.h"),
    ("delivery_tests.c", "tests/test_delivery.c"),
    ("delivery_route.c", "src/delivery/delivery_route.c"),
    ("delivery_route.h", "src/delivery/delivery_route.h"),
    ("delivery_route_tests.c", "tests/test_delivery_route.c"),
]


def main():
    server = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_SERVER
    source = os.path.join(server, "net", "delivery")
    if not os.path.isdir(source):
        print("check_delivery_copy: skipped, no server checkout with net/delivery at " + server)
        return 0
    for theirs, ours in COPIES:
        if text(os.path.join(source, theirs)) != text(os.path.join(CLIENT, ours)):
            print("check_delivery_copy: " + ours + " is not the server's net/delivery/" + theirs)
            return 1
    print("check_delivery_copy: passed: %d files are the server's" % len(COPIES))
    return 0


if __name__ == "__main__":
    sys.exit(main())
