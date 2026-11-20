"""Checks that the client runs one world of the actor core (plans/predicted-world.md, "One path").

    python tools/check_one_world.py

What the player's own character does ahead of the server is the predicted world's alone (the
piece `rules`, game/rules/prediction/predicted.c): the server's word, the player's requests and
the other actors go into it, and everything drawn of the own character is read from it. A file of
the client that makes a world of the core, runs one or puts an input into one would be a second
prediction, which could say something else. So no file under src may call the core's functions
that make, change or run a world, or name one of its inputs. Reading what a world or a record
says (actor_move_at, the structs, the kinds of a request) is free.

Fails on the first file that does, with the line.
"""
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
SRC = os.path.join(ROOT, "src")
# making, changing and running a world, and the names of the inputs
FORBIDDEN = re.compile(
    r"\b(actor_world_init|actor_input|actor_run|actor_put_state|actor_put_move|actor_set_cancel_penalty|"
    r"actor_set_request_steps|actor_take_event|ACTOR_INPUT_[A-Z_]+)\b")


def main():
    checked = 0
    for folder, _, names in os.walk(SRC):
        for name in names:
            if not name.endswith((".c", ".h")):
                continue
            path = os.path.join(folder, name)
            rel = os.path.relpath(path, SRC)
            checked += 1
            with open(path, "r", encoding="utf-8", errors="replace") as file:
                for number, line in enumerate(file, 1):
                    found = FORBIDDEN.search(line)
                    if found:
                        print("check_one_world: FAILED: src/%s:%d uses %s; only the predicted world of rules runs a "
                              "world of the actor core" % (rel.replace(os.sep, "/"), number, found.group(1)))
                        return 1
    print("check_one_world: passed: %d files, and none runs a world of the actor core" % checked)
    return 0


if __name__ == "__main__":
    sys.exit(main())
