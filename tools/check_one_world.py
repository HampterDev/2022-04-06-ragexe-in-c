"""Checks that the client runs one world of the body core (plans/predicted-world.md, "One path").

    python tools/check_one_world.py

What the player's own character does ahead of the server is the predicted world's alone
(src/world/predicted.c): the server's word, the player's requests and the other bodies go into
it, and everything drawn of the own character is read from it. A second file that makes a
world of the core, runs one or puts an input into one would be a second prediction, which
could say something else. So no file under src but the core itself (src/body) and
src/world/predicted.c may call the core's functions that make, change or run a world, or name
one of its inputs. Reading what a world or a record says (body_move_at, the structs, the
kinds of a request) is free.

Fails on the first file that does, with the line.
"""
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
SRC = os.path.join(ROOT, "src")
# the one file that may, with its header, and the core's own
ALLOWED = {os.path.join("world", "predicted.c"), os.path.join("world", "predicted.h")}
CORE = "body" + os.sep
# making, changing and running a world, and the names of the inputs
FORBIDDEN = re.compile(
    r"\b(body_world_init|body_input|body_run|body_put_state|body_put_move|body_set_cancel_penalty|"
    r"body_set_request_steps|body_take_event|BODY_INPUT_[A-Z_]+)\b")


def main():
    checked = 0
    for folder, _, names in os.walk(SRC):
        for name in names:
            if not name.endswith((".c", ".h")):
                continue
            path = os.path.join(folder, name)
            rel = os.path.relpath(path, SRC)
            if rel.startswith(CORE) or rel in ALLOWED:
                continue
            checked += 1
            with open(path, "r", encoding="utf-8", errors="replace") as file:
                for number, line in enumerate(file, 1):
                    found = FORBIDDEN.search(line)
                    if found:
                        print("check_one_world: FAILED: src/%s:%d uses %s; only src/world/predicted.c runs a world "
                              "of the body core" % (rel.replace(os.sep, "/"), number, found.group(1)))
                        return 1
    print("check_one_world: passed: %d files, and only world/predicted.c runs a world of the body core" % checked)
    return 0


if __name__ == "__main__":
    sys.exit(main())
