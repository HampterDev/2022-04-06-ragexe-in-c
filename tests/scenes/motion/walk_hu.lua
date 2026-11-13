-- hu_fild01's hills from further and flatter: the player walks a loop over them.
local M = dofile(debug.getinfo(1, "S").source:match("^@(.*[/\\])") .. "_motion.lua")
M.enter("hu_fild01", 80, 288, 800, -40)
M.walk("walk_hu", {{80, 288, 80, 300}, {80, 300, 92, 300}, {92, 300, 92, 288}, {92, 288, 80, 288}}, 120)
scene.capture("walk_hu")
