-- Prontera's main street from the default camera, shaken as a skill's earthquake shakes it.
local M = dofile(debug.getinfo(1, "S").source:match("^@(.*[/\\])") .. "_motion.lua")
M.enter("prontera", 156, 184, 488.8, -55)
M.run("pre", 20)
scene.quake(6500)
M.run("quake", 420)
scene.capture("quake")
