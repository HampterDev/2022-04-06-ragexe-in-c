-- hu_fild01's hills from further and flatter, zoomed out and back in.
local M = dofile(debug.getinfo(1, "S").source:match("^@(.*[/\\])") .. "_motion.lua")
M.enter("hu_fild01", 80, 288, 800, -40)
M.drag("zoom_hu", M.CONTROL, M.CONTROL_SCAN, 400, 200, 0, 3, 200)
M.drag("zoom_hu", M.CONTROL, M.CONTROL_SCAN, 400, 800, 0, -3, 200)
scene.capture("zoom_hu")
