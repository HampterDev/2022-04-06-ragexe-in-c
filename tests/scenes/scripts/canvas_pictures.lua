-- The pictures windows draw into their own canvases: the monster the Sense skill shows
-- (ZC_MONSTER_INFO), and in the navigation window the picture of a map, the hint under the target
-- view (Share on a map), a monster found by level ($$lv) and an NPC listed with the Npc filter
-- ($$all).
local function u16(v) return string.char(v % 256, math.floor(v / 256) % 256) end
local function u32(v) return u16(v % 65536) .. u16(math.floor(v / 65536)) end

scene.enter_map("prontera", 156, 186, {name = "Watcher", job = 4008, hair = 5})
scene.wait_ready()

scene.send(u16(0x18c) .. u16(1002) .. u16(3) .. u16(1) .. u32(55) .. u16(2) .. u16(3) .. u16(5) .. u16(21)
    .. string.char(100, 100, 25, 125, 125, 90, 100, 100, 100))
scene.wait(10)
scene.capture("canvas_monster_info")

local function search(text)
    scene.click(790, 310)
    scene.wait(2)
    for _ = 1, 16 do
        scene.key(0x08, 0x0e)
    end
    scene.text(text)
    scene.key(scene.VK.RETURN, 0x1c)
    scene.wait(10)
    scene.click(720, 362)
    scene.wait(20)
end

scene.mouse(700, 600)
scene.open(0xcb)
scene.wait(10)
search("prontera")
scene.capture("canvas_navigation_map")
scene.click(840, 553)
scene.wait(10)
scene.capture("canvas_navigation_target")
scene.click(678, 310)
scene.wait(10)
search("$$lv1~3")
scene.capture("canvas_navigation_monster")
scene.click(715, 310)
scene.wait(5)
scene.click(715, 364)
scene.wait(5)
search("$$all")
scene.capture("canvas_navigation_npc")
