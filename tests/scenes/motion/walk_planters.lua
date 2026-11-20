-- Prontera's fountain from the default camera: the player walks around it past the flower beds and
-- the banners, which come into the bottom of the view and leave it.
local M = dofile(debug.getinfo(1, "S").source:match("^@(.*[/\\])") .. "_motion.lua")
M.enter("prontera", 156, 184, 488.8, -55)
M.walk("walk_planters", {{146, 186}, {145, 197}, {145, 208}, {156, 211}, {166, 206}, {166, 195},
                         {156, 184}}, 100)
scene.capture("walk_planters")
