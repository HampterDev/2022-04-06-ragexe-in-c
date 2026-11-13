-- Many bombers on an MVP: Ifrit in Thor's Volcano (thor_v03, lib/mvp.lua) against 30 creators and
-- geneticists casting Acid Demonstration (ten numbers a hit) and Cart Cannon every 25 to 45 frames
-- each, so that a cast lands nearly every frame, with two high priests and a paladin. Ifrit has
-- its Kasa and Salamander and casts Earthquake, as it does in real play (a 0.5 s casting bar, three
-- waves on every bomber near it; here every 150 frames), and Fire Breath, Meteor Storm, Dragon Fear
-- and Pulse Strike in between; it dies right after the measured stretch and the MVP is announced.
-- The first capture is taken in the middle of an Earthquake's shaking; the camera turns and zooms.
local load = dofile(scene.dir .. "lib/load.lua")
local S = load.SKILL
load.enter("thor_v03", 249, 237, {name = "Watcher", job = 4078, hair = 5, hair_color = 3, level = 175, weapon = 8,
                                  shield = 4, head_top = 41, dir = 0})
local run = dofile(scene.dir .. "lib/mvp.lua")(load, {
    seed = 5005, job = 1832, x = 253, y = 241, quake_cast = 500, quake_every = 150,
    slaves = {1834, 1835, 1834, 1835},
    skills = {{S.FIREBREATH, "single", 0, 12000}, {S.METEOR, "ground", 1000, 5000}, {S.DRAGONFEAR, "area", 0, 4000},
              {S.PULSESTRIKE, "area", 0, 6000}},
    party = {{"creator", 15}, {"genetic", 15}, {"high_priest", 2}, {"paladin", 1}},
    period = {25, 45},
})
run("bombers_ifrit")
