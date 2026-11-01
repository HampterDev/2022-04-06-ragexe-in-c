"""Compile the handler from the patched server, never a second implementation."""
from pathlib import Path
import sys

source = (Path(sys.argv[1]) / "src/map/clif.c").read_text(encoding="utf-8")
start = source.index("/* Purist Adventurer's Agency packet extension v1.")
end = source.index("static void clif_adventurerAgencyResult", start)
Path(sys.argv[2]).write_text(source[start:end], encoding="utf-8")
