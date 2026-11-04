-- The menu buttons of the Basic Information window: status, inventory and skills open over the map.
scene.enter_map("iz_int03", 18, 26, {name = "Novice", hair = 1})
scene.wait_ready()
scene.click(150, 165)  -- skills
scene.wait(5)
scene.click(24, 165)   -- status
scene.wait(5)
scene.click(108, 165)  -- inventory
scene.wait(5)
scene.mouse(700, 600)
scene.chat("Scene harness: windows opened from the menu.")
scene.wait(10)
scene.capture("windows_open")
