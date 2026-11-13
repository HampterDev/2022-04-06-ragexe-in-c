-- What the load scenes share: the map server's packets as a busy server sends them, players dressed
-- as they are on busy servers, the ground cells of skills in their real shapes, the frame's size, a
-- camera turned and zoomed as a player turns it, and the measured stretch. A scene loads it with
--   local load = dofile(scene.dir .. "lib/load.lua")
-- (see tests/scenes/README.md, "Load scenes"). Packet layouts are rAthena's for this client's packet
-- version; skill, unit and effect IDs are the game data's.
local M = {}

-- ---- bytes ----

local floor = math.floor
local function u8(v) return string.char(v % 256) end
local function u16(v) return string.char(v % 256, floor(v / 256) % 256) end
local function u32(v) return u16(v % 65536) .. u16(floor(v / 65536) % 65536) end
local function text(s, n) s = s:sub(1, n - 1) return s .. string.rep("\0", n - #s) end

-- the 6 bytes of a walk from x0, y0 to x1, y1 (rAthena's WBUFPOS2, sub-cells 8, 8)
local function move_data(x0, y0, x1, y1)
    return string.char(floor(x0 / 4), (x0 % 4) * 64 + floor(y0 / 16) % 64, (y0 % 16) * 16 + floor(x1 / 64) % 16,
        (x1 % 64) * 4 + floor(y1 / 256) % 4, y1 % 256, 0x88)
end

-- ---- the map server's packets ----

local P = {}
M.packet = P

-- ZC_NOTIFY_MOVE: an actor walks from x0, y0 to x1, y1, starting now
function P.move(aid, x0, y0, x1, y1)
    return u16(0x86) .. u32(aid) .. move_data(x0, y0, x1, y1) .. u32(scene.ticks())
end
-- ZC_NOTIFY_ACT3: a hit (action 0), a critical one (10) or several (8), with its damage number
function P.hit(src, dst, damage, action, count)
    return u16(0x8c8) .. u32(src) .. u32(dst) .. u32(scene.ticks()) .. u32(300) .. u32(230) .. u32(damage) .. u8(0)
        .. u16(count or 1) .. u8(action or 0) .. u32(0)
end
-- ZC_NOTIFY_SKILL2: a skill hits dst (action 5 splash, 6 single, 8 several hits)
function P.skill_hit(skill, src, dst, damage, level, count, action)
    return u16(0x1de) .. u16(skill) .. u32(src) .. u32(dst) .. u32(scene.ticks()) .. u32(300) .. u32(230)
        .. u32(damage) .. u16(level or 10) .. u16(count or 1) .. u8(action or 6)
end
-- ZC_USE_SKILL2: a skill without damage (a buff, a heal of `level` points, a self skill)
function P.use_skill(skill, src, dst, level)
    return u16(0x9cb) .. u16(skill) .. u32(level or 10) .. u32(dst) .. u32(src) .. u8(1)
end
-- ZC_NOTIFY_GROUNDSKILL: a skill cast on a cell
function P.ground_skill(skill, src, x, y, level)
    return u16(0x117) .. u16(skill) .. u32(src) .. u16(level or 10) .. u16(x) .. u16(y) .. u32(scene.ticks())
end
-- ZC_USESKILL_ACK2: the casting bar of a skill on dst or on a cell (element `property`)
function P.cast(src, dst, x, y, skill, ms, property)
    return u16(0x7fb) .. u32(src) .. u32(dst or 0) .. u16(x or 0) .. u16(y or 0) .. u16(skill) .. u32(property or 0)
        .. u32(ms) .. u8(0)
end
-- ZC_SKILL_ENTRY5: a skill's ground unit (one cell, or a range for units the client draws whole)
function P.unit(id, creator, x, y, unit, range, level)
    return u16(0x9ca) .. u16(23) .. u32(id) .. u32(creator) .. u16(x) .. u16(y) .. u32(unit) .. u8(range or 0)
        .. u8(1) .. u8(level or 10)
end
-- ZC_SKILL_DISAPPEAR
function P.unit_gone(id) return u16(0x120) .. u32(id) end
-- ZC_MSG_STATE_CHANGE3: a status on (an icon, an aura) or off
function P.status(aid, index, on, ms)
    return u16(0x983) .. u16(index) .. u32(aid) .. u8(on and 1 or 0) .. u32(ms or 0) .. u32(ms or 0) .. u32(0)
        .. u32(0) .. u32(0)
end
-- ZC_STATE_CHANGE3: body state (2 frozen, 3 stunned), health state and option bits
function P.state(aid, body, health, option)
    return u16(0x229) .. u32(aid) .. u16(body or 0) .. u16(health or 0) .. u32(option or 0) .. u8(0)
end
-- ZC_NOTIFY_VANISH: gone out of sight (0), dead (1), logged out (2), teleported (3)
function P.vanish(aid, how) return u16(0x80) .. u32(aid) .. u8(how or 0) end
-- ZC_RESURRECTION: a dead player stands up
function P.resurrect(aid) return u16(0x148) .. u32(aid) .. u16(0) end
-- ZC_MVP: the MVP of a boss's death
function P.mvp(aid) return u16(0x10c) .. u32(aid) end
-- ZC_STORE_ENTRY: a vending shop's sign
function P.shop(aid, title) return u16(0x131) .. u32(aid) .. text(title, 80) end
-- ZC_ROOM_NEWENTRY: a chat room over a player
function P.room(aid, id, title, users, max, public)
    return u16(0xd7) .. u16(17 + #title) .. u32(aid) .. u32(id) .. u16(max) .. u16(users) .. u8(public and 1 or 0)
        .. title
end
-- ZC_EMOTION
function P.emotion(aid, kind) return u16(0xc0) .. u32(aid) .. u8(kind) end
-- ZC_SPIRITS: a monk's spirit spheres
function P.spirits(aid, n) return u16(0x1d0) .. u32(aid) .. u16(n) end
-- ZC_NPCACK_MAPMOVE: the player warps to x, y of another map on the same map server
function P.warp(map, x, y) return u16(0x91) .. text(map .. ".gat", 16) .. u16(x) .. u16(y) end
-- ZC_BROADCAST: a line every player on the server reads
function P.broadcast(line) return u16(0x9a) .. u16(5 + #line) .. line .. "\0" end

-- ---- skills, units and statuses (rAthena's IDs, the client's skill and effect tables) ----

M.SKILL = {
    HEAL = 28, INCAGI = 29, BLESSING = 34, PNEUMA = 25, SAFETYWALL = 12, SANCTUARY = 70, KYRIE = 73,
    MAGNIFICAT = 74, MAGNUS = 79, METEOR = 83, JUPITEL = 84, LOV = 85, ICEWALL = 87, STORMGUST = 89,
    BRANDISH = 57, BOWLINGBASH = 62, ASURA = 271, LANDPROTECTOR = 288, BRAGI = 321, SERVICE = 330,
    ASSUMPTIO = 361, PRESSURE = 367, SONICBLOW = 136, METEORASSAULT = 406, ACIDDEMO = 490, CARTCANNON = 2477,
    SHARPSHOOTING = 382, POTIONPITCHER = 231, RESURRECTION = 54, HOLYCROSS = 253, THUNDERSTORM = 21,
    GANBANTEIN = 483, SPIRALPIERCE = 397, DARKBREATH = 202, DARKSTRIKE = 340, HELLJUDGEMENT = 662,
    EARTHQUAKE = 653, PULSESTRIKE = 661, FIREBREATH = 654, DRAGONFEAR = 659,
}
-- unit types of ZC_SKILL_ENTRY5
M.UNIT = {
    SAFETYWALL = 0x7e, SANCTUARY = 0x83, MAGNUS = 0x84, PNEUMA = 0x85, ICEWALL = 0x8d, LANDPROTECTOR = 0x9d,
    BRAGI = 0xa9, SERVICE = 0xaf, EARTHQUAKE = 0xc6,
}
-- status icons and auras of ZC_MSG_STATE_CHANGE3 (EFST_*)
M.EFST = {
    TWOHANDQUICKEN = 2, BLESSING = 10, INCAGI = 12, KYRIE = 19, MAGNIFICAT = 20, EXPLOSIONSPIRITS = 86,
    POWERUP = 98, ASSUMPTIO = 110,
}
-- the cells of a skill's ground units, as offsets from the cell cast on (rAthena's layouts)
M.CELLS = {
    sanctuary = {{-1,-2},{0,-2},{1,-2},{-2,-1},{-1,-1},{0,-1},{1,-1},{2,-1},{-2,0},{-1,0},{0,0},{1,0},{2,0},
                 {-2,1},{-1,1},{0,1},{1,1},{2,1},{-1,2},{0,2},{1,2}},
    magnus = {{-1,-3},{0,-3},{1,-3},{-1,-2},{0,-2},{1,-2},{-3,-1},{-2,-1},{-1,-1},{0,-1},{1,-1},{2,-1},{3,-1},
              {-3,0},{-2,0},{-1,0},{0,0},{1,0},{2,0},{3,0},{-3,1},{-2,1},{-1,1},{0,1},{1,1},{2,1},{3,1},
              {-1,2},{0,2},{1,2},{-1,3},{0,3},{1,3}},
    icewall = {{-2,0},{-1,0},{0,0},{1,0},{2,0}},
}
-- a square of (2r+1)^2 cells: Pneuma (r 1), Land Protector (r 5 at level 5), songs (r 3)
function M.square(r)
    local cells = {}
    for dy = -r, r do
        for dx = -r, r do
            cells[#cells + 1] = {dx, dy}
        end
    end
    return cells
end

-- ---- deterministic randomness (the client's own rand() stays untouched) ----

-- The Microsoft runtime's generator, its products exact in Lua's doubles; 15 bits a step, the high
-- ones, as the low bits of such a generator repeat with a short period.
function M.rng(seed)
    local state = seed % 4294967296
    local r = {}
    local function bits15()
        state = (state * 214013 + 2531011) % 4294967296
        return floor(state / 65536) % 32768
    end
    -- an integer from lo to hi (a span of up to 2^30)
    function r.int(lo, hi) return lo + (bits15() * 32768 + bits15()) % (hi - lo + 1) end
    function r.pick(list) return list[r.int(1, #list)] end
    function r.chance(percent) return r.int(1, 100) <= percent end
    return r
end

-- ---- players as busy servers have them ----

M.OPTION = {CART = 0x8, FALCON = 0x10, RIDING = 0x20, CART5 = 0x400, DRAGON = 0x80000, MADOGEAR = 0x400000}
-- class: job ID, weapon views to pick from, shield views, the option bits the share `mounted` (in
-- percent) of them carry, and the job ID a mounted one has instead (rAthena's status_set_viewdata
-- sends a lord knight or a paladin on a Peco Peco as a job of its own)
M.CLASS = {
    lord_knight = {job = 4008, weapons = {4, 5, 3}, shields = {0, 0, 2, 4}, option = M.OPTION.RIDING, mounted = 70,
                   mounted_job = 4014},
    paladin = {job = 4015, weapons = {4, 2}, shields = {4, 3}, option = M.OPTION.RIDING, mounted = 50,
               mounted_job = 4022},
    high_priest = {job = 4009, weapons = {8, 15}, shields = {0, 2, 4}},
    high_wizard = {job = 4010, weapons = {10}, shields = {0}},
    professor = {job = 4017, weapons = {15, 10}, shields = {0, 2}},
    sniper = {job = 4012, weapons = {11}, shields = {0}, option = M.OPTION.FALCON, mounted = 60},
    assassin_cross = {job = 4013, weapons = {16, 25, 1}, shields = {0}},
    champion = {job = 4016, weapons = {12}, shields = {0}},
    creator = {job = 4019, weapons = {8, 6}, shields = {4, 2}, option = M.OPTION.CART5, mounted = 80},
    whitesmith = {job = 4011, weapons = {6, 7}, shields = {0, 3}, option = M.OPTION.CART, mounted = 80},
    stalker = {job = 4018, weapons = {2, 11}, shields = {0, 2}},
    clown = {job = 4020, weapons = {13}, shields = {0}},
    gypsy = {job = 4021, weapons = {14}, shields = {0}},
    rune_knight = {job = 4060, weapons = {5, 3}, shields = {0, 4}, option = M.OPTION.DRAGON, mounted = 50},
    arch_bishop = {job = 4063, weapons = {8, 15}, shields = {0, 2}},
    warlock = {job = 4061, weapons = {10}, shields = {0}},
    genetic = {job = 4078, weapons = {8, 6}, shields = {4, 2}, option = M.OPTION.CART5, mounted = 90},
    mechanic = {job = 4064, weapons = {6, 7}, shields = {0}, option = M.OPTION.MADOGEAR, mounted = 40},
    sura = {job = 4077, weapons = {12}, shields = {0}},
    minstrel = {job = 4075, weapons = {13}, shields = {0}},
    wanderer = {job = 4076, weapons = {14}, shields = {0}},
    royal_guard = {job = 4073, weapons = {4}, shields = {4, 3}, option = M.OPTION.RIDING, mounted = 50},
    guillotine = {job = 4065, weapons = {16}, shields = {0}},
    ranger = {job = 4062, weapons = {11}, shields = {0}, option = M.OPTION.FALCON, mounted = 50},
    sorcerer = {job = 4074, weapons = {10, 15}, shields = {0}},
    novice = {job = 0, weapons = {1}, shields = {0}},
    merchant = {job = 5, weapons = {6}, shields = {0}, option = M.OPTION.CART, mounted = 100},
}
-- headgears (accessory IDs of the game data's AccessoryId.lua) players wear, top, middle and lower
M.HEADGEAR = {
    top = {41, 225, 45, 88, 19, 99, 36, 112, 40, 110, 208, 380, 562, 137, 98, 129, 300, 39, 38},
    mid = {0, 0, 12, 26, 158, 3, 23, 82, 13},
    low = {0, 0, 0, 54, 55, 52, 22, 21, 91},
}
local NAMES = {"xX~DarkLord~Xx", "Healbot", "Sakura", "ForTheGuild", "Lazy", "Zeny Farmer", "kRO Veteran",
               "Pew Pew", "Asura4Life", "NoobSlayer", "Mr Bubbles", "Valkyrie", "Kitsune", "BerserkBob",
               "Lady Gypsy", "SGspammer", "Tankety Tank", "Smith of Steel", "Arrow Rain", "Lord of Swords",
               "Moonlight", "Priestess Ai", "Chibi", "OneShotOneKill", "Ice Queen", "Pharaoh"}

-- the spawn table of a player of `class` at x, y (see scene.spawn); `extra` adds or replaces fields
function M.player(rng, aid, class, x, y, extra)
    local c = M.CLASS[class] or error("no class " .. tostring(class))
    local mounted = c.option and rng.chance(c.mounted or 100)
    local p = {
        aid = aid, kind = "player", job = mounted and c.mounted_job or c.job, x = x, y = y, dir = rng.int(0, 7),
        sex = rng.int(0, 1), hair = rng.int(1, 27), hair_color = rng.int(0, 8), body_palette = rng.int(0, 3),
        level = c.job >= 4054 and 175 or 99, weapon = rng.pick(c.weapons), shield = rng.pick(c.shields),
        head_top = rng.pick(M.HEADGEAR.top), head_mid = rng.pick(M.HEADGEAR.mid), head_bottom = rng.pick(M.HEADGEAR.low),
        robe = rng.chance(40) and rng.int(1, 31) or 0, name = rng.pick(NAMES) .. (aid % 1000), max_hp = -1,
        hp = -1, speed = 150, option = mounted and c.option or 0,
    }
    for k, v in pairs(extra or {}) do
        p[k] = v
    end
    return p
end

-- a cell around cx, cy within r (a disc), as players stand
function M.around(rng, cx, cy, r)
    local x, y
    repeat
        x, y = rng.int(-r, r), rng.int(-r, r)
    until x * x + y * y <= r * r
    return cx + x, cy + y
end

-- ---- ground cells ----

local unit_ids = 130000000
-- places a skill's units at cx, cy in the shape `cells`; returns their IDs (for unit_gone)
function M.place(creator, unit, cx, cy, cells, level)
    local ids = {}
    for _, d in ipairs(cells) do
        unit_ids = unit_ids + 1
        scene.send(P.unit(unit_ids, creator, cx + d[1], cy + d[2], unit, 0, level))
        ids[#ids + 1] = unit_ids
    end
    return ids
end
function M.remove(ids)
    for _, id in ipairs(ids) do
        scene.send(P.unit_gone(id))
    end
end

-- ---- the frame and the camera, as a player turns and zooms it ----

-- Every load scene draws a frame of this size, whatever size the game folder's options give the
-- window, so that the numbers of two machines are of the same frame.
M.WIDTH, M.HEIGHT = 1600, 900

-- From the login screen into `map` at x, y (scene.enter_map's opts), with the frame at M.WIDTH by
-- M.HEIGHT once the map has loaded.
function M.enter(map, x, y, opts)
    scene.enter_map(map, x, y, opts)
    scene.wait_ready()
    scene.resize(M.WIDTH, M.HEIGHT)
    scene.wait(2)
end

-- One step of a camera that turns (right button held and dragged), rests and zooms (the wheel) in a
-- cycle of 360 frames; call it every frame with the frame's number. The pointer stays over the
-- middle of the view, away from the windows.
function M.camera()
    local cx, cy = floor(M.WIDTH / 2), floor(M.HEIGHT / 2) - 40
    local x = cx
    local held = false
    local period = 360
    return function(f)
        local t = f % period
        local half = floor(period / 2)
        local turning = (t >= 20 and t < 80) or (t >= half + 20 and t < half + 80)
        if turning and not held then
            x = cx
            scene.mouse(x, cy)
            scene.button(x, cy, true, "right")
            held = true
        elseif turning then
            x = x + ((t < half) and 5 or -5)
            scene.mouse(x, cy)
        elseif held then
            scene.button(x, cy, false, "right")
            held = false
        end
        if t == 100 or t == 106 then
            scene.wheel(120)
        elseif t == half + 100 or t == half + 106 then
            scene.wheel(-120)
        end
    end
end

-- ---- a fight: who stands where, what they do, and what follows ----

-- A fight keeps its actors (where they stand, their side, alive or dead, until when they walk) and
-- what is to be sent later (the waves of a Storm Gust, a ground skill's cells going, a dead player
-- standing up). F.step() runs once a frame, before the frame's own actions; F.frame counts them.
function M.fight(seed)
    local rng = M.rng(seed)
    local F = {rng = rng, actors = {}, sides = {}, frame = 0}
    local later = {}

    -- fn runs `frames` frames from now (at least one)
    function F.at(frames, fn)
        local f = F.frame + math.max(1, floor(frames))
        later[f] = later[f] or {}
        table.insert(later[f], fn)
    end

    function F.step()
        F.frame = F.frame + 1
        local due = later[F.frame]
        if due then
            later[F.frame] = nil
            for _, fn in ipairs(due) do
                fn()
            end
        end
    end

    -- spawns p (a scene.spawn table) on `side`
    function F.add(p, side)
        scene.spawn(p)
        local a = {aid = p.aid, x = p.x, y = p.y, side = side, alive = true, busy = 0, spec = p}
        F.actors[#F.actors + 1] = a
        F.sides[side] = F.sides[side] or {}
        table.insert(F.sides[side], a)
        return a
    end

    -- the living actors of `side` within r cells of x, y
    function F.within(side, x, y, r)
        local found = {}
        for _, a in ipairs(F.sides[side] or {}) do
            if a.alive and math.abs(a.x - x) <= r and math.abs(a.y - y) <= r then
                found[#found + 1] = a
            end
        end
        return found
    end

    -- one of them, or nil
    function F.pick(side, x, y, r)
        local found = F.within(side, x, y, r)
        return #found > 0 and found[rng.int(1, #found)] or nil
    end

    -- a walk to x, y, unless the actor is dead or still walking (150 ms a cell, diagonals 1.4 times)
    function F.walk(a, x, y)
        if not a.alive or F.frame < a.busy or (x == a.x and y == a.y) then
            return
        end
        scene.send(P.move(a.aid, a.x, a.y, x, y))
        local dx, dy = math.abs(x - a.x), math.abs(y - a.y)
        local long, short = math.max(dx, dy), math.min(dx, dy)
        a.x, a.y = x, y
        a.busy = F.frame + floor(((long - short) * 150 + short * 212) / 16) + 2
    end

    -- a walk to a cell within r of x, y
    function F.wander(a, x, y, r)
        local tx, ty = M.around(rng, x, y, r)
        F.walk(a, tx, ty)
    end

    -- a dead player stands up (ZC_RESURRECTION)
    function F.stand(a)
        if a.alive then
            return
        end
        scene.send(P.resurrect(a.aid))
        a.alive = true
        a.busy = F.frame + 30
    end

    -- dies (lies on the ground); a player stands up again after `stand` frames unless something
    -- stood them up before (F.stand)
    function F.die(a, stand)
        if not a.alive then
            return
        end
        a.alive = false
        a.deaths = (a.deaths or 0) + 1
        scene.send(P.vanish(a.aid, 1))
        if stand then
            local death = a.deaths
            F.at(stand, function()
                if a.deaths == death then
                    F.stand(a)
                end
            end)
        end
    end

    local function damage(around)
        return rng.int(floor(around * 0.7), floor(around * 1.3))
    end

    -- a normal hit, critical now and then, its number over the target
    function F.hit(a, t, around)
        if a.alive and t and t.alive then
            local crit = rng.chance(20)
            scene.send(P.hit(a.aid, t.aid, damage(around), crit and 10 or 0))
        end
    end

    -- a skill on t after a casting bar of opts.cast ms: a hit of about opts.dmg in opts.count numbers,
    -- or without opts.dmg a skill without damage (opts.level is a heal's amount) and its status icon
    -- opts.status; opts.then_(t) runs as it lands
    function F.skill(a, t, skill, opts)
        if not a.alive or not t or not t.alive then
            return
        end
        local cast = opts.cast or 0
        if cast > 0 then
            scene.send(P.cast(a.aid, t.aid, t.x, t.y, skill, cast, opts.element or 0))
        end
        F.at(cast / 16 + 1, function()
            if not a.alive or not t.alive then
                return
            end
            if opts.dmg then
                local count = opts.count or 1
                scene.send(P.skill_hit(skill, a.aid, t.aid, damage(opts.dmg), opts.level or 10, count,
                    opts.action or (count > 1 and 8 or 6)))
            else
                scene.send(P.use_skill(skill, a.aid, t.aid, opts.level or 10))
                if opts.status then
                    scene.send(P.status(t.aid, opts.status, true, opts.status_ms or 120000))
                end
            end
            if opts.then_ then
                opts.then_(t)
            end
        end)
    end

    -- a ground skill at x, y after a casting bar of opts.cast ms: its effect, then opts.hits waves
    -- opts.every frames apart, each hitting the living of side opts.enemy within opts.r cells for
    -- about opts.dmg; opts.on_hit(t, wave) runs for each target hit
    function F.ground(a, skill, x, y, opts)
        if not a.alive then
            return
        end
        local cast = opts.cast or 0
        if cast > 0 then
            scene.send(P.cast(a.aid, 0, x, y, skill, cast, opts.element or 0))
        end
        F.at(cast / 16 + 1, function()
            if not a.alive then
                return
            end
            scene.send(P.ground_skill(skill, a.aid, x, y, opts.level or 10))
            for wave = 0, (opts.hits or 1) - 1 do
                F.at(wave * (opts.every or 27) + 1, function()
                    for _, t in ipairs(F.within(opts.enemy, x, y, opts.r or 3)) do
                        scene.send(P.skill_hit(skill, a.aid, t.aid, damage(opts.dmg), opts.level or 10,
                            opts.count or 1, 5))
                        if opts.on_hit then
                            opts.on_hit(t, wave)
                        end
                    end
                end)
            end
        end)
    end

    -- a skill's ground cells at x, y in the shape `cells` for `frames` frames, cast on the ground
    -- (ZC_NOTIFY_GROUNDSKILL) or, with self_cast, on the caster (ZC_USE_SKILL2, songs and dances)
    function F.cells(a, skill, unit, x, y, cells, frames, self_cast)
        if not a.alive then
            return
        end
        if self_cast then
            scene.send(P.use_skill(skill, a.aid, a.aid, 10))
        else
            scene.send(P.ground_skill(skill, a.aid, x, y, 10))
        end
        local ids = M.place(a.aid, unit, x, y, cells, 10)
        F.at(frames, function()
            M.remove(ids)
        end)
    end

    return F
end

-- ---- the measured stretch ----

-- RAGEXE_LOAD_FRAMES: the frames every scene measures instead of its own number, for a quick look
local frames_asked = tonumber(os.getenv("RAGEXE_LOAD_FRAMES") or "")

-- Calls step(f) once a frame for f from 1: `opts.warm` frames (120) to get the scene going, then
-- the measured stretch, opts.frames frames (600, or RAGEXE_LOAD_FRAMES) that scene.measure logs to
-- <out>/<name>.frames.jsonl, then 30 frames more before <name> is captured. When the stretch has
-- ended, before those last frames, opts.after() runs if given: what is to happen in the capture but
-- outside the measured stretch.
function M.measured(name, step, opts)
    opts = opts or {}
    local f = 0
    local function run()
        f = f + 1
        step(f)
        scene.wait(1)
    end
    for _ = 1, opts.warm or 120 do
        run()
    end
    scene.measure(frames_asked or opts.frames or 600, name)
    while scene.measuring() > 0 do
        run()
    end
    if opts.after then
        opts.after()
    end
    for _ = 1, 30 do
        run()
    end
    scene.capture(name)
end

return M
