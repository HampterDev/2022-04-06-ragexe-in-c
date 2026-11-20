-- hu_fild01's hills from further and flatter: the player walks a loop over them.
local M = dofile(debug.getinfo(1, "S").source:match("^@(.*[/\\])") .. "_motion.lua")
M.enter("hu_fild01", 141, 90, 800, -40)
M.walk("walk_hu", {{141, 102}, {153, 102}, {153, 90}, {141, 90}}, 120)
scene.capture("walk_hu")
