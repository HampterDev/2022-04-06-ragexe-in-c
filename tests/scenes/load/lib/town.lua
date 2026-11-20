-- A busy town as a busy server's main town is: rows of vending shops with their signs (merchants'
-- carts, mechanics' mado gears), chat rooms, players walking up and down the street with their pets,
-- players standing about talking and pulling faces, mounts, falcons, long names and guild emblems.
--
-- `town` describes it, or names one of the towns below:
--   seed
--   street = {x0, x1, y0, y1}  the street people walk along (north and south, x0 to x1 wide)
--   shops = {x, ...}           the columns the vendors stand in, from y0 to y1 two cells apart
--   vendors, rooms, walkers, idlers, pets   how many of each
--   extra = {...}              fields every player gets (a Santa hat, for example)
-- All of them appear at once when the builder is called (as on arriving in the town). Returns the
-- town's step, to be called once a frame.
local TOWNS = {
    -- Prontera's main street south of the fountain, 180 players and 20 pets
    prontera = {seed = 7007, street = {151, 161, 135, 190}, shops = {145, 147, 165, 167}, vendors = 60, rooms = 15,
                walkers = 70, idlers = 35, pets = 20},
    -- Lutie at Christmas, under its snow: 100 players in Santa hats and 10 pets
    lutie = {seed = 8008, street = {198, 208, 115, 150}, shops = {193, 195, 211, 213}, vendors = 20, rooms = 6,
             walkers = 44, idlers = 30, pets = 10, extra = {head_top = 20}},
}
return function(load, town)
    town = TOWNS[town] or town
    local P = load.packet
    local F = load.fight(town.seed)
    local rng = F.rng
    local st = town.street
    local aid = 2600000
    local function next_aid()
        aid = aid + 1
        return aid
    end
    local function dress(p)
        for k, v in pairs(town.extra or {}) do
            p[k] = v
        end
        if rng.chance(50) then
            p.guild, p.emblem = rng.int(1, 12) * 101, 1
        end
        return p
    end
    local SHOPS = {"S> +10 Cards cheap!!", "B> Emperium 1.5m", "S> Elu/Ori/Potions", "S>Yggdrasil Berries 5k",
                   "S> MVP cards PM me", "Refine service +7 safe", "S> Blue Gemstones", "B> Old Card Album",
                   "S> Arrows all kinds", "S> Headgears rare!!!", "Cheap pots @ here", "S> Mastela Fruit"}
    local ROOMS = {"LF party Thor Volcano", "WoE guild recruiting", "Selling accounts? no", "Free buffs inside",
                   "Bio3 party 2/12", "Trading cards here", "afk", "Endless Tower 99f"}
    local LINES = {"anyone for ET?", "lol", "wts +9 vesper core", "brb", "where is the kafra", "pvp me",
                   "need priest for bio", "zeny pls", "gg", "thanks!"}
    local vendor_classes = {"whitesmith", "creator", "genetic", "mechanic", "merchant"}
    local walker_classes = {"lord_knight", "high_priest", "high_wizard", "sniper", "assassin_cross", "champion",
                            "rune_knight", "arch_bishop", "warlock", "ranger", "sura", "royal_guard", "sorcerer",
                            "guillotine", "minstrel", "wanderer", "stalker", "professor", "novice"}

    -- the vendors in rows, a sign each
    local column, row = 1, 0
    for _ = 1, town.vendors do
        local x, y = town.shops[column], st[3] + row * 2
        local a = F.add(dress(load.player(rng, next_aid(), rng.pick(vendor_classes), x, y, {dir = 2})), "vendor")
        scene.send(P.shop(a.aid, rng.pick(SHOPS)))
        column = column + 1
        if column > #town.shops then
            column, row = 1, row + 1
            if st[3] + row * 2 > st[4] then
                row = 0
            end
        end
    end
    -- the chat rooms, around the street's middle
    for i = 1, town.rooms do
        local x, y = load.around(rng, math.floor((st[1] + st[2]) / 2), math.floor((st[3] + st[4]) / 2), 10)
        local a = F.add(dress(load.player(rng, next_aid(), rng.pick(walker_classes), x, y)), "room")
        local max = rng.int(2, 20)
        scene.send(P.room(a.aid, 900 + i, rng.pick(ROOMS), rng.int(1, max), max, rng.chance(80)))
    end
    -- the walkers, some with a pet at heel
    local pet_jobs = {1002, 1063, 1107, 1056, 1077, 1031, 1052, 1113, 1167, 1170}
    local pets = 0
    for _ = 1, town.walkers do
        local x, y = rng.int(st[1], st[2]), rng.int(st[3], st[4])
        local a = F.add(dress(load.player(rng, next_aid(), rng.pick(walker_classes), x, y)), "walker")
        if pets < town.pets then
            pets = pets + 1
            a.pet = F.add({aid = 110030000 + pets, kind = "pet", job = rng.pick(pet_jobs), x = x + 1, y = y,
                           dir = 0, speed = 150}, "pet")
        end
    end
    -- the players standing about
    for _ = 1, town.idlers do
        local x, y = rng.int(st[1] - 6, st[2] + 6), rng.int(st[3], st[4])
        F.add(dress(load.player(rng, next_aid(), rng.pick(walker_classes), x, y)), "idler")
    end

    local walkers = F.sides.walker or {}
    for _, a in ipairs(walkers) do
        a.period = rng.int(30, 90)
        a.phase = rng.int(0, a.period - 1)
    end
    local function step()
        F.step()
        local f = F.frame
        for _, a in ipairs(walkers) do
            if (f + a.phase) % a.period == 0 then
                local x = rng.int(st[1], st[2])
                local y = a.y + (rng.chance(50) and 1 or -1) * rng.int(6, 14)
                y = math.max(st[3], math.min(st[4], y))
                F.walk(a, x, y)
                if a.pet and a.busy > f then
                    a.pet.busy = 0
                    F.walk(a.pet, x + 1, y)
                end
            end
        end
        if f % 23 == 0 then
            local a = F.actors[rng.int(1, #F.actors)]
            if a.spec.kind == "player" then
                scene.say(a.aid, a.spec.name .. " : " .. rng.pick(LINES))
            end
        end
        if f % 17 == 0 then
            local a = F.actors[rng.int(1, #F.actors)]
            scene.send(P.emotion(a.aid, rng.int(0, 33)))
        end
    end
    return step
end
