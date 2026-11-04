-- The Granny guardians and the treasure box die: their dead animations come from model\3dmob_bone.
scene.enter_map("prontera", 156, 180, {name = "Watcher", job = 4008, hair = 5})
scene.wait_ready()
local models = {{1285, 150, 186}, {1286, 155, 186}, {1287, 160, 186}, {1324, 155, 182}}
for i, m in ipairs(models) do
    scene.spawn{aid = 110003000 + i, kind = "monster", job = m[1], x = m[2], y = m[3], dir = 4}
end
scene.wait(30)
for i = 1, #models do
    scene.vanish(110003000 + i, 1)
end
scene.wait(45)
scene.capture("granny_dying")
scene.wait(300)
scene.capture("granny_dead")
