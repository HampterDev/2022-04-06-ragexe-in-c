"""The chat lines' lengths and repeats: a line longer than the stock 70 characters in the chat
line and in a chat room's title, and the same line said four times."""
from common import scenario


LONG = 'This line runs well past the seventy characters the stock client allowed in it'
REPEAT = 'again and again'

s = scenario('parity42')
s.type(LONG, then=10).snap('chat_line_long')
s.key('enter', then=30).snap('chat_line_sent')
for n in range(4):
    s.say(REPEAT, then=20)
s.snap('repeated_lines')

s.baseline().key('alt+c', then=60).snap('chat_room_setup')
s.click_ui('UIChatRoomMakeWnd/UIEditCtrl', then=10).type(LONG, then=10).snap('chat_room_title_long')
s.close_new()
s.quit()
STEPS = s.steps
