"""The NPC windows (the Parity Talk NPC of the parity server): its illustration, a page and the
next, a twelve-row menu answered by row (the NPC menu rows setting's window), a number and a
word asked for, the closing page. Talked through twice: the menu's first row, then its last."""
from common import scenario

TALK_AT, NPC_JOB = (806, 370), 117


s = scenario('parity09')
s.baseline().click_actor(TALK_AT, job=NPC_JOB, then=120).snap('talk_first')
s.converse('talk_a', choices=(0,), rounds=10)
s.close_new().snap('talk_a_done')
s.baseline().click_actor(TALK_AT, job=NPC_JOB, then=120)
s.converse('talk_b', choices=(11,), rounds=10)
s.close_new().snap('talk_b_done')
s.quit()
STEPS = s.steps
