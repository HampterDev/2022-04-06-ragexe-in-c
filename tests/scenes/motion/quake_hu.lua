-- hu_fild01's hills from further and flatter, shaken as a skill's earthquake shakes it.
local M = dofile(debug.getinfo(1, "S").source:match("^@(.*[/\\])") .. "_motion.lua")
M.enter("hu_fild01", 80, 288, 800, -40)
scene.quake(6500)
M.run("quake_hu", 420)
scene.capture("quake_hu")
