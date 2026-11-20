-- What granny_models does not show of the Granny (3D) actors: several of one model (the device
-- draws a mesh of all of them as one instanced draw), turned to each of the eight directions, one
-- walking (scene.walk) and two attacking the player (scene.attack), and others at the screen's
-- edges, partly off it (the right one behind a roof). Two captures half a second apart show them in
-- other poses.
local player = 2000000

scene.enter_map("prontera", 156, 180, {name = "Watcher", job = 4008, hair = 5})
scene.wait_ready()
-- eight guardians of one model, each turned another way
for dir = 0, 7 do
    scene.spawn{aid = 110005000 + dir, kind = "monster", job = 1286, x = 148 + 2 * dir, y = 186, dir = dir}
end
-- the screen's edges: left, right (behind a roof), top and bottom, partly off it
local edges = {{1287, 136, 181}, {1285, 177, 190}, {1287, 158, 196}, {1285, 154, 165}}
for i, m in ipairs(edges) do
    scene.spawn{aid = 110005100 + i, kind = "monster", job = m[1], x = m[2], y = m[3], dir = i}
end
-- one that walks, two that attack the player
scene.spawn{aid = 110005201, kind = "monster", job = 1285, x = 150, y = 176, dir = 2}
scene.spawn{aid = 110005202, kind = "monster", job = 1287, x = 160, y = 177, dir = 6}
scene.spawn{aid = 110005203, kind = "monster", job = 1286, x = 153, y = 181, dir = 4}
scene.wait(30)
if not scene.walk(110005201, 162, 176) then
    scene.fail("the walker has no way to 162,176")
end
scene.attack{src = 110005202, target = player, motion = 400, flinch = 200}
scene.attack{src = 110005203, target = player, motion = 400, flinch = 200}
scene.wait(20)
scene.capture("granny_actions")
scene.wait(30)
scene.capture("granny_actions_later")
