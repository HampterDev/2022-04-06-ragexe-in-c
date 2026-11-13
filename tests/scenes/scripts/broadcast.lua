-- Server broadcasts (ZC_BROADCAST), a plain one and a blue one, each shown over the game and
-- written in the chat window with its sender's name, which the 64-bit client once narrowed to an
-- int on the way to the chat window and crashed on.
local function u16(v) return string.char(v % 256, math.floor(v / 256) % 256) end
local function broadcast(line) return u16(0x9a) .. u16(5 + #line) .. line .. "\0" end

scene.enter_map("prontera", 156, 186, {name = "Watcher", job = 4008, hair = 5})
scene.wait_ready()
scene.send(broadcast("Healbot has defeated the MVP!"))
scene.wait(5)
scene.send(broadcast("blueThe War of Emperium has begun."))
scene.wait(20)
scene.capture("broadcast")
