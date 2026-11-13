-- Prontera's main street from the default camera: the player walks a loop south of the fountain.
local M = dofile(debug.getinfo(1, "S").source:match("^@(.*[/\\])") .. "_motion.lua")
M.enter("prontera", 156, 184, 488.8, -55)
M.walk("walk", {{156, 184, 156, 174}, {156, 174, 156, 164}, {156, 164, 166, 164}, {166, 164, 166, 174},
                {166, 174, 156, 184}}, 100)
scene.capture("walk")
