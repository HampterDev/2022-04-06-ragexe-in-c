`cooldown_geometry.bin` records 1,600 frames from the previous production
cooldown geometry implementation, captured before replacing its tree with
an eight-element array. `test_cooldown_geometry.c` compares the new code
against every frame byte for byte.

Each little-endian record contains nine 32-bit fields (segment, vertex count,
angle, total duration, start, elapsed, last-minute flag, color, queued flag)
followed by 24 vertices of 32 bytes each: 804 bytes per record. The angle and
vertex coordinates are IEEE 754 floats. The fixture's SHA-256 is
`de1edd1f824c3a0fd0cf26ff7ce74da2295ab30dd3022b594befc674452f7492`.

Scenarios include both drawing modes, live and replay clocks, all segment
boundaries, zero and expired durations, durations above the signed range,
the last-minute transition, clock wrap, elapsed offsets, and repeated layout
with changing dimensions. The test specifies the scenario order.

The capture used the original `state_icon_sheet.c` and `std_tree.c`, with
adapters for clock reads and render submission. Capture sources, executable,
logs, and original inputs are archived locally in
`build/native_c/cooldown_geometry_baseline.zip`. Keep this fixture unchanged
when refactoring; do not regenerate expected output from the new implementation.
