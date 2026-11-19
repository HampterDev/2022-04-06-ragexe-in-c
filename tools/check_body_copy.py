"""Checks that the body core here is the server's (plans/body-core.md).

    python tools/check_body_copy.py [server checkout]

The core's source lives in the server's repository under sim/body and this client holds a copy:
the core under src/body, its tests under tests. Both sides compile the same files, so a rule
changed on one side only would be a client that predicts something the server does not do.
This compares the files, line ends aside, and fails on the first that differs.

Without a server checkout at the given place, or with one that has no sim/body, it says so and
passes: the check is for a machine that has both.
"""
import os
import sys

CLIENT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
# where tools/grid_path_vectors.mjs looks for the server too
DEFAULT_SERVER = r"C:\Users\Patrick Kuhlmann\GIT\ring-buffer\local\worktrees\action-timing"

# the server's file under sim/body -> this client's
COPIES = [
    ("body.c", "src/body/body.c"),
    ("body.h", "src/body/body.h"),
    ("grid_path.c", "src/body/grid_path.c"),
    ("grid_path.h", "src/body/grid_path.h"),
    ("body_tests.c", "tests/test_body.c"),
    ("body_scenario.c", "tests/body_scenario.c"),
    ("body_scenario.h", "tests/body_scenario.h"),
    ("body_vectors.h", "tests/body_vectors.h"),
]


def text(path):
    with open(path, "rb") as file:
        return file.read().replace(b"\r\n", b"\n")


def main():
    server = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_SERVER
    source = os.path.join(server, "sim", "body")
    if not os.path.isdir(source):
        print("check_body_copy: skipped, no server checkout with sim/body at " + server)
        return 0
    for theirs, ours in COPIES:
        if text(os.path.join(source, theirs)) != text(os.path.join(CLIENT, ours)):
            print("check_body_copy: " + ours + " is not the server's sim/body/" + theirs)
            return 1
    print("check_body_copy: passed: %d files are the server's" % len(COPIES))
    return 0


if __name__ == "__main__":
    sys.exit(main())
