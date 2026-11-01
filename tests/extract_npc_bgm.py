"""Compile the actual NPC music handoff against a controlled audio backend."""
from pathlib import Path
import sys

source, out = map(Path, sys.argv[1:])
out.mkdir(parents=True, exist_ok=True)


def function(path, signature):
    text = (source / path).read_text(encoding="utf-8")
    assert text.count(signature) == 1, signature
    start = text.index(signature)
    end = text.index("\n}", start) + 2
    return text[start:end]


npc = (source / "src/game/packets_npc.c").read_text(encoding="utf-8")
start = npc.index("#pragma pack(push, 1)\nstruct PACKET_ZC_PLAY_NPC_BGM")
end = npc.index("\n#pragma pack(pop)", start) + len("\n#pragma pack(pop)")
parts = [npc[start:end],
         function("src/game/packets_npc.c", "void game_mode_zc_play_npc_bgm("),
         function("src/game/mode_run.c", "static void restart_bgm(")]
(out / "npc_bgm_functions.inc").write_text("\n\n".join(parts) + "\n", encoding="utf-8")
sound_path = "src/contents/sound.c"
sound = (source / sound_path).read_text(encoding="utf-8")
stream_parts = [sound[:sound.index("static int clamp_volume(")]]
for signature in ("static char *copy_name(", "static void close_stream(",
                  "static void play_stream(", "void sound_play_stream(",
                  "void sound_play_bgm(", "void sound_stop_stream(",
                  "int sound_stream_playing(", "void sound_set_room_type("):
    stream_parts.append(function(sound_path, signature))
(out / "npc_bgm_sound.inc").write_text("\n\n".join(stream_parts) + "\n", encoding="utf-8")
