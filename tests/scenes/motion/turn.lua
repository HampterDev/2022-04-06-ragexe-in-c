-- Prontera's main street from the default camera, turned two pixels of a right drag a frame.
local M = dofile(debug.getinfo(1, "S").source:match("^@(.*[/\\])") .. "_motion.lua")
M.enter("prontera", 156, 184, 488.8, -55)
M.drag("turn", 0, 0, 112, 300, 2, 0, 400)
scene.capture("turn")
