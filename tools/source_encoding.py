"""Keep UTF-8 sources readable while compiling legacy narrow literals as CP949."""

import argparse
from pathlib import Path
import re


TOKEN = re.compile(
    r'//[^\n]*|/\*.*?\*/|(?:u8|[LuU])?(?:"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')',
    re.DOTALL,
)


def encode_literals(source: str) -> str:
    # Splicing precedes tokenization in C, including inside // comments.
    positions = []
    pieces = []
    start = 0
    for splice in re.finditer(r"\\\r?\n", source):
        pieces.append(source[start:splice.start()])
        positions.extend(range(start, splice.start()))
        start = splice.end()
    pieces.append(source[start:])
    positions.extend(range(start, len(source)))
    logical = "".join(pieces)
    replacements = {}
    for match in TOKEN.finditer(logical):
        token = match.group()
        if token[0] not in "\"'":
            continue
        for offset, char in enumerate(token, match.start()):
            if ord(char) >= 128:
                replacements[positions[offset]] = "".join(f"\\{byte:03o}" for byte in char.encode("cp949"))
    return "".join(replacements.get(index, char) for index, char in enumerate(source))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.source.resolve() == args.output.resolve():
        parser.error("output must differ from the UTF-8 source")
    try:
        output = encode_literals(args.source.read_text(encoding="utf-8-sig"))
    except UnicodeError as error:
        parser.error(f"{args.source}: {error}")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(output, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
