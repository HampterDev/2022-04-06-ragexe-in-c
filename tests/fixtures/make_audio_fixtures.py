"""Regenerate the owned audio fixtures with FFmpeg and independent reference decoders.

Each ADPCM file is a synthesized sine encoded by FFmpeg. The matching .pcm file (raw signed 16-bit
little-endian, interleaved) is what the POSIX Miles decoder must reproduce exactly:
- Microsoft ADPCM: FFmpeg's own decoding, which follows Microsoft's algorithm bit for bit.
- IMA ADPCM: CPython's audioop.adpcm2lin, a port of the Intel/DVI reference decoder that
  Microsoft's codec also follows. FFmpeg's IMA decoder rounds differently, so it is not used.
The MP3 is a 440 Hz sine; its test checks the decoded tone, not exact samples.
Needs Python 3.12 or older (audioop) and FFmpeg with libmp3lame (set FFMPEG if not on PATH).
"""

import audioop
import os
import struct
import subprocess
from pathlib import Path


FFMPEG = os.environ.get("FFMPEG", "ffmpeg")
HERE = Path(__file__).parent
BITEXACT = ["-fflags", "+bitexact", "-flags:a", "+bitexact"]


def run(*args):
    subprocess.run([FFMPEG, "-hide_banner", "-loglevel", "error", "-y", *args], check=True)


def source(channels, seconds):
    tone = "sine=f={}:r=22050:d=%s" % seconds
    if channels == 1:
        return ["-filter_complex", tone.format(440) + ",volume=6[a]", "-map", "[a]"]
    return ["-filter_complex",
            tone.format(440) + "[l];" + tone.format(660) + "[r];[l][r]amerge=inputs=2,volume=6[a]",
            "-map", "[a]"]


def chunks(data):
    offset = 12
    while offset + 8 <= len(data):
        name, length = data[offset:offset + 4], struct.unpack_from("<I", data, offset + 4)[0]
        yield name, data[offset + 8:offset + 8 + length]
        offset += 8 + length + (length & 1)


def ima_reference(wav):
    parts = dict(chunks(wav.read_bytes()))
    _, channels, _, _, block_align, _ = struct.unpack_from("<HHIIHH", parts[b"fmt "])
    data = parts[b"data"]
    frames = []
    for start in range(0, len(data), block_align):
        block = data[start:start + block_align]
        decoded = []
        for channel in range(channels):
            predictor, index = struct.unpack_from("<hB", block, 4 * channel)
            nibbles = []
            for group in range(4 * channels, len(block) - 4 * channels + 1, 4 * channels):
                for byte in block[group + 4 * channel:group + 4 * channel + 4]:
                    nibbles += [byte & 15, byte >> 4]
            packed = bytes(nibbles[i] << 4 | nibbles[i + 1] for i in range(0, len(nibbles), 2))
            samples = audioop.adpcm2lin(packed, 2, (predictor, index))[0]
            decoded.append([predictor] + list(struct.unpack("<%dh" % (len(samples) // 2), samples)))
        frames += zip(*decoded)
    return b"".join(struct.pack("<%dh" % channels, *frame) for frame in frames)


for codec, short in (("adpcm_ms", "ms"), ("adpcm_ima_wav", "ima")):
    for channels, layout in ((1, "mono"), (2, "stereo")):
        wav = HERE / f"audio_{short}_adpcm_{layout}.wav"
        run(*source(channels, 0.1), "-c:a", codec, *BITEXACT, str(wav))
        if short == "ms":
            run("-i", str(wav), "-f", "s16le", "-c:a", "pcm_s16le", *BITEXACT, str(wav.with_suffix(".pcm")))
        else:
            wav.with_suffix(".pcm").write_bytes(ima_reference(wav))

run(*source(1, 0.3), "-c:a", "libmp3lame", "-b:a", "32k", "-metadata", "title=sine", str(HERE / "audio_sine.mp3"))
