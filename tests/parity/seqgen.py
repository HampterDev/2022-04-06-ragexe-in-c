"""Generated map server streams for the differential tester (seqfuzz.py).

A case is a seeded list of events (actors entering, moving and leaving, ground skill units and items
coming and going, hits, skills, status changes, deaths, map changes, state checkpoints), turned into
the packets Hercules (PACKETVER 20220406, ~/ragexe/Hercules src/map/clif.c) sends for them, laid out as
src/map/packets_struct.h lays them out (LAYOUTS; tests/ragexe/seqgen checks them against Hercules'
own struct sizes). The stream follows a recorded session's login and map entry (PREFIX), so both
clients are standing on the map when the generated part starts, and is written as a recording:

    {"connections": [... the recording's login and char connections ...,
                     {map connection: its recorded events up to the cut, then one server event per
                      generated chunk}],
     "seqgen": {"seed", "events", "prefix", "cut", ...}}

A generated chunk is {'s': frame, 'after': bytes the client sent by the cut, 'data': hex}, as in a
recording: due once the client has sent that many bytes and as many frames have passed since. Two
keys go past the recording format, read by seqfuzz.FuzzReplay:
    'wait': [n, frame]   due (frame - that frame) frames after the client's n-th CZ_NOTIFY_ACTORINIT
                         (0x007d) since the cut, instead of after the byte count: the server sends
                         nothing on a new map before the client's map-loaded packet.
    'state': name        no data: a state probe taken when the event is due.

Only sequences the server can send are made (World.apply refuses the rest): an id is entered only
while the client does not list it, except for an actor's refresh (clif_spawn and clif_refresh send
an entry for a unit the client already shows); a monster that died comes back only by its respawn
(the same block id, mob_spawn) at least RESPAWN_FRAMES later; a player that logged out never comes
back; ground units and items leave either out of sight (and may come back under the same id) or
for good; hits name a source and a target the client lists (Ice Walls are the only ground units
with HP). A map change (ZC_NPCACK_MAPMOVE) drops everything the client lists; once it has loaded
the map the server sends the map-entry burst and the entries of everything in view there.
"""
import json
import os
import random
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
RECORDINGS = os.path.join(HERE, 'recordings')
FRAME_MS = 16

# ---- packet layouts ----------------------------------------------------------------------------
# name -> (packet id, [(field, struct format)]), little-endian and packed. A struct's name is
# Hercules' (src/map/packets_struct.h); a packet clif.c writes with WBUF* has its ZC_ name and the
# fields of clif.c's comment. A 'PacketLength' field is filled with the packet's size.

_UNIT_HEAD = [('PacketType', 'H'), ('PacketLength', 'h'), ('objecttype', 'B'), ('AID', 'I'), ('GID', 'I'),
              ('speed', 'h'), ('bodyState', 'h'), ('healthState', 'h'), ('effectState', 'i'), ('job', 'h'),
              ('head', 'H'), ('weapon', 'I'), ('shield', 'I'), ('accessory', 'H')]
_UNIT_LOOK = [('accessory2', 'H'), ('accessory3', 'H'), ('headpalette', 'h'), ('bodypalette', 'h'),
              ('headDir', 'h'), ('robe', 'H'), ('GUID', 'I'), ('GEmblemVer', 'h'), ('honor', 'h'),
              ('virtue', 'i'), ('isPKModeON', 'B'), ('sex', 'B')]
_UNIT_TAIL = [('clevel', 'h'), ('font', 'h'), ('maxHP', 'i'), ('HP', 'i'), ('isBoss', 'B')]

LAYOUTS = {
    'packet_idle_unit': (0x09ff, _UNIT_HEAD + _UNIT_LOOK + [('PosDir', '3s'), ('xSize', 'B'), ('ySize', 'B'),
                                                             ('state', 'B')] + _UNIT_TAIL
                         + [('body', 'H'), ('name', '24s')]),
    'packet_spawn_unit': (0x09fe, _UNIT_HEAD + _UNIT_LOOK + [('PosDir', '3s'), ('xSize', 'B'), ('ySize', 'B')]
                          + _UNIT_TAIL + [('body', 'h'), ('name', '24s')]),
    'packet_unit_walking': (0x09fd, _UNIT_HEAD + [('moveStartTime', 'I')] + _UNIT_LOOK
                            + [('MoveData', '6s'), ('xSize', 'B'), ('ySize', 'B')] + _UNIT_TAIL
                            + [('body', 'H'), ('name', '24s')]),
    'packet_skill_entry': (0x09ca, [('PacketType', 'H'), ('PacketLength', 'h'), ('AID', 'I'), ('creatorAID', 'I'),
                                    ('xPos', 'h'), ('yPos', 'h'), ('job', 'i'), ('RadiusRange', 'b'),
                                    ('isVisible', 'B'), ('level', 'B')]),
    'packet_dropflooritem': (0x0add, [('PacketType', 'H'), ('ITAID', 'I'), ('ITID', 'I'), ('type', 'H'),
                                      ('IsIdentified', 'B'), ('xPos', 'h'), ('yPos', 'h'), ('subX', 'B'),
                                      ('subY', 'B'), ('count', 'h'), ('showdropeffect', 'b'),
                                      ('dropeffectmode', 'h')]),
    'PACKET_ZC_ITEM_ENTRY': (0x009d, [('packetType', 'H'), ('AID', 'I'), ('itemId', 'I'), ('identify', 'B'),
                                      ('x', 'H'), ('y', 'H'), ('amount', 'H'), ('subX', 'B'), ('subY', 'B')]),
    'packet_damage': (0x08c8, [('PacketType', 'H'), ('GID', 'I'), ('targetGID', 'I'), ('startTime', 'I'),
                               ('attackMT', 'i'), ('attackedMT', 'i'), ('damage', 'i'), ('is_sp_damaged', 'B'),
                               ('count', 'h'), ('action', 'B'), ('leftDamage', 'i')]),
    'PACKET_ZC_NOTIFY_SKILL': (0x01de, [('PacketType', 'H'), ('SKID', 'H'), ('AID', 'I'), ('targetID', 'I'),
                                        ('startTime', 'I'), ('attackMT', 'i'), ('attackedMT', 'i'),
                                        ('damage', 'i'), ('level', 'h'), ('count', 'h'), ('action', 'b')]),
    'PACKET_ZC_USE_SKILL': (0x09cb, [('PacketType', 'H'), ('SKID', 'H'), ('level', 'i'), ('targetAID', 'I'),
                                     ('srcAID', 'I'), ('result', 'b')]),
    'PACKET_ZC_NOTIFY_GROUNDSKILL': (0x0117, [('PacketType', 'H'), ('SKID', 'H'), ('AID', 'I'), ('level', 'h'),
                                              ('xPos', 'h'), ('yPos', 'h'), ('startTime', 'I')]),
    'PACKET_ZC_USESKILL_ACK': (0x0b1a, [('packetType', 'H'), ('srcId', 'I'), ('dstId', 'I'), ('x', 'H'),
                                        ('y', 'H'), ('skillId', 'H'), ('element', 'I'), ('delayTime', 'I'),
                                        ('disposable', 'B'), ('attackMT', 'I')]),
    'packet_status_change': (0x0983, [('PacketType', 'H'), ('index', 'h'), ('AID', 'I'), ('state', 'B'),
                                      ('Total', 'I'), ('Left', 'I'), ('val1', 'i'), ('val2', 'i'),
                                      ('val3', 'i')]),
    'packet_status_change_end': (0x0196, [('PacketType', 'H'), ('index', 'h'), ('AID', 'I'), ('state', 'B')]),
    'PACKET_ZC_STATE_CHANGE': (0x0229, [('packetType', 'H'), ('AID', 'I'), ('bodyState', 'h'),
                                        ('healthState', 'h'), ('effectState', 'i'), ('isPKModeON', 'b')]),
    'PACKET_ZC_WALK_STYLE': (0x0b8b, [('PacketType', 'H'), ('PacketLength', 'h'), ('aid', 'I'),
                                      ('alt_path', 'B')]),
    # clif.c's WBUF packets
    'ZC_NOTIFY_TIME': (0x007f, [('PacketType', 'H'), ('time', 'I')]),
    'ZC_NOTIFY_VANISH': (0x0080, [('PacketType', 'H'), ('AID', 'I'), ('type', 'B')]),
    'ZC_NOTIFY_MOVE': (0x0086, [('PacketType', 'H'), ('AID', 'I'), ('MoveData', '6s'), ('moveStartTime', 'I')]),
    'ZC_NOTIFY_PLAYERMOVE': (0x0087, [('PacketType', 'H'), ('moveStartTime', 'I'), ('MoveData', '6s')]),
    'ZC_STOPMOVE': (0x0088, [('PacketType', 'H'), ('AID', 'I'), ('x', 'h'), ('y', 'h')]),
    'ZC_NPCACK_MAPMOVE': (0x0091, [('PacketType', 'H'), ('mapName', '16s'), ('x', 'h'), ('y', 'h')]),
    'ZC_ITEM_DISAPPEAR': (0x00a1, [('PacketType', 'H'), ('AID', 'I')]),
    'ZC_SKILL_DISAPPEAR': (0x0120, [('PacketType', 'H'), ('AID', 'I')]),
    'ZC_RESURRECTION': (0x0148, [('PacketType', 'H'), ('AID', 'I'), ('type', 'h')]),
    'ZC_UPDATE_MAPINFO': (0x0192, [('PacketType', 'H'), ('x', 'h'), ('y', 'h'), ('type', 'h'), ('mapName', '16s')]),
}

# The last field of a variable packet: bytes of any length after the fixed part (a struct's flexible
# array, or the text clif.c copies after its fields).
TAIL = '*'

_SLOT = [(f'slot.card[{i}]', 'I') for i in range(4)]                                       # struct EQUIPSLOTINFO
_OPTIONS = [(f'option_data[{i}].{k}', f) for i in range(5) for k, f in (('index', 'h'), ('value', 'h'), ('param', 'B'))]
_ROOM = [('PacketType', 'H'), ('PacketLength', 'h'), ('ownerAID', 'I'), ('roomId', 'I'), ('limit', 'h'),
         ('users', 'h'), ('type', 'B'), ('title', TAIL)]

# The packets seqtarget.py aims at handlers the parity scenarios never reach (seqfuzz.py target):
# Hercules' struct where clif.c fills one (the struct's name and field names), else the ZC_ name and
# the fields clif.c writes with WBUF*/WFIFO*, in its order.
LAYOUTS.update({
    # chat rooms (clif_createchat, clif_dispchat, clif_clearchat, clif_joinchatfail, clif_joinchatok,
    # clif_addchat, clif_leavechat, clif_changechatstatus, clif_chat_role)
    'ZC_ACK_CREATE_CHATROOM': (0x00d6, [('PacketType', 'H'), ('result', 'B')]),
    'ZC_ROOM_NEWENTRY': (0x00d7, _ROOM),
    'ZC_DESTROY_ROOM': (0x00d8, [('PacketType', 'H'), ('roomId', 'I')]),
    'ZC_REFUSE_ENTER_ROOM': (0x00da, [('PacketType', 'H'), ('result', 'B')]),
    'ZC_ENTER_ROOM': (0x00db, [('PacketType', 'H'), ('PacketLength', 'h'), ('roomId', 'I'), ('members', TAIL)]),
    'ZC_MEMBER_NEWENTRY': (0x00dc, [('PacketType', 'H'), ('users', 'h'), ('name', '24s')]),
    'ZC_MEMBER_EXIT': (0x00dd, [('PacketType', 'H'), ('users', 'h'), ('name', '24s'), ('flag', 'B')]),
    'ZC_CHANGE_CHATROOM': (0x00df, _ROOM),
    'PACKET_ZC_ROLE_CHANGE': (0x00e1, [('packetType', 'H'), ('flag', 'i'), ('name', '24s')]),
    'ZC_DISAPPEAR_ENTRY': (0x0132, [('PacketType', 'H'), ('AID', 'I')]),
    # trade (clif_traderequest, clif_tradestart, clif_tradeitemok, clif_tradeadditem, clif_tradedeal_lock,
    # clif_tradecancelled, clif_tradecompleted, clif_tradeundo)
    'ZC_REQ_EXCHANGE_ITEM2': (0x01f4, [('PacketType', 'H'), ('name', '24s'), ('charId', 'I'), ('level', 'h')]),
    'ZC_ACK_EXCHANGE_ITEM2': (0x01f5, [('PacketType', 'H'), ('result', 'B'), ('charId', 'I'), ('level', 'h')]),
    'ZC_ACK_ADD_EXCHANGE_ITEM': (0x00ea, [('PacketType', 'H'), ('index', 'h'), ('result', 'B')]),
    'PACKET_ZC_ADD_EXCHANGE_ITEM': (0x0b42, [('packetType', 'H'), ('itemId', 'I'), ('itemType', 'B'), ('amount', 'i'),
                                             ('identified', 'B'), ('damaged', 'B')] + _SLOT + _OPTIONS
                                    + [('location', 'I'), ('look', 'H'), ('refine', 'B'), ('grade', 'B')]),
    'ZC_CONCLUDE_EXCHANGE_ITEM': (0x00ec, [('PacketType', 'H'), ('who', 'B')]),
    'ZC_CANCEL_EXCHANGE_ITEM': (0x00ee, [('PacketType', 'H')]),
    'ZC_EXEC_EXCHANGE_ITEM': (0x00f0, [('PacketType', 'H'), ('result', 'B')]),
    'ZC_EXCHANGEITEM_UNDO': (0x00f1, [('PacketType', 'H')]),
    # friends (clif_friendslist_send, clif_friendslist_toggle, clif_friendlist_req, clif_friendslist_reqack)
    'ZC_FRIENDS_LIST': (0x0201, [('PacketType', 'H'), ('PacketLength', 'h'), ('friends', TAIL)]),
    'ZC_FRIENDS_STATE': (0x0206, [('PacketType', 'H'), ('AID', 'I'), ('GID', 'I'), ('state', 'B'), ('name', '24s')]),
    'ZC_REQ_ADD_FRIENDS': (0x0207, [('PacketType', 'H'), ('AID', 'I'), ('GID', 'I'), ('name', '24s')]),
    'ZC_ADD_FRIENDS': (0x0209, [('PacketType', 'H'), ('result', 'h'), ('AID', 'I'), ('GID', 'I'), ('name', '24s')]),
    'ZC_DELETE_FRIENDS': (0x020a, [('PacketType', 'H'), ('AID', 'I'), ('GID', 'I')]),
    # party (clif_party_info, clif_party_withdraw, clif_PartyLeaderChanged, clif_party_message,
    # clif_party_invite, clif_party_job_and_level, clif_party_dead_notification)
    'PACKET_ZC_GROUP_LIST': (0x0ae5, [('packetType', 'H'), ('packetLen', 'h'), ('partyName', '24s'), ('members', TAIL)]),
    'PACKET_ZC_DELETE_MEMBER_FROM_GROUP': (0x0105, [('PacketType', 'h'), ('AID', 'i'), ('characterName', '24s'),
                                                    ('result', 'b')]),
    'packet_party_leader_changed': (0x07fc, [('PacketType', 'h'), ('prev_leader_aid', 'I'), ('new_leader_aid', 'I')]),
    'PACKET_ZC_NOTIFY_CHAT_PARTY': (0x0109, [('PacketType', 'h'), ('PacketLength', 'h'), ('AID', 'i'),
                                             ('chatMsg', TAIL)]),
    'PACKET_ZC_PARTY_JOIN_REQ': (0x02c6, [('PacketType', 'h'), ('GRID', 'i'), ('groupName', '24s')]),
    'PACKET_ZC_NOTIFY_MEMBERINFO_TO_GROUPM': (0x0abd, [('PacketType', 'h'), ('AID', 'i'), ('job', 'h'), ('level', 'h')]),
    'PACKET_ZC_GROUP_ISALIVE': (0x0ab2, [('packetType', 'h'), ('AID', 'I'), ('isDead', 'B')]),
    'PACKET_ZC_ITEM_PICKUP_PARTY': (0x0b67, [('packetType', 'h'), ('AID', 'I'), ('itemId', 'I'), ('identified', 'B'),
                                             ('damaged', 'B')] + _SLOT
                                    + [('location', 'H'), ('itemType', 'B'), ('refine', 'B'), ('grade', 'B')]),
    # messages (clif_msgtable, clif_msgtable_num, clif_msgtable_skill, clif_msgtable_color, clif_msgtable_str,
    # clif_msgtable_str_color, clif_broadcast2, clif_mvp_item, clif_mvp_noitem, clif_user_count, clif_font,
    # clif_movetoattack, clif_showdigit, clif_ShowScript, clif_manner_message, clif_GM_silence)
    'ZC_MSG': (0x0291, [('PacketType', 'H'), ('msgId', 'H')]),
    'ZC_MSG_VALUE': (0x07e2, [('PacketType', 'H'), ('msgId', 'H'), ('value', 'i')]),
    'ZC_MSG_SKILL': (0x07e6, [('PacketType', 'H'), ('skillId', 'H'), ('msgId', 'i')]),
    'PACKET_ZC_MSG_COLOR': (0x09cd, [('PacketType', 'H'), ('MessageId', 'H'), ('MessageColor', 'I')]),
    'PACKET_ZC_FORMATSTRING_MSG': (0x02c2, [('PacketType', 'H'), ('PacketLength', 'H'), ('MessageId', 'H'),
                                            ('MessageString', TAIL)]),
    'PACKET_ZC_FORMATSTRING_MSG_COLOR': (0x0a6f, [('PacketType', 'H'), ('PacketLength', 'H'), ('messageId', 'H'),
                                                  ('color', 'I'), ('messageString', TAIL)]),
    'ZC_BROADCAST2': (0x01c3, [('PacketType', 'H'), ('PacketLength', 'h'), ('fontColor', 'I'), ('fontType', 'h'),
                               ('fontSize', 'h'), ('fontAlign', 'h'), ('fontY', 'h'), ('message', TAIL)]),
    'PACKET_ZC_MVP_GETTING_ITEM': (0x010a, [('packetType', 'h'), ('itemId', 'I')]),
    'ZC_THROW_MVPITEM': (0x010d, [('PacketType', 'H')]),
    'ZC_USER_COUNT': (0x00c2, [('PacketType', 'H'), ('count', 'i')]),
    'ZC_NOTIFY_FONT': (0x02ef, [('PacketType', 'H'), ('AID', 'I'), ('font', 'h')]),
    'PACKET_ZC_ATTACK_FAILURE_FOR_DISTANCE': (0x0139, [('PacketType', 'h'), ('targetAID', 'I'), ('targetXPos', 'h'),
                                                       ('targetYPos', 'h'), ('xPos', 'h'), ('yPos', 'h'),
                                                       ('currentAttRange', 'h')]),
    'PACKET_ZC_DEBUGMSG': (0x0adb, [('packetType', 'h'), ('packetLength', 'h'), ('color', 'i'), ('message', TAIL)]),
    'ZC_SHOWDIGIT': (0x01b1, [('PacketType', 'H'), ('type', 'B'), ('value', 'i')]),
    'ZC_SHOWSCRIPT': (0x08b3, [('PacketType', 'H'), ('PacketLength', 'h'), ('AID', 'I'), ('message', TAIL)]),
    'ZC_ACK_GIVE_MANNER_POINT': (0x014a, [('PacketType', 'H'), ('type', 'I')]),
    'ZC_NOTIFY_MANNER_POINT_GIVEN': (0x014b, [('PacketType', 'H'), ('type', 'B'), ('name', '24s')]),
    'PACKET_ZC_BROADCASTING_SPECIAL_ITEM_OBTAIN_item': (0x07fd, [('PacketType', 'h'), ('PacketLength', 'h'),
                                                                 ('type', 'B'), ('ItemID', 'I'), ('len', 'b'),
                                                                 ('Name', '24s'), ('boxItemID_len', 'b'),
                                                                 ('BoxItemID', 'I')]),
    # skills (clif_addskill, clif_skillunit_update, clif_item_skill, clif_spiritball, clif_graffiti_entry,
    # clif_devotion, clif_starskill, clif_skill_select_request, clif_autospell, clif_arrow_create_list,
    # clif_skill_produce_mix_list, clif_cooking_list, clif_item_refine_list, clif_upgrademessage,
    # clif_produceeffect, clif_use_card, clif_skill_warppoint, clif_skill_itemlistwindow, clif_changeoption2,
    # clif_skill_estimation, clif_bossmapinfo, clif_snap, clif_pet_roulette's capture pair)
    'PACKET_ZC_ADD_SKILL': (0x0111, [('packetType', 'h'), ('skill.id', 'H'), ('skill.inf', 'i'), ('skill.level', 'H'),
                                     ('skill.sp', 'H'), ('skill.range2', 'H'), ('skill.name', '24s'),
                                     ('skill.upFlag', 'B')]),
    'ZC_SKILL_UPDATE': (0x01ac, [('PacketType', 'H'), ('AID', 'I')]),
    'PACKET_ZC_AUTORUN_SKILL': (0x0147, [('packetType', 'h'), ('skill_id', 'H'), ('skill_type', 'I'),
                                         ('skill_lv', 'H'), ('skill_sp', 'H'), ('skill_range', 'H'),
                                         ('skill_name', '24s'), ('up_flag', 'b')]),
    'PACKET_ZC_SPIRITS': (0x01d0, [('PacketType', 'h'), ('AID', 'I'), ('num', 'h')]),
    'packet_graffiti_entry': (0x01c9, [('PacketType', 'h'), ('AID', 'I'), ('creatorAID', 'I'), ('xPos', 'h'),
                                       ('yPos', 'h'), ('job', 'B'), ('isVisible', 'B'), ('isContens', 'B'),
                                       ('msg', '80s')]),
    'ZC_DEVOTIONLIST': (0x01cf, [('PacketType', 'H'), ('AID', 'I'), ('dst0', 'I'), ('dst1', 'I'), ('dst2', 'I'),
                                 ('dst3', 'I'), ('dst4', 'I'), ('range', 'h')]),
    'ZC_STARSKILL': (0x020e, [('PacketType', 'H'), ('mapName', '24s'), ('monsterId', 'i'), ('star', 'B'),
                              ('result', 'B')]),
    'PACKET_ZC_SKILL_SELECT_REQUEST': (0x0442, [('packetType', 'h'), ('packetLength', 'h'), ('flag', 'i'),
                                                ('skillIds', TAIL)]),
    'PACKET_ZC_AUTOSPELLLIST': (0x0afb, [('packetType', 'h'), ('packetLength', 'h'), ('skills', TAIL)]),
    'PACKET_ZC_MAKINGARROW_LIST': (0x01ad, [('packetType', 'h'), ('packetLength', 'h'), ('items', TAIL)]),
    'PACKET_ZC_MAKABLEITEMLIST': (0x018d, [('packetType', 'h'), ('packetLength', 'h'), ('items', TAIL)]),
    'PACKET_ZC_MAKINGITEM_LIST': (0x025a, [('packetType', 'h'), ('packetLength', 'h'), ('makeItem', 'H'),
                                           ('items', TAIL)]),
    'PACKET_ZC_NOTIFY_WEAPONITEMLIST': (0x0221, [('packetType', 'h'), ('packetLength', 'h'), ('items', TAIL)]),
    'PACKET_ZC_ACK_WEAPONREFINE': (0x0223, [('packetType', 'h'), ('result', 'i'), ('itemId', 'I')]),
    'PACKET_ZC_ACK_REQMAKINGITEM': (0x018f, [('packetType', 'h'), ('result', 'h'), ('itemId', 'I')]),
    'ZC_ITEMCOMPOSITION_LIST': (0x017b, [('PacketType', 'H'), ('PacketLength', 'h'), ('indexes', TAIL)]),
    'PACKET_ZC_WARPLIST': (0x0abe, [('packetType', 'h'), ('packetLength', 'h'), ('skillId', 'H'), ('maps', TAIL)]),
    # clif.c writes the skill level as a long at 2 and a zero long at 4 into a 6-byte packet
    'ZC_ITEMLISTWIN_OPEN': (0x07e3, [('PacketType', 'H'), ('type', 'h'), ('zero', 'h')]),
    'ZC_NPC_SHOWEFST_UPDATE': (0x028a, [('PacketType', 'H'), ('AID', 'I'), ('option', 'I'), ('level', 'I'),
                                        ('opt3', 'I')]),
    'ZC_MONSTER_INFO': (0x018c, [('PacketType', 'H'), ('job', 'h'), ('level', 'h'), ('size', 'h'), ('hp', 'I'),
                                 ('def', 'h'), ('race', 'h'), ('mdef', 'h'), ('element', 'h'), ('ratios', '9s')]),
    'ZC_BOSS_INFO': (0x0293, [('PacketType', 'H'), ('flag', 'B'), ('x', 'I'), ('y', 'I'), ('hours', 'H'),
                              ('minutes', 'H'), ('unused', '4s'), ('name', '24s'), ('rest', '27s')]),
    'ZC_FASTMOVE': (0x08d2, [('PacketType', 'H'), ('AID', 'I'), ('x', 'h'), ('y', 'h')]),
    'PACKET_ZC_START_CAPTURE': (0x019e, [('PacketType', 'h')]),
    'PACKET_ZC_TRYCAPTURE_MONSTER': (0x01a0, [('PacketType', 'h'), ('result', 'b')]),
    'ZC_SKILL_POSTDELAY_LIST2': (0x0985, [('PacketType', 'H'), ('PacketLength', 'h'), ('skills', TAIL)]),
    # quests (clif_quest_delete, clif_quest_update_status, clif_quest_add, clif_quest_send_list,
    # clif_quest_update_objective, clif_quest_show_event)
    'ZC_DEL_QUEST': (0x02b4, [('PacketType', 'H'), ('questID', 'I')]),
    'ZC_ACTIVE_QUEST': (0x02b7, [('PacketType', 'H'), ('questID', 'I'), ('active', 'B')]),
    # struct packet_quest_add_header with its three objectives (struct packet_quest_hunt_sub, unused ones zero)
    'ZC_ADD_QUEST_EX2': (0x0b0c, [('PacketType', 'H'), ('questID', 'I'), ('active', 'B'), ('quest_svrTime', 'i'),
                                  ('quest_endTime', 'i'), ('count', 'h'), ('objectives', '138s')]),
    'packet_quest_list_header': (0x0aff, [('PacketType', 'H'), ('PacketLength', 'H'), ('questCount', 'i'),
                                          ('quests', TAIL)]),
    'packet_quest_update_header': (0x0afe, [('PacketType', 'H'), ('PacketLength', 'H'), ('count', 'h'),
                                            ('objectives', TAIL)]),
    'ZC_QUEST_NOTIFY_EFFECT': (0x0446, [('PacketType', 'H'), ('AID', 'I'), ('x', 'h'), ('y', 'h'), ('state', 'h'),
                                        ('color', 'h')]),
    # NPC scripts (clif_scriptclear, clif_playBGM, clif_soundeffect, clif_progressbar, clif_progressbar_abort,
    # clif_scriptdialogalign, clif_readbook, clif_navigate_to)
    'packet_script_clear': (0x08d6, [('PacketType', 'h'), ('NpcID', 'I')]),
    'PACKET_ZC_PLAY_NPC_BGM': (0x07fe, [('PacketType', 'h'), ('bgm', '24s')]),
    'PACKET_ZC_SOUND': (0x01d3, [('PacketType', 'h'), ('name', '24s'), ('act', 'B'), ('term', 'I'), ('AID', 'I')]),
    'ZC_PROGRESS': (0x02f0, [('PacketType', 'H'), ('color', 'I'), ('seconds', 'I')]),
    'ZC_PROGRESS_CANCEL': (0x02f2, [('PacketType', 'H')]),
    'PACKET_ZC_DIALOG_TEXT_ALIGN': (0x0ba1, [('PacketType', 'h'), ('align', 'B')]),
    'ZC_READ_BOOK': (0x0294, [('PacketType', 'H'), ('bookId', 'I'), ('page', 'I')]),
    'ZC_NAVIGATION_ACTIVE': (0x08e2, [('PacketType', 'H'), ('type', 'B'), ('flag', 'B'), ('hide', 'B'),
                                      ('mapName', '16s'), ('x', 'H'), ('y', 'H'), ('mobId', 'H')]),
    # items (clif_item_damaged, clif_cart_additem_ack, clif_cart_additem, clif_selectcart, clif_update_cardslot,
    # clif_takeoff_equip_all_ack, clif_unequipitemack, clif_favorite_item, clif_notify_bounditem,
    # clif_extend_inventory_*, clif_move_item_failed, clif_cash time counters)
    'ZC_EQUIPITEM_DAMAGED': (0x02bb, [('PacketType', 'H'), ('position', 'H'), ('AID', 'I')]),
    'packet_cart_additem_ack': (0x012c, [('PacketType', 'h'), ('result', 'b')]),
    'PACKET_ZC_ADD_ITEM_TO_CART': (0x0b45, [('packetType', 'h'), ('index', 'h'), ('amount', 'i'), ('itemId', 'I'),
                                            ('itemType', 'B'), ('identified', 'B'), ('damaged', 'B')] + _SLOT
                                   + _OPTIONS + [('refine', 'B'), ('grade', 'B')]),
    'ZC_SELECTCART': (0x097f, [('PacketType', 'H'), ('PacketLength', 'h'), ('AID', 'I'), ('carts', TAIL)]),
    'PACKET_ZC_UPDATE_CARDSLOT': (0x0a3f, [('packetType', 'h'), ('wearState', 'h'), ('cardSlot', 'h'),
                                           ('itemId', 'i'), ('equipFlag', 'b')]),
    'PACKET_ZC_ACK_TAKEOFF_EQUIP_ALL': (0x0bae, [('PacketType', 'h'), ('result', 'B')]),
    'packet_unequipitem_ack': (0x099a, [('PacketType', 'h'), ('index', 'H'), ('wearLocation', 'I'), ('result', 'B')]),
    'ZC_INVENTORY_TAB': (0x0908, [('PacketType', 'H'), ('index', 'h'), ('favorite', 'B')]),
    'packet_notify_bounditem': (0x02d3, [('PacketType', 'h'), ('index', 'H')]),
    'PACKET_ZC_ACK_OPEN_MSGBOX_EXTEND_BODYITEM_SIZE': (0x0b15, [('packetType', 'h'), ('result', 'B'), ('itemId', 'I')]),
    'PACKET_ZC_ACK_EXTEND_BODYITEM_SIZE': (0x0b17, [('packetType', 'h'), ('result', 'B')]),
    'PACKET_ZC_EXTEND_BODYITEM_SIZE': (0x0b18, [('packetType', 'h'), ('expansionSize', 'h')]),
    'PACKET_ZC_MOVE_ITEM_FAILED': (0x0aa7, [('packetType', 'h'), ('itemIndex', 'h'), ('itemCount', 'h')]),
    'PACKET_ZC_CASH_TIME_COUNTER': (0x0298, [('packetType', 'h'), ('itemId', 'I'), ('seconds', 'I')]),
    'PACKET_ZC_CASH_ITEM_DELETE': (0x0299, [('packetType', 'h'), ('index', 'H'), ('itemId', 'I')]),
    # marriage and adoption (clif_callpartner, clif_wedding_effect, clif_Adopt_request, clif_Adopt_reply)
    'ZC_COUPLENAME': (0x01e6, [('PacketType', 'H'), ('name', '24s')]),
    'ZC_CONGRATULATION': (0x01ea, [('PacketType', 'H'), ('AID', 'I')]),
    'ZC_REQ_BABY': (0x01f6, [('PacketType', 'H'), ('AID', 'I'), ('partyId', 'I'), ('name', '24s')]),
    'ZC_BABYMSG': (0x0216, [('PacketType', 'H'), ('type', 'i')]),
    # rankings (clif_pvpset, clif_update_rankingpoint)
    'ZC_NOTIFY_RANKING': (0x019a, [('PacketType', 'H'), ('AID', 'I'), ('rank', 'I'), ('total', 'I')]),
    'ZC_UPDATE_RANKING_POINT': (0x097e, [('PacketType', 'H'), ('type', 'h'), ('points', 'i'), ('total', 'i')]),
    # memorial dungeons (clif_instance, clif_instance_join, clif_instance_leave)
    'ZC_MEMORIALDUNGEON_SUBSCRIPTION_INFO': (0x02cb, [('PacketType', 'H'), ('name', '61s'), ('flag', 'h')]),
    'ZC_MEMORIALDUNGEON_SUBSCRIPTION_NOTIFY': (0x02cc, [('PacketType', 'H'), ('flag', 'h')]),
    'ZC_MEMORIALDUNGEON_INFO': (0x02cd, [('PacketType', 'H'), ('name', '61s'), ('progress', 'I'), ('idle', 'I')]),
    'ZC_MEMORIALDUNGEON_NOTIFY': (0x02ce, [('PacketType', 'H'), ('flag', 'I'), ('enterLimitDate', 'I')]),
    # buying stores and store search (clif_buyingstore_*, clif_search_store_info_*, clif_buyvending)
    'PACKET_ZC_BUYING_STORE_ENTRY': (0x0814, [('packetType', 'h'), ('makerAID', 'I'), ('storeName', '80s')]),
    'PACKET_ZC_DISAPPEAR_BUYING_STORE_ENTRY': (0x0816, [('packetType', 'h'), ('makerAID', 'I')]),
    'PACKET_ZC_ACK_ITEMLIST_BUYING_STORE': (0x0818, [('packetType', 'h'), ('packetLength', 'h'), ('AID', 'I'),
                                                     ('storeId', 'I'), ('zenyLimit', 'I'), ('items', TAIL)]),
    'ZC_FAILED_OPEN_BUYING_STORE_TO_BUYER': (0x0812, [('PacketType', 'H'), ('result', 'H'), ('weight', 'I')]),
    'ZC_FAILED_TRADE_BUYING_STORE_TO_BUYER': (0x081a, [('PacketType', 'H'), ('result', 'h')]),
    'ZC_ITEM_DELETE_BUYING_STORE': (0x081c, [('PacketType', 'H'), ('index', 'h'), ('amount', 'H'), ('price', 'i')]),
    'PACKET_ZC_FAILED_TRADE_BUYING_STORE_TO_SELLER': (0x0824, [('packetType', 'h'), ('result', 'H'), ('itemId', 'I')]),
    'PACKET_ZC_UPDATE_ITEM_FROM_BUYING_STORE': (0x09e6, [('packetType', 'h'), ('itemId', 'I'), ('amount', 'H'),
                                                         ('zeny', 'I'), ('zenyLimit', 'I'), ('charId', 'I'),
                                                         ('updateTime', 'I')]),
    'ZC_SEARCH_STORE_INFO_FAILED': (0x0837, [('PacketType', 'H'), ('reason', 'B')]),
    'ZC_SSILIST_ITEM_CLICK_ACK': (0x083d, [('PacketType', 'H'), ('x', 'h'), ('y', 'h')]),
    'ZC_PC_PURCHASE_RESULT_FROMMC': (0x0135, [('PacketType', 'H'), ('index', 'h'), ('amount', 'h'), ('result', 'B')]),
    # the cash shop (clif_cashshop_show, clif_cashshop_ack, clif_cashShopBuyAck)
    'PACKET_ZC_PC_CASH_POINT_ITEMLIST': (0x0287, [('packetType', 'h'), ('packetLength', 'h'), ('cashPoints', 'I'),
                                                  ('kafraPoints', 'I'), ('items', TAIL)]),
    'ZC_PC_CASH_POINT_UPDATE': (0x0289, [('PacketType', 'H'), ('cashPoints', 'I'), ('kafraPoints', 'I'),
                                         ('error', 'H')]),
    'PACKET_ZC_SE_PC_BUY_CASHITEM_RESULT': (0x0849, [('packetType', 'h'), ('itemId', 'I'), ('result', 'H'),
                                                     ('cashPoints', 'I'), ('kafraPoints', 'I')]),
    'PACKET_ZC_SE_CASHSHOP_OPEN': (0x0b6e, [('packetType', 'h'), ('cashPoints', 'I'), ('kafraPoints', 'I'),
                                            ('tab', 'I')]),
    # item skills and NPC services (clif_item_identify_list, clif_item_identified, clif_insert_card,
    # clif_refine, clif_item_repaireffect, clif_item_repair_list, clif_merge_item_open, clif_ackmergeitems,
    # clif_overweight_percent, clif_cart_delitem, clif_vendingreport)
    'ZC_ITEMIDENTIFY_LIST': (0x0177, [('PacketType', 'H'), ('PacketLength', 'h'), ('indexes', TAIL)]),
    'ZC_ACK_ITEMIDENTIFY': (0x0179, [('PacketType', 'H'), ('index', 'h'), ('result', 'B')]),
    'ZC_ACK_ITEMCOMPOSITION': (0x017d, [('PacketType', 'H'), ('equipIndex', 'h'), ('cardIndex', 'h'), ('result', 'B')]),
    'ZC_ACK_ITEMREFINING': (0x0188, [('PacketType', 'H'), ('result', 'h'), ('index', 'h'), ('refine', 'h')]),
    'ZC_ACK_ITEMREPAIR': (0x01fe, [('PacketType', 'H'), ('index', 'h'), ('result', 'B')]),
    'PACKET_ZC_REPAIRITEMLIST': (0x0b65, [('packetType', 'h'), ('packetLength', 'h'), ('items', TAIL)]),
    'PACKET_ZC_MERGE_ITEM_OPEN': (0x096d, [('packetType', 'h'), ('packetLen', 'H'), ('items', TAIL)]),
    'PACKET_ZC_ACK_MERGE_ITEM': (0x096f, [('packetType', 'h'), ('index', 'h'), ('amount', 'h'), ('reason', 'B')]),
    'PACKET_ZC_OVERWEIGHT_PERCENT': (0x0ade, [('packetType', 'h'), ('percent', 'I')]),
    'ZC_DELETE_ITEM_FROM_CART': (0x0125, [('PacketType', 'H'), ('index', 'h'), ('amount', 'i')]),
    'ZC_DELETEITEM_FROM_MCSTORE2': (0x09e5, [('PacketType', 'H'), ('index', 'h'), ('amount', 'h'), ('GID', 'I'),
                                             ('date', 'I'), ('zeny', 'i')]),
    'PACKET_ZC_BROADCAST_ITEMREFINING_RESULT': (0x0ada, [('packetType', 'h'), ('name', '24s'), ('itemId', 'I'),
                                                         ('refine_level', 'b'), ('status', 'b')]),
    # combat and skill notices (clif_bladestop, clif_combo_delay, clif_mvp_effect, clif_gospel_info,
    # clif_removeSpecialEffect, clif_talkiebox, clif_pet_emotion, clif_changestatus, clif_class_change)
    'ZC_BLADESTOP': (0x01d1, [('PacketType', 'H'), ('srcAID', 'I'), ('dstAID', 'I'), ('active', 'i')]),
    'ZC_COMBODELAY': (0x01d2, [('PacketType', 'H'), ('AID', 'I'), ('wait', 'i')]),
    'ZC_MVP': (0x010c, [('PacketType', 'H'), ('AID', 'I')]),
    'ZC_SKILLMSG': (0x0215, [('PacketType', 'H'), ('type', 'i')]),
    'PACKET_ZC_REMOVE_EFFECT': (0x0b0d, [('packetType', 'h'), ('aid', 'I'), ('effectId', 'I')]),
    'PACKET_ZC_TALKBOX_CHATCONTENTS': (0x0191, [('PacketType', 'h'), ('aid', 'I'), ('message', '21s')]),
    'ZC_PET_ACT': (0x01aa, [('PacketType', 'H'), ('AID', 'I'), ('param', 'i')]),
    'ZC_PAR_CHANGE_USER': (0x01ab, [('PacketType', 'H'), ('AID', 'I'), ('type', 'h'), ('value', 'i')]),
    'ZC_NPCSPRITE_CHANGE': (0x01b0, [('PacketType', 'H'), ('AID', 'I'), ('type', 'B'), ('job', 'i')]),
    # the map, NPC marks and the camera (clif_map_type, clif_viewpoint, clif_camerainfo, clif_ui_open)
    'ZC_NOTIFY_MAPPROPERTY2': (0x01d6, [('PacketType', 'H'), ('type', 'h')]),
    'ZC_COMPASS': (0x0144, [('PacketType', 'H'), ('npcId', 'I'), ('type', 'i'), ('x', 'i'), ('y', 'i'), ('id', 'B'),
                            ('color', 'I')]),
    'PACKET_ZC_VIEW_CAMERAINFO': (0x0a78, [('packetType', 'h'), ('action', 'b'), ('range', 'f'), ('rotation', 'f'),
                                           ('latitude', 'f')]),
    'PACKET_ZC_UI_OPEN': (0x0ae2, [('PacketType', 'h'), ('UIType', 'b'), ('data', 'i')]),
    'PACKET_ZC_UI_OPEN2': (0x0b9a, [('PacketType', 'h'), ('UIType', 'b'), ('data', 'q')]),
    'ZC_DRESSROOM_OPEN': (0x0a02, [('PacketType', 'H'), ('view', 'h')]),
    # whispers and names (clif_wisexin, clif_wisall, clif_PMIgnoreList, clif_solved_charname, clif_check)
    'ZC_SETTING_WHISPER_PC': (0x00d1, [('PacketType', 'H'), ('type', 'B'), ('result', 'B')]),
    'ZC_SETTING_WHISPER_STATE': (0x00d2, [('PacketType', 'H'), ('type', 'B'), ('result', 'B')]),
    'ZC_WHISPER_LIST': (0x00d4, [('PacketType', 'H'), ('PacketLength', 'h'), ('names', TAIL)]),
    'ZC_ACK_REQNAME_BYGID2': (0x0af7, [('PacketType', 'H'), ('flag', 'H'), ('GID', 'I'), ('name', '24s')]),
    'ZC_ACK_STATUS_GM': (0x0214, [('PacketType', 'H'), ('str', 'B'), ('needStr', 'B'), ('agi', 'B'), ('needAgi', 'B'),
                                  ('vit', 'B'), ('needVit', 'B'), ('int', 'B'), ('needInt', 'B'), ('dex', 'B'),
                                  ('needDex', 'B'), ('luk', 'B'), ('needLuk', 'B'), ('atk', 'h'), ('atk2', 'h'),
                                  ('matkMax', 'h'), ('matkMin', 'h'), ('def', 'h'), ('def2', 'h'), ('mdef', 'h'),
                                  ('mdef2', 'h'), ('hit', 'h'), ('flee', 'h'), ('flee2', 'h'), ('cri', 'h'),
                                  ('aspd', 'h'), ('plusAspd', 'h')]),
    # guilds (clif_guild_invite, clif_guild_inviteack, clif_guild_reqalliance, clif_guild_allianceack,
    # clif_guild_oppositionack, clif_guild_memberpositionchanged, clif_guild_memberlogin_notice, clif_guild_xy,
    # clif_guild_set_position)
    'ZC_REQ_JOIN_GUILD': (0x016a, [('PacketType', 'H'), ('guildId', 'I'), ('name', '24s')]),
    'ZC_ACK_REQ_JOIN_GUILD': (0x0169, [('PacketType', 'H'), ('result', 'B')]),
    'ZC_REQ_ALLY_GUILD': (0x0171, [('PacketType', 'H'), ('AID', 'I'), ('name', '24s')]),
    # clif.c writes the result as a long into a 3-byte packet: its low byte goes out
    'ZC_ACK_REQ_ALLY_GUILD': (0x0173, [('PacketType', 'H'), ('result', 'B')]),
    'ZC_ACK_REQ_HOSTILE_GUILD': (0x0181, [('PacketType', 'H'), ('result', 'B')]),
    'ZC_ACK_REQ_CHANGE_MEMBERS': (0x0156, [('PacketType', 'H'), ('PacketLength', 'h'), ('AID', 'I'), ('GID', 'I'),
                                           ('position', 'i'), ('rest', TAIL)]),
    'ZC_UPDATE_CHARSTAT2': (0x01f2, [('PacketType', 'H'), ('AID', 'I'), ('GID', 'I'), ('status', 'i'), ('sex', 'h'),
                                     ('hair', 'h'), ('hairColor', 'h')]),
    'ZC_NOTIFY_POSITION_TO_GUILDM': (0x01eb, [('PacketType', 'H'), ('AID', 'I'), ('x', 'h'), ('y', 'h')]),
    'PACKET_ZC_GUILD_POSITION': (0x0afd, [('packetType', 'h'), ('packetLength', 'h'), ('AID', 'I'),
                                          ('position', TAIL)]),
    # marriage (clif_marriage_proposal, clif_marriage_process, clif_divorced)
    'ZC_REQ_COUPLE': (0x01e2, [('PacketType', 'H'), ('AID', 'I'), ('GID', 'I'), ('name', '24s')]),
    'ZC_START_COUPLE': (0x01e4, [('PacketType', 'H')]),
    'ZC_DIVORCE': (0x0205, [('PacketType', 'H'), ('name', '24s')]),
    # the adventurer agency (clif_adventurerAgencyJoinReq, clif_adventurerAgencyResult)
    'PACKET_ZC_ADVENTURER_AGENCY_JOIN_REQ': (0x0ae7, [('packetType', 'h'), ('GRID', 'i'), ('AID', 'i'),
                                                      ('groupName', '24s'), ('level', 'h'), ('job', 'h')]),
    'PACKET_ZC_ADVENTURER_AGENCY_JOIN_RESULT': (0x0afa, [('packetType', 'h'), ('player_name', '24s'),
                                                         ('party_name', '24s'), ('AID', 'i'), ('result', 'i')]),
    # a whisper's failure, mail answers, guild members expelled and leaving, the bank, the stylist
    # (clif_wis_end, clif_rodex_*, clif_guild_expulsion, clif_guild_leave, clif_parse_BankWithdraw,
    # clif_style_change_response)
    'packet_wis_end': (0x09df, [('PacketType', 'h'), ('result', 'b'), ('AID', 'I')]),
    'PACKET_ZC_ACK_ZENY_FROM_MAIL': (0x09f2, [('PacketType', 'h'), ('MailID', 'q'), ('opentype', 'b'), ('result', 'b')]),
    'PACKET_ZC_ACK_ITEM_FROM_MAIL': (0x09f4, [('PacketType', 'h'), ('MailID', 'q'), ('opentype', 'b'), ('result', 'b')]),
    'PACKET_ZC_ACK_DELETE_MAIL': (0x09f6, [('PacketType', 'h'), ('opentype', 'b'), ('MailID', 'q')]),
    'PACKET_ZC_ACK_REMOVE_ITEM_MAIL': (0x0a07, [('PacketType', 'h'), ('result', 'b'), ('index', 'h'), ('cnt', 'H'),
                                                ('weight', 'h')]),
    'PACKET_ZC_ACK_BAN_GUILD3': (0x0a82, [('packetType', 'h'), ('reason', '40s'), ('GID', 'I')]),
    'PACKET_ZC_ACK_LEAVE_GUILD2': (0x0a83, [('packetType', 'h'), ('GID', 'I'), ('reason', '40s')]),
    'packet_banking_withdraw_ack': (0x09aa, [('PacketType', 'h'), ('Reason', 'h'), ('Money', 'q'), ('Balance', 'i')]),
    'PACKET_ZC_STYLE_CHANGE_RES': (0x0a47, [('PacketType', 'h'), ('flag', 'b')]),
    # guild positions and relations (clif_guild_positionchanged, clif_guild_delalliance,
    # clif_guild_broken)
    'ZC_ACK_CHANGE_GUILD_POSITIONINFO': (0x0174, [('PacketType', 'H'), ('PacketLength', 'h'), ('positionId', 'i'),
                                                  ('mode', 'i'), ('ranking', 'i'), ('payRate', 'i'), ('name', '24s'),
                                                  ('rest', TAIL)]),
    'ZC_DELETE_RELATED_GUILD': (0x0184, [('PacketType', 'H'), ('guildId', 'i'), ('relation', 'i')]),
    'ZC_ACK_DISORGANIZE_GUILD_RESULT': (0x015e, [('PacketType', 'H'), ('result', 'i')]),
    # scripts' extras (clif_progressbar_unit, clif_quest_update_objective's hunt info, clif_specialeffect,
    # clif_specialeffect_value, clif_inventory_item_option_update's option change)
    'ZC_PROGRESS_ACTOR': (0x09d1, [('PacketType', 'h'), ('GID', 'i'), ('color', 'i'), ('time', 'I')]),
    'packet_quest_hunt_info': (0x08fe, [('PacketType', 'H'), ('PacketLength', 'H'), ('info', TAIL)]),
    'ZC_NOTIFY_EFFECT2': (0x01f3, [('PacketType', 'H'), ('AID', 'I'), ('effectId', 'i')]),
    'PACKET_ZC_NOTIFY_EFFECT3': (0x0b69, [('packetType', 'h'), ('aid', 'I'), ('effectId', 'I'), ('num', 'Q')]),
    'PACKET_ZC_CHANGE_ITEM_OPTION': (0x0b43, [('packetType', 'h'), ('index', 'h'), ('isDamaged', 'b')] + _SLOT
                                     + _OPTIONS + [('refiningLevel', 'B'), ('grade', 'B')]),
    # NPC markets and barter shops (clif_npc_market_open, clif_npc_market_purchase_ack,
    # clif_npc_barter_open, clif_npc_expanded_barter_open)
    'PACKET_ZC_NPC_MARKET_OPEN': (0x0b7a, [('packetType', 'h'), ('packetLength', 'h'), ('list', TAIL)]),
    'PACKET_ZC_NPC_MARKET_PURCHASE_RESULT': (0x0b4e, [('PacketType', 'h'), ('PacketLength', 'h'), ('result', 'H'),
                                                      ('list', TAIL)]),
    'PACKET_ZC_NPC_BARTER_MARKET_ITEMINFO': (0x0b78, [('packetType', 'h'), ('packetLength', 'h'), ('list', TAIL)]),
    # the ranking window and guild castles (clif_ranklist, clif_agit_info, clif_agit_investment,
    # clif_agit_move)
    'PACKET_ZC_ACK_RANKING': (0x0af6, [('packetType', 'h'), ('rankType', 'h')] + [(f'chars[{i}]', 'I') for i in range(10)]
                              + [(f'points[{i}]', 'I') for i in range(10)] + [('myPoints', 'I')]),
    'PACKET_ZC_GUILD_AGIT_INFO': (0x0b27, [('packetType', 'h'), ('packetLength', 'h'), ('castle_list', TAIL)]),
    'PACKET_ZC_REQ_ACK_AGIT_INVESTMENT': (0x0b2d, [('packetType', 'h'), ('castle_id', 'b'), ('economy', 'i'),
                                                   ('defense', 'i')]),
    'PACKET_ZC_REQ_ACK_MOVE_GUILD_AGIT': (0x0b2e, [('packetType', 'h'), ('result', 'h')]),
    # the macro detector (macro.c through clif_macro_detector_request, clif_macro_detector_request_show,
    # clif_macro_detector_status, clif_macro_reporter_status, clif_captcha_preview_*, clif_captcha_upload_*)
    'PACKET_ZC_APPLY_MACRO_DETECTOR': (0x0a58, [('PacketType', 'h'), ('imageSize', 'H'), ('captchaKey', '4s')]),
    'PACKET_ZC_APPLY_MACRO_DETECTOR_CAPTCHA': (0x0a59, [('PacketType', 'h'), ('PacketLength', 'h'),
                                                        ('captchaKey', '4s'), ('imageData', TAIL)]),
    'PACKET_ZC_REQ_ANSWER_MACRO_DETECTOR': (0x0a5b, [('PacketType', 'h'), ('retryCount', 'B'), ('timeout', 'i')]),
    'PACKET_ZC_CLOSE_MACRO_DETECTOR': (0x0a5d, [('PacketType', 'h'), ('status', 'i')]),
    'PACKET_ZC_ACK_APPLY_MACRO_DETECTOR': (0x0a57, [('PacketType', 'h'), ('status', 'i')]),
    'PACKET_ZC_ACK_PREVIEW_MACRO_DETECTOR': (0x0a6a, [('PacketType', 'h'), ('captchaFlag', 'i'), ('imageSize', 'H'),
                                                      ('captchaKey', '4s')]),
    'PACKET_ZC_PREVIEW_MACRO_DETECTOR_CAPTCHA': (0x0a6b, [('PacketType', 'h'), ('PacketLength', 'h'),
                                                          ('captchaKey', '4s'), ('imageData', TAIL)]),
    'PACKET_ZC_ACK_UPLOAD_MACRO_DETECTOR': (0x0a53, [('PacketType', 'h'), ('captchaKey', '4s'), ('captchaFlag', 'i')]),
    'PACKET_ZC_COMPLETE_UPLOAD_MACRO_DETECTOR_CAPTCHA': (0x0a55, [('PacketType', 'h')]),
    'PACKET_ZC_ACK_PLAYER_AID_IN_RANGE': (0x0a6d, [('PacketType', 'h'), ('PacketLength', 'h'), ('AID', TAIL)]),
    # battlegrounds (clif_bg_message, clif_sendbgemblem_area, clif_bg_updatescore, clif_bg_xy, clif_bg_hp)
    'ZC_BATTLEFIELD_CHAT': (0x02dc, [('PacketType', 'H'), ('PacketLength', 'h'), ('AID', 'I'), ('name', '24s'),
                                     ('message', TAIL)]),
    'ZC_BATTLEFIELD_NOTIFY_CAMPINFO': (0x02dd, [('PacketType', 'H'), ('AID', 'I'), ('name', '24s'), ('camp', 'h')]),
    'ZC_BATTLEFIELD_NOTIFY_POINT': (0x02de, [('PacketType', 'H'), ('lion', 'h'), ('eagle', 'h')]),
    'ZC_BATTLEFIELD_NOTIFY_POSITION': (0x02df, [('PacketType', 'H'), ('AID', 'I'), ('name', '24s'), ('job', 'h'),
                                                ('x', 'h'), ('y', 'h')]),
    'PACKET_ZC_BATTLEFIELD_NOTIFY_HP': (0x0a0e, [('PacketType', 'h'), ('AID', 'I'), ('hp', 'i'), ('maxhp', 'i')]),
    # party booking (clif_PartyBooking*)
    'ZC_PARTY_BOOKING_ACK_REGISTER': (0x0803, [('PacketType', 'H'), ('result', 'h')]),
    'ZC_PARTY_BOOKING_ACK_SEARCH': (0x0805, [('PacketType', 'H'), ('PacketLength', 'h'), ('more', 'B'),
                                             ('ads', TAIL)]),
    'ZC_PARTY_BOOKING_ACK_DELETE': (0x0807, [('PacketType', 'H'), ('result', 'h')]),
    'ZC_PARTY_BOOKING_NOTIFY_INSERT': (0x0809, [('PacketType', 'H'), ('index', 'I'), ('name', '24s'), ('expire', 'I'),
                                                ('level', 'h'), ('mapId', 'h')] + [(f'job{i}', 'h') for i in range(6)]),
    'ZC_PARTY_BOOKING_NOTIFY_UPDATE': (0x080a, [('PacketType', 'H'), ('index', 'I')]
                                       + [(f'job{i}', 'h') for i in range(6)]),
    'ZC_PARTY_BOOKING_NOTIFY_DELETE': (0x080b, [('PacketType', 'H'), ('index', 'I')]),
    # personal information (clif_personal_information) and expanded barter shops
    # (clif_npc_expanded_barter_open)
    'PACKET_ZC_PERSONAL_INFOMATION': (0x097b, [('packetType', 'h'), ('length', 'h'), ('total_exp', 'i'),
                                               ('total_death', 'i'), ('total_drop', 'i'), ('details', TAIL)]),
    'PACKET_ZC_NPC_EXPANDED_BARTER_MARKET_ITEMINFO': (0x0b79, [('packetType', 'h'), ('packetLength', 'h'),
                                                               ('items_count', 'i'), ('items', TAIL)]),
    # chat, whispers, broadcasts, NPC talk and names, the packets the scenarios reach with short text
    # (clif_GlobalMessage, clif_notify_playerchat, clif_wis_message, clif_broadcast, clif_scriptmes,
    # clif_scriptmenu, clif_showvendingboard, clif_guild_notice, clif_guild_message, clif_messagecolor,
    # clif_send_petstatus, clif_pcname_ack, clif_npcname_ack)
    'PACKET_ZC_NOTIFY_CHAT': (0x008d, [('PacketType', 'h'), ('PacketLength', 'h'), ('GID', 'I'),
                                       ('Message', TAIL)]),
    'PACKET_ZC_NOTIFY_PLAYERCHAT': (0x008e, [('PacketType', 'h'), ('PacketLength', 'h'), ('Message', TAIL)]),
    'PACKET_ZC_WHISPER': (0x09de, [('PacketType', 'h'), ('PacketLength', 'h'), ('senderGID', 'I'),
                                   ('sender', '24s'), ('isAdmin', 'B'), ('message', TAIL)]),
    'ZC_BROADCAST': (0x009a, [('PacketType', 'H'), ('PacketLength', 'h'), ('message', TAIL)]),
    'PACKET_ZC_SAY_DIALOG': (0x00b4, [('PacketType', 'h'), ('PacketLength', 'h'), ('NpcID', 'I'),
                                      ('message', TAIL)]),
    'ZC_MENU_LIST': (0x00b7, [('PacketType', 'H'), ('PacketLength', 'h'), ('npcId', 'I'), ('message', TAIL)]),
    'PACKET_ZC_STORE_ENTRY': (0x0131, [('packetType', 'h'), ('makerAID', 'I'), ('storeName', '80s')]),
    'ZC_GUILD_NOTICE': (0x016f, [('PacketType', 'H'), ('subject', '60s'), ('notice', '120s')]),
    'ZC_GUILD_CHAT': (0x017f, [('PacketType', 'H'), ('PacketLength', 'h'), ('message', TAIL)]),
    'ZC_NPC_CHAT': (0x02c1, [('PacketType', 'H'), ('PacketLength', 'h'), ('AID', 'I'), ('color', 'I'),
                             ('message', TAIL)]),
    'PACKET_ZC_PROPERTY_PET': (0x01a2, [('PacketType', 'h'), ('szName', '24s'), ('bModified', 'b'),
                                        ('nLevel', 'h'), ('nFullness', 'h'), ('nRelationship', 'h'),
                                        ('ITID', 'h'), ('job', 'h')]),
    'PACKET_ZC_ACK_REQNAMEALL': (0x0a30, [('packet_id', 'H'), ('gid', 'i'), ('name', '24s'), ('party_name', '24s'),
                                          ('guild_name', '24s'), ('position_name', '24s'), ('title_id', 'i')]),
    'PACKET_ZC_ACK_REQNAMEALL_NPC': (0x0adf, [('packet_id', 'H'), ('gid', 'i'), ('groupId', 'i'), ('name', '24s'),
                                              ('title', '24s')]),
    # clans and the guild's own information (clif_clan_basicinfo, clif_clan_message, clif_guild_basicinfo)
    'PACKET_ZC_CLANINFO': (0x098a, [('PacketType', 'h'), ('PacketLength', 'h'), ('ClanID', 'I'),
                                    ('ClanName', '24s'), ('MasterName', '24s'), ('Map', '16s'), ('AllyCount', 'B'),
                                    ('AntagonistCount', 'B'), ('names', TAIL)]),
    'PACKET_ZC_NOTIFY_CLAN_CHAT': (0x098e, [('PacketType', 'h'), ('PacketLength', 'h'), ('MemberName', '24s'),
                                            ('Message', TAIL)]),
    'PACKET_ZC_GUILD_INFO': (0x0b7b, [('PacketType', 'h'), ('GDID', 'i'), ('level', 'i'), ('userNum', 'i'),
                                      ('maxUserNum', 'i'), ('userAverageLevel', 'i'), ('exp', 'i'), ('maxExp', 'i'),
                                      ('point', 'i'), ('honor', 'i'), ('virtue', 'i'), ('emblemVersion', 'i'),
                                      ('guildname', '24s'), ('manageLand', '16s'), ('zeny', 'i'), ('masterGID', 'i'),
                                      ('masterName', '24s')]),
    # the private packets of the Ragexe Saga server's plugins and clif.c additions that carry text
    # (clif_whisper_replay, clif_colorchat_self, the friends panel's clif_friends_info)
    'PACKET_ZC_WHISPER_REPLAY': (0x0be3, [('PacketType', 'h'), ('PacketLength', 'h'), ('direction', 'B'),
                                          ('isAdmin', 'B'), ('gid', 'I'), ('name', '24s'), ('message', TAIL)]),
    'PACKET_ZC_COLOR_CHAT': (0x00ae, [('PacketType', 'h'), ('PacketLength', 'h'), ('color', 'I'),
                                      ('message', TAIL)]),
    'PACKET_ZC_FRIENDS_INFO': (0x0b86, [('PacketType', 'h'), ('PacketLength', 'h'), ('offlineChar', 'B'),
                                        ('offlineAccount', 'B'), ('friends', TAIL)]),
    # the other names the map server sends (clif_party_member_info, clif_hominfo, clif_guild_belonginfo,
    # clif_rodex_send_..._write, clif_grade_enchant_broadcast_result, clif_cutin, clif_viewequip_ack,
    # clif_guild_positionnamelist, clif_rodex_checkname_result, clif_party_invite_reply, clif_charnameack)
    'PACKET_ZC_ADD_MEMBER_TO_GROUP': (0x0ae4, [('packetType', 'h'), ('AID', 'I'), ('GID', 'I'), ('leader', 'I'),
                                               ('class', 'h'), ('baseLevel', 'h'), ('x', 'h'), ('y', 'h'),
                                               ('offline', 'B'), ('partyName', '24s'), ('playerName', '24s'),
                                               ('mapName', '16s'), ('sharePickup', 'b'), ('shareLoot', 'b')]),
    'PACKET_ZC_PROPERTY_HOMUN': (0x0ba4, [('packetType', 'h'), ('name', '24s'), ('flags', 'B'), ('level', 'H'),
                                          ('hunger', 'H'), ('intimacy', 'H'), ('atk2', 'H'), ('matk', 'H'),
                                          ('hit', 'H'), ('crit', 'H'), ('def', 'H'), ('mdef', 'H'), ('flee', 'H'),
                                          ('amotion', 'H'), ('hp', 'I'), ('maxHp', 'I'), ('sp', 'I'), ('maxSp', 'I'),
                                          ('exp', 'q'), ('expNext', 'q'), ('skillPoints', 'H'), ('range', 'H')]),
    'PACKET_ZC_UPDATE_GDID': (0x02f7, [('PacketType', 'h'), ('guildId', 'I'), ('emblemVersion', 'i'), ('mode', 'I'),
                                       ('isMaster', 'B'), ('interSid', 'i'), ('guildName', '24s'),
                                       ('masterGID', 'I')]),
    'PACKET_ZC_ACK_OPEN_WRITE_MAIL': (0x0a12, [('PacketType', 'h'), ('receiveName', '24s'), ('result', 'b')]),
    'PACKET_ZC_GRADE_ENCHANT_BROADCAST_RESULT': (0x0b5e, [('packetType', 'h'), ('name', '24s'), ('itemId', 'I'),
                                                          ('grade', 'h'), ('status', 'b')]),
    'PACKET_ZC_SHOW_IMAGE': (0x01b3, [('packetType', 'h'), ('image', '64s'), ('type', 'B')]),
    'PACKET_ZC_EQUIPWIN_MICROSCOPE': (0x0b37, [('PacketType', 'h'), ('PacketLength', 'h'), ('characterName', '24s'),
                                               ('job', 'h'), ('head', 'h'), ('accessory', 'h'), ('accessory2', 'h'),
                                               ('accessory3', 'h'), ('robe', 'h'), ('headpalette', 'h'),
                                               ('bodypalette', 'h'), ('body2', 'h'), ('sex', 'B'), ('list', TAIL)]),
    'PACKET_ZC_POSITION_ID_NAME_INFO': (0x0166, [('PacketType', 'h'), ('PacketLength', 'h'), ('posInfo', TAIL)]),
    'PACKET_ZC_CHECKNAME': (0x0a51, [('PacketType', 'h'), ('CharId', 'i'), ('Class', 'h'), ('BaseLevel', 'h'),
                                     ('Name', '24s')]),
    'PACKET_ZC_PARTY_JOIN_REQ_ACK': (0x02c5, [('PacketType', 'h'), ('characterName', '24s'), ('result', 'i')]),
    'packet_reqname_ack': (0x0095, [('packet_id', 'H'), ('gid', 'i'), ('name', '24s')]),
})

# The entries of the list packets above: Hercules' struct of one entry, or clif.c's fields of one.
SUBLAYOUTS = {
    'ZC_ENTER_ROOM_member': [('role', 'I'), ('name', '24s')],
    'ZC_FRIENDS_LIST_friend': [('AID', 'I'), ('GID', 'I'), ('name', '24s')],
    'PACKET_ZC_GROUP_LIST_SUB': [('AID', 'I'), ('GID', 'I'), ('playerName', '24s'), ('mapName', '16s'),
                                 ('leader', 'B'), ('offline', 'B'), ('class', 'h'), ('baseLevel', 'h')],
    'PACKET_ZC_MAKABLEITEMLIST_sub': [('itemId', 'I'), ('material[0]', 'I'), ('material[1]', 'I'), ('material[2]', 'I')],
    'PACKET_ZC_MAKINGARROW_LIST_sub': [('itemId', 'I')],
    'PACKET_ZC_MAKINGITEM_LIST_sub': [('itemId', 'I')],
    'PACKET_ZC_NOTIFY_WEAPONITEMLIST_sub': [('index', 'h'), ('itemId', 'I'), ('refine', 'B')] + _SLOT,
    'PACKET_ZC_WARPLIST_sub': [('map', '16s')],
    'ZC_SKILL_POSTDELAY_LIST2_skill': [('skillId', 'H'), ('total', 'I'), ('left', 'I')],
    'packet_quest_hunt_sub': [('huntIdent', 'I'), ('huntIdent2', 'I'), ('mobType', 'I'), ('mob_id', 'I'),
                              ('levelMin', 'h'), ('levelMax', 'h'), ('huntCount', 'h'), ('mobName', '24s')],
    'packet_quest_list_info': [('questID', 'i'), ('active', 'b'), ('quest_svrTime', 'i'), ('quest_endTime', 'i'),
                               ('hunting_count', 'h')],
    'packet_mission_info_sub': [('huntIdent', 'I'), ('huntIdent2', 'I'), ('mobType', 'I'), ('mob_id', 'I'),
                                ('levelMin', 'h'), ('levelMax', 'h'), ('huntCount', 'h'), ('maxCount', 'h'),
                                ('mobName', '24s')],
    'packet_quest_update_hunt': [('questID', 'I'), ('huntIdent', 'I'), ('huntIdent2', 'I'), ('maxCount', 'h'),
                                 ('count', 'h')],
    'PACKET_ZC_ACK_ITEMLIST_BUYING_STORE_sub': [('price', 'I'), ('amount', 'H'), ('itemType', 'B'), ('itemId', 'I')],
    'PACKET_ZC_PC_CASH_POINT_ITEMLIST_sub': [('price', 'I'), ('discountPrice', 'I'), ('itemType', 'B'),
                                             ('itemId', 'I')],
    'REPAIRITEM_INFO2': [('index', 'h'), ('itemId', 'I')] + _SLOT + [('refine', 'B'), ('grade', 'B')],
    'PACKET_ZC_MERGE_ITEM_OPEN_sub': [('index', 'h')],
    'packet_quest_hunt_info_sub': [('questID', 'I'), ('mob_id', 'I'), ('maxCount', 'h'), ('count', 'h')],
    'PACKET_ZC_NPC_MARKET_OPEN_sub': [('nameid', 'I'), ('type', 'B'), ('price', 'I'), ('qty', 'I'), ('weight', 'H'),
                                      ('location', 'I')],
    'PACKET_ZC_NPC_MARKET_PURCHASE_RESULT_sub': [('ITID', 'I'), ('qty', 'H'), ('price', 'I')],
    'ZC_PARTY_BOOKING_ACK_SEARCH_ad': [('index', 'I'), ('name', '24s'), ('expire', 'I'), ('level', 'h'),
                                       ('mapId', 'h')] + [(f'job{i}', 'h') for i in range(6)],
    'PACKET_ZC_PERSONAL_INFOMATION_SUB': [('type', 'b'), ('exp', 'i'), ('death', 'i'), ('drop', 'i')],
    'PACKET_ZC_NPC_EXPANDED_BARTER_MARKET_ITEMINFO_sub': [('nameid', 'I'), ('type', 'H'), ('amount', 'I'),
                                                          ('weight', 'I'), ('index', 'I'), ('zeny', 'I'),
                                                          ('viewSprite', 'H'), ('location', 'I'),
                                                          ('currency_count', 'I'),
                                                          ('currencies[0].nameid', 'I'),
                                                          ('currencies[0].refine_level', 'H'),
                                                          ('currencies[0].amount', 'I'), ('currencies[0].type', 'H')],
    'PACKET_ZC_NPC_EXPANDED_BARTER_MARKET_ITEMINFO_sub2': [('nameid', 'I'), ('refine_level', 'H'), ('amount', 'I'),
                                                           ('type', 'H')],
    'PACKET_ZC_FRIENDS_INFO_sub': [('AID', 'i'), ('GID', 'i'), ('baseLevel', 'h'), ('jobLevel', 'h'),
                                   ('job', '24s'), ('listed', '24s')],
    'PACKET_ZC_POSITION_ID_NAME_INFO_sub': [('positionID', 'i'), ('posName', '24s')],
    'PACKET_ZC_NPC_BARTER_MARKET_ITEMINFO_sub': [('nameid', 'I'), ('type', 'B'), ('amount', 'I'),
                                                 ('currencyNameid', 'I'), ('currencyAmount', 'I'), ('weight', 'I'),
                                                 ('index', 'I'), ('viewSprite', 'H'), ('location', 'I')],
}

# The client asks with these; FuzzReplay answers them as the server does (seqfuzz.py).
CZ_NOTIFY_ACTORINIT = 0x007d


def layout_format(name):
    return '<' + ''.join(f for _, f in LAYOUTS[name][1] if f != TAIL)


def is_variable(name):
    """True for a packet whose last field (TAIL) is a run of bytes of any length."""
    return LAYOUTS[name][1][-1][1] == TAIL


def pack(_name, **fields):
    """The packet `_name` with the fields given (the rest zero); its id and length are filled in. A
    variable packet's TAIL field takes bytes (or a str, cp949), its length counted in PacketLength."""
    name = _name
    pid, layout = LAYOUTS[name]
    tail = b''
    if is_variable(name):
        tail = fields.pop(layout[-1][0], b'')
        if isinstance(tail, str):
            tail = tail.encode('cp949')
    size = struct.calcsize(layout_format(name)) + len(tail)
    values = []
    for at, (field, fmt) in enumerate(layout):
        if fmt == TAIL:
            continue
        if field in ('PacketType', 'packetType', 'packet_id'):
            v = pid
        elif field in ('PacketLength', 'packetLength', 'packetLen') or (at == 1 and field == 'length'):
            v = size
        else:
            v = fields.pop(field, b'' if fmt.endswith('s') else 0)
            if isinstance(v, str):
                v = v.encode('cp949')
        values.append(v)
    if fields:
        raise KeyError(f'{name} has no field {sorted(fields)}')
    return struct.pack(layout_format(name), *values) + tail


def pack_sub(_name, **fields):
    """One entry of a list packet's array (SUBLAYOUTS), the fields not given zero."""
    layout = SUBLAYOUTS[_name]
    values = []
    for field, fmt in layout:
        v = fields.pop(field, b'' if fmt.endswith('s') else 0)
        if isinstance(v, str):
            v = v.encode('cp949')
        values.append(v)
    if fields:
        raise KeyError(f'{_name} has no field {sorted(fields)}')
    return struct.pack('<' + ''.join(f for _, f in layout), *values)


def pos_dir(x, y, d):
    """WBUFPOS: x.10 y.10 dir.4."""
    return bytes([(x >> 2) & 0xff, ((x << 6) | ((y >> 4) & 0x3f)) & 0xff, ((y << 4) | (d & 0x0f)) & 0xff])


def move_data(x0, y0, x1, y1, sx0=8, sy0=8):
    """WBUFPOS2: x0.10 y0.10 x1.10 y1.10 sx0.4 sy0.4."""
    return bytes([(x0 >> 2) & 0xff, ((x0 << 6) | ((y0 >> 4) & 0x3f)) & 0xff,
                  ((y0 << 4) | ((x1 >> 6) & 0x0f)) & 0xff, ((x1 << 2) | ((y1 >> 8) & 0x03)) & 0xff,
                  y1 & 0xff, ((sx0 << 4) | (sy0 & 0x0f)) & 0xff])


def map_name(name):
    return (name + '.gat').encode()


# ---- the recorded prefix -----------------------------------------------------------------------

# The recording whose login and map entry start every case, the frame the map connection is cut at
# (every event before it is kept), and the recorded map-entry burst a later map change reuses: the
# first one after the cut (the scenario's @warp), whose own-character packets are this account's.
PREFIX = 'world'
CUT_FRAME = 600
BURST_FRAME = 729
TAIL_START = 640          # the first generated event's frame


def _frame_packets(data, sizes):
    out, i = [], 0
    while i + 2 <= len(data):
        pid = data[i] | data[i + 1] << 8
        size = sizes.get(pid, -1)
        if size == -1:
            size = data[i + 2] | data[i + 3] << 8
        out.append(data[i:i + size])
        i += size
    return out


class Prefix:
    """What a case takes from the recording PREFIX: its connections up to CUT_FRAME, the client's
    account id, map, cell and the server's tick at a known frame, the NPC entries of the map entry,
    and the map-entry burst a map change sends (its NPC entries and keepalive left out)."""

    def __init__(self, sizes, name=PREFIX, cut=CUT_FRAME, burst_frame=BURST_FRAME):
        with open(os.path.join(RECORDINGS, name + '.json')) as f:
            rec = json.load(f)['connections']
        self.name, self.cut = name, cut
        self.connections = rec[:-1]
        mapconn = rec[-1]
        self.map_conn = {k: v for k, v in mapconn.items() if k != 'events'}
        self.events = [e for e in mapconn['events'] if e.get('c', e.get('s', e.get('close', e.get('peer_close'))))
                       < cut]
        self.sent = sum(len(e['data']) // 2 for e in self.events if 'c' in e)
        stream = b''.join(bytes.fromhex(e['data']) for e in self.events if 's' in e)
        packets = _frame_packets(stream, sizes)
        first = next(e for e in self.events if 's' in e)
        self.npc_entries = []
        for p in packets:
            pid = struct.unpack_from('<H', p)[0]
            if pid == 0x0283:
                self.aid = struct.unpack_from('<I', p, 2)[0]
            elif pid == 0x02eb:
                self.tick0, self.tick_frame = struct.unpack_from('<I', p, 2)[0], first['s']
            elif pid == 0x0091:
                self.map = p[2:18].split(b'\0')[0].decode()[:-4]
                self.x, self.y = struct.unpack_from('<hh', p, 18)
            elif pid == 0x09ff:
                self.npc_entries.append(p)
        burst = next(e for e in mapconn['events'] if e.get('s') == burst_frame)
        self.burst = b''.join(p for p in _frame_packets(bytes.fromhex(burst['data']), sizes)
                              if struct.unpack_from('<H', p)[0] not in (0x09ff, 0x007f))
        self.npc_ids = [struct.unpack_from('<I', p, 5)[0] for p in self.npc_entries]

    def tick(self, frame):
        """The server's tick at a client frame (both clocks advance FRAME_MS a frame in a replay)."""
        return (self.tick0 + (frame - self.tick_frame) * FRAME_MS) & 0xffffffff


# ---- the world as the server sees it ------------------------------------------------------------

# Maps a case moves between, with the cell the player lands on (cells Hercules warps to in the
# recordings). The first is the prefix's own map.
MAPS = {'prontera': (155, 185), 'geffen': (119, 59)}
MOB_JOBS = [1002, 1113, 1063, 1031, 1049, 1007]
NPC_JOBS = [117, 122, 83, 92, 45]
PC_JOBS = [0, 1, 2, 4, 7, 12]
UNIT_TYPES = {0x8d: 87, 0x85: 25, 0x7e: 12, 0x7f: 18, 0x91: 117, 0x8e: 92, 0x83: 70, 0x86: 0}  # UNT -> skill
ICE_WALL = 0x8d
ITEMS = [501, 502, 909, 512, 1201, 2301, 4001]
OTHER_ICONS = [437, 464, 645, 660, 734, 755, 760]    # SCs with HasVisualEffect: shown on other units
OWN_ICONS = [0, 1, 2, 10, 12, 45]
HIT_SKILLS = [19, 14, 89, 5, 46, 13]
SUPPORT_SKILLS = [28, 34, 29]
GROUND_SKILLS = [89, 21, 83]
VANISH_OUT, VANISH_DEAD, VANISH_LOGOUT, VANISH_TELEPORT = 0, 1, 2, 3
CLUT = {'pc': 0x0, 'mob': 0x5, 'npc': 0x6}
RESPAWN_FRAMES = 320       # 5 s: the shortest monster respawn delay


class Ent:
    def __init__(self, eid, kind, map_, **attrs):
        self.id, self.kind, self.map = eid, kind, map_
        self.listed = False     # the client holds it
        self.near = False       # in the player's view on the server
        self.gone = False       # logged out, expired, picked up: never back
        self.dead = False
        self.dead_at = None
        self.opt = (0, 0, 0)
        self.icons = set()
        self.__dict__.update(attrs)


class World:
    """The server's view of the player's surroundings, and the packets each event makes. apply()
    returns an event's chunks ([(frame gap, packets, waits for the map load)]) or None when the
    server could not send it now; it then changes nothing."""

    def __init__(self, prefix):
        self.prefix = prefix
        self.map, self.x, self.y = prefix.map, prefix.x, prefix.y
        self.ents = {}
        self.frame = TAIL_START
        self.loads = 0           # map changes so far: the client's next CZ_NOTIFY_ACTORINIT is number loads
        self.load_frame = None   # the frame the last map change's load is taken to end at
        for eid, p in zip(prefix.npc_ids, prefix.npc_entries):
            e = Ent(eid, 'prefix_npc', prefix.map, packet=p)
            e.listed = e.near = True
            self.ents[eid] = e

    # -- lookups

    def here(self, kinds, **cond):
        out = []
        for e in self.ents.values():
            if e.kind in kinds and e.map == self.map and not e.gone and all(getattr(e, k) == v for k, v in cond.items()):
                out.append(e)
        return sorted(out, key=lambda e: e.id)

    def _target(self, tid):
        """A hit's target or source: the player, a listed living actor, or a listed Ice Wall."""
        if tid == self.prefix.aid:
            return True
        e = self.ents.get(tid)
        if not e or not e.listed or e.map != self.map:
            return False
        if e.kind == 'unit':
            return e.utype == ICE_WALL
        return e.kind in ('mob', 'pc') and not e.dead

    # -- packets

    def entry(self, e, how):
        if e.kind == 'prefix_npc':
            return [e.packet]
        if e.kind == 'unit':
            out = [pack('packet_skill_entry', AID=e.id, creatorAID=e.creator, xPos=e.x, yPos=e.y, job=e.utype,
                        RadiusRange=e.range, isVisible=1, level=e.level)]
            if e.utype == ICE_WALL:
                out.append(pack('ZC_UPDATE_MAPINFO', x=e.x, y=e.y, type=5, mapName=map_name(self.map)))
            return out
        if e.kind == 'item':
            if how == 'drop':
                return [pack('packet_dropflooritem', ITAID=e.id, ITID=e.item, type=e.itype, IsIdentified=1,
                             xPos=e.x, yPos=e.y, subX=e.subx, subY=e.suby, count=e.amount)]
            return [pack('PACKET_ZC_ITEM_ENTRY', AID=e.id, itemId=e.item, identify=1, x=e.x, y=e.y,
                         amount=e.amount, subX=e.subx, subY=e.suby)]
        common = dict(objecttype=CLUT[e.kind], AID=e.id, GID=e.gid, speed=e.speed, bodyState=e.opt[0],
                      healthState=e.opt[1], effectState=e.opt[2], job=e.job, head=e.head, weapon=e.weapon,
                      headpalette=e.hpal, bodypalette=e.bpal, sex=e.sex, clevel=e.level, maxHP=e.maxhp,
                      HP=0 if e.dead else e.maxhp, name=e.name)
        out = [pack('PACKET_ZC_WALK_STYLE', aid=e.id)] if e.kind == 'pc' else []
        if how == 'walk':
            out.append(pack('packet_unit_walking', moveStartTime=self.prefix.tick(self.frame),
                            MoveData=move_data(e.x, e.y, e.tx, e.ty), **common))
        elif how == 'spawn':
            out.append(pack('packet_spawn_unit', PosDir=pos_dir(e.x, e.y, e.dir), **common))
        else:
            out.append(pack('packet_idle_unit', PosDir=pos_dir(e.x, e.y, e.dir),
                            state=1 if e.dead else 0, **common))
        return out

    def new_id(self, kind):
        self.last_id = getattr(self, 'last_id', {})
        base = {'pc': 2900500, 'npc': 110030000, 'obj': 40000}[kind]
        n = self.last_id.get(kind, base) + 1
        self.last_id[kind] = n
        return n

    def near_cell(self, r, spread=7):
        return (self.x + r.randint(-spread, spread), self.y + r.randint(-spread, spread))

    # -- the events

    def apply(self, ev):
        k = ev['k']
        fn = getattr(self, 'ev_' + k)
        chunks = fn(ev)
        if chunks is not None:
            for gap, _, _ in chunks:
                self.frame += gap
        return chunks

    def _one(self, ev, packets):
        return [(ev['gap'], packets, False)]

    def ev_state(self, ev):
        return [(ev['gap'], [], False)]

    def ev_pkt(self, ev):
        """A packet made for a handler (seqtarget.py), sent as it is: 'name' its layout, 'data' its hex."""
        return self._one(ev, [bytes.fromhex(ev['data'])])

    def ev_new(self, ev):
        """A new actor, ground unit or item (made by the generator with its attributes)."""
        if ev['id'] in self.ents or self.map != ev['map']:
            return None
        a = dict(ev['attrs'])
        e = Ent(ev['id'], ev['kind'], ev['map'], **a)
        self.ents[e.id] = e
        how = ev.get('how', 'idle')
        if e.kind == 'mob' and how == 'walk':
            e.tx, e.ty = ev['to']
        e.listed = e.near = True
        packets = self.entry(e, how)
        if how == 'walk':
            e.x, e.y = e.tx, e.ty
        return self._one(ev, packets)

    def ev_enter(self, ev):
        """A known one comes (back) into view; 'refresh' for an actor already shown."""
        e = self.ents.get(ev['id'])
        how = ev['how']
        if not e or e.gone or e.map != self.map:
            return None
        if e.listed and not (how == 'refresh' and e.kind in ('mob', 'pc', 'npc', 'prefix_npc') and not e.dead):
            return None
        if not e.listed and how == 'refresh':
            return None
        if e.kind == 'mob' and e.dead:
            # only its respawn brings a dead monster back
            if how != 'spawn' or self.frame + ev['gap'] - e.dead_at < RESPAWN_FRAMES:
                return None
            e.dead = False
            e.opt, e.icons = (0, 0, 0), set()
        elif how == 'spawn' and e.kind != 'mob':
            how = 'idle'
        if how == 'walk' and e.kind != 'mob':
            how = 'idle'
        if how == 'walk':
            e.tx, e.ty = ev['to']
        e.listed = e.near = True
        packets = self.entry(e, 'idle' if how == 'refresh' else how)
        if how == 'walk':
            e.x, e.y = e.tx, e.ty
        return self._one(ev, packets)

    def ev_leave(self, ev):
        e = self.ents.get(ev['id'])
        if not e or not e.listed or e.map != self.map:
            return None
        why = ev['why']
        if e.kind in ('unit', 'item'):
            pkt = (pack('ZC_SKILL_DISAPPEAR', AID=e.id) if e.kind == 'unit' else pack('ZC_ITEM_DISAPPEAR', AID=e.id))
            packets = [pkt]
            if e.kind == 'unit' and e.utype == ICE_WALL:
                packets.append(pack('ZC_UPDATE_MAPINFO', x=e.x, y=e.y, type=0, mapName=map_name(self.map)))
            e.listed = False
            if why == 'gone':
                e.gone = True
            else:
                e.near = False
            return self._one(ev, packets)
        if why == VANISH_LOGOUT and e.kind != 'pc':
            return None
        if why != VANISH_OUT and e.kind in ('npc', 'prefix_npc'):
            return None
        if why == VANISH_DEAD:
            return None
        e.listed = False
        e.near = False
        if why == VANISH_LOGOUT:
            e.gone = True
        if why == VANISH_TELEPORT and 'to' in ev:
            e.x, e.y = ev['to']
        return self._one(ev, [pack('ZC_NOTIFY_VANISH', AID=e.id, type=why)])

    def ev_move(self, ev):
        e = self.ents.get(ev['id'])
        if not e or not e.listed or e.kind != 'mob' or e.dead or e.map != self.map:
            return None
        x1, y1 = ev['to']
        p = pack('ZC_NOTIFY_MOVE', AID=e.id, MoveData=move_data(e.x, e.y, x1, y1),
                 moveStartTime=self.prefix.tick(self.frame + ev['gap']))
        e.x, e.y = x1, y1
        return self._one(ev, [p])

    def ev_stop(self, ev):
        e = self.ents.get(ev['id'])
        if not e or not e.listed or e.kind != 'mob' or e.dead or e.map != self.map:
            return None
        return self._one(ev, [pack('ZC_STOPMOVE', AID=e.id, x=e.x, y=e.y)])

    def ev_die(self, ev):
        e = self.ents.get(ev['id'])
        if not e or not e.listed or e.kind not in ('mob', 'pc') or e.dead or e.map != self.map:
            return None
        # status_change_clear ends what the unit shows before it is taken off the map as dead
        packets = [pack('packet_status_change_end', index=i, AID=e.id, state=0) for i in sorted(e.icons)]
        if e.opt != (0, 0, 0):
            packets.append(pack('PACKET_ZC_STATE_CHANGE', AID=e.id))
        e.icons, e.opt = set(), (0, 0, 0)
        e.dead = True
        e.dead_at = self.frame + ev['gap']
        if e.kind == 'mob':
            e.listed = False
        return self._one(ev, packets + [pack('ZC_NOTIFY_VANISH', AID=e.id, type=VANISH_DEAD)])

    def ev_revive(self, ev):
        e = self.ents.get(ev['id'])
        if not e or not e.listed or e.kind != 'pc' or not e.dead or e.map != self.map:
            return None
        e.dead = False
        return self._one(ev, [pack('ZC_RESURRECTION', AID=e.id, type=0)])

    def ev_hit(self, ev):
        src, dst = ev['src'], ev['dst']
        if not self._target(src) or not self._target(dst) or src == dst:
            return None
        se = self.ents.get(src)
        if se is not None and se.kind == 'unit':
            return None
        tick = self.prefix.tick(self.frame + ev['gap'])
        if ev['how'] == 'melee':
            p = pack('packet_damage', GID=src, targetGID=dst, startTime=tick, attackMT=ev['amt'],
                     attackedMT=ev['dmt'], damage=ev['dmg'], count=ev['div'], action=ev['act'])
        else:
            p = pack('PACKET_ZC_NOTIFY_SKILL', SKID=ev['skill'], AID=src, targetID=dst, startTime=tick,
                     attackMT=ev['amt'], attackedMT=ev['dmt'], damage=ev['dmg'], level=ev['lv'],
                     count=ev['div'], action=ev['act'])
        return self._one(ev, [p])

    def ev_support(self, ev):
        src, dst = ev['src'], ev['dst']
        if not self._target(src) or not self._target(dst):
            return None
        if any(self.ents.get(i) is not None and self.ents[i].kind == 'unit' for i in (src, dst)):
            return None
        return self._one(ev, [pack('PACKET_ZC_USE_SKILL', SKID=ev['skill'], level=ev['lv'], targetAID=dst,
                                   srcAID=src, result=1)])

    def ev_ground(self, ev):
        src = ev['src']
        if not self._target(src) or (self.ents.get(src) is not None and self.ents[src].kind == 'unit'):
            return None
        x, y = ev['at']
        return self._one(ev, [pack('PACKET_ZC_NOTIFY_GROUNDSKILL', SKID=ev['skill'], AID=src, level=ev['lv'],
                                   xPos=x, yPos=y, startTime=self.prefix.tick(self.frame + ev['gap']))])

    def ev_cast(self, ev):
        src, dst = ev['src'], ev['dst']
        if not self._target(src) or (dst and not self._target(dst)):
            return None
        if any(self.ents.get(i) is not None and self.ents[i].kind == 'unit' for i in (src,)):
            return None
        x, y = ev['at']
        return self._one(ev, [pack('PACKET_ZC_USESKILL_ACK', srcId=src, dstId=dst, x=x, y=y, skillId=ev['skill'],
                                   element=ev['elem'], delayTime=ev['delay'])])

    def _status_target(self, tid):
        if tid == self.prefix.aid:
            return 'self'
        e = self.ents.get(tid)
        if not e or not e.listed or e.kind not in ('mob', 'pc') or e.dead or e.map != self.map:
            return None
        return e

    def ev_icon(self, ev):
        t = self._status_target(ev['id'])
        if t is None:
            return None
        icons = self.own_icons if t == 'self' else t.icons
        icon = ev['icon']
        if ev['on']:
            if icon in icons:
                return None
            icons.add(icon)
            p = pack('packet_status_change', index=icon, AID=ev['id'], state=1, Total=ev['ms'], Left=ev['ms'],
                     val1=1)
        else:
            if icon not in icons:
                return None
            icons.discard(icon)
            p = pack('packet_status_change_end', index=icon, AID=ev['id'], state=0)
        return self._one(ev, [p])

    @property
    def own_icons(self):
        if not hasattr(self, '_own_icons'):
            self._own_icons = set()
        return self._own_icons

    def ev_opt(self, ev):
        t = self._status_target(ev['id'])
        if t is None or t == 'self':
            return None
        t.opt = tuple(ev['opt'])
        return self._one(ev, [pack('PACKET_ZC_STATE_CHANGE', AID=t.id, bodyState=t.opt[0], healthState=t.opt[1],
                                   effectState=t.opt[2])])

    def ev_warp(self, ev):
        dest = ev['map']
        if dest == self.map and dest != self.prefix.map:
            return None
        x, y = MAPS[dest]
        for e in self.ents.values():
            e.listed = False
        self.map, self.x, self.y = dest, x, y
        self.loads += 1
        first = [(ev['gap'], [pack('ZC_NPCACK_MAPMOVE', mapName=map_name(dest), x=x, y=y)], False)]
        # the map-entry burst and everything in view: 3 frames after the client's map-loaded packet,
        # as in the recording; the client is taken to send it the frame after the map change
        packets = [self.prefix.burst]
        for e in sorted(self.ents.values(), key=lambda e: e.id):
            if e.map != dest or e.gone or not e.near:
                continue
            if e.kind == 'mob' and e.dead:
                continue
            e.listed = True
            packets += self.entry(e, 'idle')
        self.load_frame = self.frame + ev['gap'] + 1
        return first + [(4, packets, True)]


# ---- generation --------------------------------------------------------------------------------

WEIGHTS = {'new': 8, 'enter': 9, 'leave': 9, 'move': 6, 'stop': 2, 'die': 4, 'revive': 2, 'hit': 12,
           'support': 3, 'ground': 2, 'cast': 3, 'icon': 5, 'opt': 3, 'warp': 1.2, 'state': 5}


def _gap(r):
    x = r.random()
    if x < 0.25:
        return 0
    if x < 0.6:
        return r.randint(1, 6)
    if x < 0.9:
        return r.randint(7, 30)
    return r.randint(31, 90)


def _hit_targets(w):
    out = [w.prefix.aid] + [e.id for e in w.here(('mob', 'pc'), listed=True, dead=False)]
    out += [e.id for e in w.here(('unit',), listed=True) if e.utype == ICE_WALL]
    return out


def _sources(w):
    return [w.prefix.aid] + [e.id for e in w.here(('mob', 'pc'), listed=True, dead=False)]


def propose(w, r):
    """One random event the server could send now, or None."""
    k = r.choices(list(WEIGHTS), list(WEIGHTS.values()))[0]
    ev = {'k': k, 'gap': _gap(r)}
    if k == 'state':
        ev['gap'] = max(ev['gap'], 2)
        return ev
    if k == 'new':
        kind = r.choices(['mob', 'pc', 'npc', 'unit', 'item'], [5, 2, 1, 5, 3])[0]
        x, y = w.near_cell(r)
        attrs = {'x': x, 'y': y, 'dir': r.randrange(8)}
        if kind in ('mob', 'pc', 'npc'):
            attrs.update(gid=0, speed=r.choice([150, 200, 400]), head=r.randint(1, 20) if kind == 'pc' else 0,
                         weapon=r.choice([0, 1201]) if kind == 'pc' else 0, hpal=0, bpal=0,
                         sex=r.randrange(2), level=r.randint(1, 99), maxhp=r.randint(50, 9000),
                         job={'mob': r.choice(MOB_JOBS), 'pc': r.choice(PC_JOBS), 'npc': r.choice(NPC_JOBS)}[kind])
            eid = w.new_id({'mob': 'obj', 'pc': 'pc', 'npc': 'npc'}[kind])
            attrs['name'] = f'{kind}{eid % 1000}'
            if kind == 'pc':
                attrs['gid'] = 150000 + eid % 100000
        elif kind == 'unit':
            utype = r.choice(list(UNIT_TYPES))
            if r.random() < 0.4:
                utype = ICE_WALL
            creators = [w.prefix.aid] + [e.id for e in w.here(('mob', 'pc'), listed=True)]
            attrs.update(utype=utype, creator=r.choice(creators), range=r.choice([0, 1, 2]), level=r.randint(1, 10))
            eid = w.new_id('obj')
        else:
            item = r.choice(ITEMS)
            attrs.update(item=item, itype=r.choice([0, 2, 4, 5]), amount=r.randint(1, 30), subx=r.randint(0, 15),
                         suby=r.randint(0, 15))
            eid = w.new_id('obj')
        ev.update(id=eid, kind=kind, map=w.map, attrs=attrs)
        ev['how'] = {'mob': r.choice(['idle', 'spawn', 'walk']), 'pc': r.choice(['idle', 'spawn']),
                     'npc': 'idle', 'unit': 'idle', 'item': r.choice(['drop', 'drop', 'idle'])}[kind]
        if ev['how'] == 'walk':
            ev['to'] = w.near_cell(r)
        return ev
    if k == 'enter':
        out = [e for e in w.here(('mob', 'pc', 'npc', 'unit', 'item')) if not e.listed]
        refresh = [e for e in w.here(('mob', 'pc', 'npc', 'prefix_npc'), listed=True, dead=False)]
        if out and (not refresh or r.random() < 0.85):
            e = r.choice(out)
            how = 'spawn' if e.kind == 'mob' and e.dead else r.choice(['idle', 'idle', 'walk', 'spawn'])
            if e.kind == 'item':
                how = 'idle'
            ev.update(id=e.id, how=how)
            if e.kind == 'mob' and e.dead:
                ev['gap'] = max(ev['gap'], RESPAWN_FRAMES - (w.frame - e.dead_at))
            if how == 'walk':
                ev['to'] = w.near_cell(r)
            return ev
        if refresh:
            ev.update(id=r.choice(refresh).id, how='refresh')
            return ev
        return None
    if k == 'leave':
        cands = w.here(('mob', 'pc', 'npc', 'unit', 'item', 'prefix_npc'), listed=True)
        if not cands:
            return None
        e = r.choice(cands)
        if e.kind in ('unit', 'item'):
            ev.update(id=e.id, why=r.choice(['out', 'out', 'gone']))
        else:
            why = r.choice([VANISH_OUT, VANISH_OUT, VANISH_TELEPORT] + ([VANISH_LOGOUT] if e.kind == 'pc' else []))
            if e.kind in ('npc', 'prefix_npc'):
                why = VANISH_OUT
            ev.update(id=e.id, why=why)
            if why == VANISH_TELEPORT:
                ev['to'] = w.near_cell(r, 12)
        return ev
    if k in ('move', 'stop'):
        cands = w.here(('mob',), listed=True, dead=False)
        if not cands:
            return None
        e = r.choice(cands)
        ev['id'] = e.id
        if k == 'move':
            ev['to'] = (e.x + r.randint(-5, 5), e.y + r.randint(-5, 5))
        return ev
    if k == 'die':
        cands = w.here(('mob', 'pc'), listed=True, dead=False)
        if not cands:
            return None
        ev['id'] = r.choice(cands).id
        return ev
    if k == 'revive':
        cands = w.here(('pc',), listed=True, dead=True)
        if not cands:
            return None
        ev['id'] = r.choice(cands).id
        return ev
    if k == 'hit':
        srcs, dsts = _sources(w), _hit_targets(w)
        if len(dsts) < 1 or len(srcs) < 1:
            return None
        src, dst = r.choice(srcs), r.choice(dsts)
        if src == dst:
            return None
        div = r.choice([1, 1, 1, 2, 3, 5])
        ev.update(src=src, dst=dst, how=r.choice(['melee', 'melee', 'skill']), amt=r.choice([432, 576, 864, 1152]),
                  dmt=r.choice([288, 384, 480, 576]), dmg=r.choice([0, r.randint(1, 50), r.randint(50, 3000)]),
                  div=div, act=(8 if div > 1 else r.choice([0, 0, 10, 4])), skill=r.choice(HIT_SKILLS),
                  lv=r.randint(1, 10))
        if ev['how'] == 'skill':
            ev['act'] = 8 if div > 1 else 6
        return ev
    if k == 'support':
        srcs = _sources(w)
        ev.update(src=r.choice(srcs), dst=r.choice(srcs), skill=r.choice(SUPPORT_SKILLS), lv=r.randint(1, 500))
        return ev
    if k == 'ground':
        ev.update(src=r.choice(_sources(w)), skill=r.choice(GROUND_SKILLS), lv=r.randint(1, 10), at=w.near_cell(r))
        return ev
    if k == 'cast':
        srcs = _sources(w)
        src = r.choice(srcs)
        dst = r.choice(_hit_targets(w) + [0])
        ev.update(src=src, dst=dst, at=w.near_cell(r), skill=r.choice(HIT_SKILLS + GROUND_SKILLS),
                  elem=r.randrange(10), delay=r.choice([300, 800, 1500, 3000]))
        return ev
    if k == 'icon':
        cands = [w.prefix.aid] + [e.id for e in w.here(('mob', 'pc'), listed=True, dead=False)]
        tid = r.choice(cands)
        icons = w.own_icons if tid == w.prefix.aid else w.ents[tid].icons
        pool = OWN_ICONS if tid == w.prefix.aid else OTHER_ICONS
        on = r.random() < 0.6 or not icons
        icon = r.choice(sorted(set(pool) - icons) or pool) if on else r.choice(sorted(icons))
        ev.update(id=tid, icon=icon, on=on, ms=r.choice([2000, 10000, 60000]))
        return ev
    if k == 'opt':
        cands = w.here(('mob', 'pc'), listed=True, dead=False)
        if not cands:
            return None
        ev.update(id=r.choice(cands).id, opt=r.choice([(0, 0, 0), (3, 0, 0), (2, 0, 0), (0, 1, 0), (0, 0, 2),
                                                       (0, 0, 4), (1, 0, 0)]))
        return ev
    if k == 'warp':
        ev['map'] = r.choice([m for m in MAPS if m != w.map] or [w.map])
        ev['gap'] = max(ev['gap'], 1)
        return ev
    return None


def generate(seed, prefix, length=50):
    """The events of case `seed`: `length` events the server could send, in order."""
    r = random.Random(seed)
    w = World(prefix)
    events = []
    tries = 0
    while len(events) < length and tries < length * 50:
        tries += 1
        ev = propose(w, r)
        if ev is None:
            continue
        if w.apply(ev) is None:
            continue
        events.append(ev)
    return events


# ---- a case as a recording ---------------------------------------------------------------------

SETTLE_FRAMES = 400   # after the last event, before the last state: every hit's and skill's objects gone
START_STATE, END_STATE = 'start', 'end'


def render(events, prefix):
    """(the chunks of `events` as recording server events, the events kept, the quit frame).
    Events the server could not send at their place (a shortened list's) are left out."""
    w = World(prefix)
    out, kept = [{'s': TAIL_START - 10, 'after': prefix.sent, 'state': START_STATE}], []
    n_state = 0
    wait = None          # every chunk after a map change waits for that map's load
    for ev in events:
        start = w.frame
        chunks = w.apply(ev)
        if chunks is None:
            continue
        kept.append(ev)
        frame = start
        for gap, packets, after_load in chunks:
            frame += gap
            if after_load:
                wait = [w.loads, w.load_frame]
            rec = {'s': frame, 'after': prefix.sent}
            if wait:
                rec['wait'] = list(wait)
            if ev['k'] == 'state':
                rec['state'] = f's{n_state:02d}'
                n_state += 1
            else:
                rec['data'] = b''.join(packets).hex()
            out.append(rec)
    end = w.frame + SETTLE_FRAMES
    last = {'s': end, 'after': prefix.sent, 'state': END_STATE}
    if wait:
        last['wait'] = list(wait)
    out.append(last)
    return out, kept, end + 10


def case_recording(seed, prefix, events, quit_frame, chunks):
    conn = dict(prefix.map_conn)
    conn['events'] = prefix.events + chunks
    return {'connections': prefix.connections + [conn],
            'seqgen': {'seed': seed, 'prefix': prefix.name, 'cut': prefix.cut, 'sent': prefix.sent,
                       'quit': quit_frame, 'aid': prefix.aid, 'events': events}}


def build(seed, prefix, events=None, length=50):
    """Case `seed` as a recording dict: its own events, or `events` (a list to replay, such as a
    minimized one)."""
    if events is None:
        events = generate(seed, prefix, length)
    chunks, kept, quit_frame = render(events, prefix)
    return case_recording(seed, prefix, kept, quit_frame, chunks)
