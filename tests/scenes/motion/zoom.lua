-- Prontera's main street from the default camera, zoomed out and back in (Ctrl and a right drag).
local M = dofile(debug.getinfo(1, "S").source:match("^@(.*[/\\])") .. "_motion.lua")
M.enter("prontera", 156, 184, 488.8, -55)
M.drag("zoom", M.CONTROL, M.CONTROL_SCAN, 400, 200, 0, 3, 200)
M.drag("zoom", M.CONTROL, M.CONTROL_SCAN, 400, 800, 0, -3, 200)
scene.capture("zoom")
