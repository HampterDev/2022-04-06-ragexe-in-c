-- Baphomet in the Labyrinth (prt_maze03) against a party of 19 (lib/mvp.lua): the boss with six
-- Baphomet Jr., its Earthquake (a 2 s casting bar, then three waves on everyone near, each shaking the
-- camera) every 150 frames, Dark Breath, Hell Judgement, Lord of Vermilion, Brandish Spear and Dark
-- Strike in between; the party tanks, heals, buffs (Blessing, Increase AGI, Kyrie, Assumptio,
-- Magnificat), plays songs and hits with Asura Strike, Spiral Pierce, Acid Demonstration, Sharp
-- Shooting and Jupitel Thunder; players die and are resurrected. Baphomet dies right after the
-- measured stretch and the MVP is announced. The first capture is taken in the middle of an
-- Earthquake's shaking, before the measured stretch; the camera turns and zooms.
local load = dofile(scene.dir .. "lib/load.lua")
local S = load.SKILL
load.enter("prt_maze03", 135, 55, {name = "Watcher", job = 4009, hair = 5, hair_color = 3, level = 99, weapon = 8,
                                   head_top = 41, dir = 0})
local run = dofile(scene.dir .. "lib/mvp.lua")(load, {
    seed = 3003, job = 1039, x = 139, y = 59, quake_cast = 2000, quake_every = 150,
    slaves = {1101, 1101, 1101, 1101, 1101, 1101},
    skills = {{S.DARKBREATH, "single", 800, 9000}, {S.HELLJUDGEMENT, "area", 1000, 6000},
              {S.LOV, "ground", 1500, 4000}, {S.BRANDISH, "single", 0, 7000}, {S.DARKSTRIKE, "ground", 1000, 3000}},
    party = {{"paladin", 1}, {"high_priest", 3}, {"high_wizard", 2}, {"sniper", 3}, {"champion", 2},
             {"lord_knight", 2}, {"creator", 2}, {"clown", 1}, {"gypsy", 1}, {"professor", 1}, {"assassin_cross", 1}},
})
run("mvp_baphomet")
