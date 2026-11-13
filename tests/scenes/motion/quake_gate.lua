-- Prontera's south gate from further and flatter, shaken as a skill's earthquake shakes it.
local M = dofile(debug.getinfo(1, "S").source:match("^@(.*[/\\])") .. "_motion.lua")
M.enter("prontera", 156, 40, 800, -40)
scene.quake(6500)
M.run("quake_gate", 420)
scene.capture("quake_gate")
