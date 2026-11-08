"""A chat room made from the chat room window (Alt+C): its title typed, the member limit stepped down and
up, made private with a password and back, created; a line said in the room; the room left with its close
button; then a second window cancelled."""
from common import scenario


MAKE = 'UIChatRoomMakeWnd'
ROOM = 'UIChatRoomWnd'

s = scenario('parity46')
s.baseline().key('alt+c', then=40).snap('make_window')
s.click_ui(f'{MAKE}/UIEditCtrl', index=0, then=10).type('Parity room', then=10)
s.click_ui(f'{MAKE}/UIBitmapButton#220', then=10).click_ui(f'{MAKE}/UIBitmapButton#220', then=10)
s.click_ui(f'{MAKE}/UIBitmapButton#221', then=10).snap('make_limit')
s.click_ui(f'{MAKE}/UIRadioBtn#207', index=1, then=10)
s.click_ui(f'{MAKE}/UIEditCtrl', index=1, then=10).type('pw12', then=10).snap('make_private')
s.click_ui(f'{MAKE}/UIRadioBtn#207', index=0, then=10).snap('make_public')
s.click_ui(f'{MAKE}/UIBitmapButton#176', then=90).snap('room_open')
s.say('hello room', then=60).snap('room_said')
s.click_ui(f'{ROOM}/UIBitmapButton#193', then=90).snap('room_left')
s.baseline().key('alt+c', then=40).snap('make_again')
s.click_ui(f'{MAKE}/UIBitmapButton#177', then=40).snap('make_cancelled')
s.quit()
STEPS = s.steps
