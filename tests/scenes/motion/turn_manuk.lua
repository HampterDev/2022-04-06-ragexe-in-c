-- Manuk's town on its hill from further and flatter, turned a pixel of a right drag a frame.
local M = dofile(debug.getinfo(1, "S").source:match("^@(.*[/\\])") .. "_motion.lua")
M.enter("manuk", 304, 80, 800, -40)
M.drag("turn_manuk", 0, 0, 112, 300, 1, 0, 800)
scene.capture("turn_manuk")
