-- Text in scripts the interface's face lacks: Hangul, symbols such as ★ and kana, which the text DC
-- draws in each script's fallback font (Gulim or Malgun Gothic, MS PGothic), in the chat window, in
-- names over the actors and in what they say.
scene.enter_map("iz_int03", 18, 26, {name = "한글이름", hair = 1})
scene.wait_ready()
scene.spawn{aid = 2000101, kind = "player", job = 7, name = "기사★", x = 20, y = 28, hair = 2, dir = 0, level = 99}
scene.spawn{aid = 2000102, kind = "player", job = 8, name = "Priest 사제", x = 16, y = 28, hair = 3, sex = 0, dir = 0}
scene.spawn{aid = 110000001, kind = "monster", job = 1002, x = 22, y = 24, dir = 4}
scene.wait(30)
scene.say(2000101, "기사★ : 안녕하세요! ★☆♥ 한글 텍스트")
scene.say(2000102, "Priest 사제 : 힐 받으세요 ♪")
scene.chat("가나다라마바사 아자차카타파하 ★ 0123")
scene.chat("Mixed: Hangul 한글, symbols ★☆♥→※, kana あいうアイウ")
scene.mouse(700, 600)
scene.wait(10)
scene.capture("hangul_text")
