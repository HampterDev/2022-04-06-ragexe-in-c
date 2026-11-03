-- The keyboard: Alt+E opens the inventory, Enter opens the chat line, a typed line goes out with
-- Enter and comes back from the map server.
scene.enter_map("iz_int03", 18, 26, {name = "Novice", hair = 1})
scene.wait_ready()
scene.mouse(700, 600)
scene.wait(2)
scene.alt_key(0x45, 0x12) -- E
scene.wait(5)
scene.key(scene.VK.RETURN, 0x1c)
scene.wait(5)
scene.text("typed in a scene")
scene.wait(3)
scene.key(scene.VK.RETURN, 0x1c)
scene.wait(10)
scene.capture("keyboard")
