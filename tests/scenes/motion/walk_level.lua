-- Prontera's main street with the camera zoomed in to its closest and tilted to its flattest (as
-- camera_level_view), the eye a few units above the ground; then the player walks a loop.
local M = dofile(debug.getinfo(1, "S").source:match("^@(.*[/\\])") .. "_motion.lua")
M.enter("prontera", 155, 161, 488.8, -55)
M.drag("level_zoom", M.CONTROL, M.CONTROL_SCAN, 400, 300, 0, 40, 40)
M.drag("level_tilt", M.SHIFT, M.SHIFT_SCAN, 400, 300, 0, -40, 40)
scene.wait(60)
M.walk("walk_level", {{155, 161, 155, 171}, {155, 171, 160, 176}, {160, 176, 155, 161}}, 120)
scene.capture("walk_level")
