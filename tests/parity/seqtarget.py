"""Packet sequences aimed at the client's packet handlers that the parity scenarios never reach, for the
differential tester (seqfuzz.py target).

Each SCRIPT is a short run of what Hercules (PACKETVER 20220406, ~/ragexe/Hercules src/map/clif.c) sends
in one situation: a chat room joined, changed and left; a trade from the request to its end; a party's
members coming and going; the answers to a skill's item or map list. Its packets are built from
seqgen.LAYOUTS, which tests/ragexe/seqgen checks against Hercules' structs and length table, and each is
followed by a state checkpoint. The server sends most of these only as the answer to something the
client asked for; here they come unasked, which both clients take the same way.

A case is one or more scripts after the prefix's map entry (seqgen.build with the events given):

    events(names, prefix)        the events of the named scripts, in order
    scripts_for(targets, tree)   the scripts that send a packet one of the targets handles: a
                                 handler function's name (a `case HEADER_...:` arm of
                                 src/game/packet_dispatch.c or packet_dispatch2.c), a packet's
                                 HEADER_ name, its layout name or its id (0x....)
"""
import os
import re
import struct

import seqgen
from seqgen import pack, pack_sub

GAP = 4             # frames from a script's packet to the next one
STATE_GAP = 24      # frames from a packet to the state taken after it

# Ids a script gives the units it brings in: out of the generator's own ranges (seqgen.World.new_id).
PC_ID, PC_GID = 2950001, 160001
MOB_ID = 48001


class Ctx:
    """What a script needs of the prefix: the player's ids and map, the NPCs standing there."""

    def __init__(self, prefix):
        self.prefix = prefix
        self.aid = prefix.aid
        self.gid = _own_gid(prefix)
        self.map = prefix.map
        self.x, self.y = prefix.x, prefix.y
        self.npcs = prefix.npc_ids


def _own_gid(prefix):
    """The character id the client sent in CZ_ENTER2 (0x0436: id.W AID.L GID.L ...)."""
    for e in prefix.events:
        if 'c' in e:
            data = bytes.fromhex(e['data'])
            if data[:2] == b'\x36\x04':
                return struct.unpack_from('<I', data, 6)[0]
    return 0


def pkt(_layout, _gap=GAP, _state=True, **fields):
    """The events for one packet: the packet, then (unless _state is False) a state after it."""
    out = [{'k': 'pkt', 'gap': _gap, 'name': _layout, 'data': pack(_layout, **fields).hex()}]
    if _state:
        out.append({'k': 'state', 'gap': STATE_GAP})
    return out


def new_pc(ctx, eid, name, job=0, dx=2, dy=1):
    attrs = {'x': ctx.x + dx, 'y': ctx.y + dy, 'dir': 2, 'gid': PC_GID + eid - PC_ID, 'speed': 150, 'head': 3,
             'weapon': 0, 'hpal': 0, 'bpal': 0, 'sex': 1, 'level': 80, 'maxhp': 5000, 'job': job, 'name': name}
    return [{'k': 'new', 'gap': GAP, 'id': eid, 'kind': 'pc', 'map': ctx.map, 'how': 'idle', 'attrs': attrs},
            {'k': 'state', 'gap': STATE_GAP}]


def new_mob(ctx, eid, job=1002, dx=-2, dy=2):
    attrs = {'x': ctx.x + dx, 'y': ctx.y + dy, 'dir': 0, 'gid': 0, 'speed': 200, 'head': 0, 'weapon': 0, 'hpal': 0,
             'bpal': 0, 'sex': 0, 'level': 10, 'maxhp': 500, 'job': job, 'name': f'mob{eid % 1000}'}
    return [{'k': 'new', 'gap': GAP, 'id': eid, 'kind': 'mob', 'map': ctx.map, 'how': 'idle', 'attrs': attrs},
            {'k': 'state', 'gap': STATE_GAP}]


def name24(s):
    return s.encode('cp949')[:23]


# ---- the scripts -------------------------------------------------------------------------------

def chat_room_member(ctx):
    """Another player's chat room shown, joined, changed, and left: members coming and going, the owner
    handing over, the player kicked, the room closed; then a refused entry."""
    owner, room = PC_ID, 7001
    ev = new_pc(ctx, owner, 'RoomOwner')
    ev += pkt('ZC_ROOM_NEWENTRY', ownerAID=owner, roomId=room, limit=20, users=1, type=1, title='Trading 501')
    members = (pack_sub('ZC_ENTER_ROOM_member', role=0, name='RoomOwner')
               + pack_sub('ZC_ENTER_ROOM_member', role=1, name='parity04'))
    ev += pkt('ZC_ENTER_ROOM', roomId=room, members=members)
    ev += pkt('ZC_MEMBER_NEWENTRY', users=3, name='Third')
    ev += pkt('ZC_CHANGE_CHATROOM', ownerAID=owner, roomId=room, limit=5, users=3, type=0, title='Renamed room')
    ev += pkt('PACKET_ZC_ROLE_CHANGE', flag=0, name='Third')
    ev += pkt('PACKET_ZC_ROLE_CHANGE', flag=1, name='RoomOwner')
    ev += pkt('ZC_MEMBER_EXIT', users=2, name='Third', flag=0)
    ev += pkt('ZC_MEMBER_EXIT', users=1, name='parity04', flag=1)
    ev += pkt('ZC_DESTROY_ROOM', roomId=room)
    ev += pkt('ZC_REFUSE_ENTER_ROOM', result=0)
    ev += pkt('ZC_REFUSE_ENTER_ROOM', result=1)
    ev += pkt('ZC_ROOM_NEWENTRY', ownerAID=owner, roomId=room + 1, limit=2, users=2, type=0, title='Private')
    ev += pkt('ZC_DESTROY_ROOM', roomId=room + 1)
    return ev


def chat_room_owner(ctx):
    """The player's own chat room: created, shown over the player, a member in and out, closed; then a
    creation refused."""
    room = 7101
    ev = pkt('ZC_ACK_CREATE_CHATROOM', result=0)
    ev += pkt('ZC_ROOM_NEWENTRY', ownerAID=ctx.aid, roomId=room, limit=10, users=1, type=1, title='My room')
    ev += pkt('ZC_MEMBER_NEWENTRY', users=2, name='Visitor')
    ev += pkt('ZC_CHANGE_CHATROOM', ownerAID=ctx.aid, roomId=room, limit=10, users=2, type=1, title='My room 2')
    ev += pkt('ZC_MEMBER_EXIT', users=1, name='Visitor', flag=0)
    ev += pkt('ZC_DESTROY_ROOM', roomId=room)
    ev += pkt('ZC_ACK_CREATE_CHATROOM', result=1)
    ev += pkt('ZC_ACK_CREATE_CHATROOM', result=2)
    return ev


def vending_board(ctx):
    """A vendor's shop sign taken down, and a purchase from a vendor refused for each reason."""
    vendor = PC_ID + 10
    ev = new_pc(ctx, vendor, 'Vendor', job=5)
    ev += pkt('ZC_DISAPPEAR_ENTRY', AID=vendor)
    for result in (0, 1, 2, 4, 6, 7):
        ev += pkt('ZC_PC_PURCHASE_RESULT_FROMMC', index=2, amount=1, result=result)
    return ev


def _trade_item(**kw):
    base = dict(itemId=501, itemType=0, amount=5, identified=1, damaged=0, location=0, look=0, refine=0, grade=0)
    base.update(kw)
    return base


def trade(ctx):
    """A trade from another player's request: accepted, items and an equipment added, the partner and the
    player locking in, an undo, the trade done; then a second request cancelled, and refusals."""
    partner = PC_ID + 1
    ev = new_pc(ctx, partner, 'Trader', job=4)
    ev += pkt('ZC_REQ_EXCHANGE_ITEM2', name='Trader', charId=PC_GID + 1, level=77)
    ev += pkt('ZC_ACK_EXCHANGE_ITEM2', result=3, charId=PC_GID + 1, level=77)
    ev += pkt('PACKET_ZC_ADD_EXCHANGE_ITEM', **_trade_item())
    ev += pkt('PACKET_ZC_ADD_EXCHANGE_ITEM', **_trade_item(itemId=1201, itemType=5, amount=1, refine=7, location=2,
                                                          **{'slot.card[0]': 4001, 'option_data[0].index': 1,
                                                             'option_data[0].value': 10}))
    ev += pkt('PACKET_ZC_ADD_EXCHANGE_ITEM', **_trade_item(itemId=2301, itemType=4, amount=1, damaged=1,
                                                          location=16))
    ev += pkt('ZC_ACK_ADD_EXCHANGE_ITEM', index=2, result=0)
    ev += pkt('ZC_ACK_ADD_EXCHANGE_ITEM', index=2, result=1)
    ev += pkt('ZC_CONCLUDE_EXCHANGE_ITEM', who=1)
    ev += pkt('ZC_EXCHANGEITEM_UNDO')
    ev += pkt('ZC_CONCLUDE_EXCHANGE_ITEM', who=0)
    ev += pkt('ZC_CONCLUDE_EXCHANGE_ITEM', who=1)
    ev += pkt('ZC_EXEC_EXCHANGE_ITEM', result=0)
    ev += pkt('ZC_REQ_EXCHANGE_ITEM2', name='Trader', charId=PC_GID + 1, level=77)
    ev += pkt('ZC_ACK_EXCHANGE_ITEM2', result=3, charId=PC_GID + 1, level=77)
    ev += pkt('ZC_CANCEL_EXCHANGE_ITEM')
    for result in (0, 1, 2, 4):
        ev += pkt('ZC_ACK_EXCHANGE_ITEM2', result=result, charId=PC_GID + 1, level=77)
    ev += pkt('ZC_EXEC_EXCHANGE_ITEM', result=1)
    return ev


def friends(ctx):
    """The friend list sent, friends going on and offline, a friend request, its answers, a removal."""
    fr = [(PC_ID + 20 + i, PC_GID + 20 + i, f'Friend{i}') for i in range(3)]
    tail = b''.join(pack_sub('ZC_FRIENDS_LIST_friend', AID=a, GID=g, name=n) for a, g, n in fr)
    ev = pkt('ZC_FRIENDS_LIST', friends=tail)
    ev += pkt('ZC_FRIENDS_STATE', AID=fr[0][0], GID=fr[0][1], state=0, name=fr[0][2])
    ev += pkt('ZC_FRIENDS_STATE', AID=fr[1][0], GID=fr[1][1], state=0, name=fr[1][2])
    ev += pkt('ZC_FRIENDS_STATE', AID=fr[0][0], GID=fr[0][1], state=1, name=fr[0][2])
    ev += pkt('ZC_REQ_ADD_FRIENDS', AID=PC_ID + 30, GID=PC_GID + 30, name='Asker')
    ev += pkt('ZC_ADD_FRIENDS', result=0, AID=PC_ID + 30, GID=PC_GID + 30, name='Asker')
    for result in (1, 2, 3, 4):
        ev += pkt('ZC_ADD_FRIENDS', result=result, AID=PC_ID + 31, GID=PC_GID + 31, name='Other')
    ev += pkt('ZC_DELETE_FRIENDS', AID=fr[2][0], GID=fr[2][1])
    ev += pkt('ZC_DELETE_FRIENDS', AID=PC_ID + 30, GID=PC_GID + 30)
    ev += pkt('ZC_FRIENDS_LIST', friends=b'')
    return ev


def party(ctx):
    """A party of the player and two others: its chat, a member's level and job, a death and revival, an
    item a member picked up, the leader handing over, a member leaving and one expelled; then an invitation
    to another party."""
    a, b = PC_ID + 40, PC_ID + 41
    ev = new_pc(ctx, a, 'Member1', job=1)

    def members(*rows):
        return b''.join(pack_sub('PACKET_ZC_GROUP_LIST_SUB', AID=aid, GID=gid, playerName=n,
                                 mapName=(ctx.map + '.gat'), leader=lead, offline=off, **{'class': job},
                                 baseLevel=lv) for aid, gid, n, lead, off, job, lv in rows)

    rows = [(ctx.aid, ctx.gid, 'parity04', 0, 0, 0, 10), (a, PC_GID + 40, 'Member1', 1, 0, 1, 50),
            (b, PC_GID + 41, 'Member2', 1, 1, 4, 60)]
    ev += pkt('PACKET_ZC_GROUP_LIST', partyName='Party One', members=members(*rows))
    ev += pkt('PACKET_ZC_NOTIFY_CHAT_PARTY', AID=a, chatMsg=b'Member1 : hello party\0')
    ev += pkt('PACKET_ZC_NOTIFY_MEMBERINFO_TO_GROUPM', AID=a, job=7, level=51)
    ev += pkt('PACKET_ZC_GROUP_ISALIVE', AID=a, isDead=1)
    ev += pkt('PACKET_ZC_GROUP_ISALIVE', AID=a, isDead=0)
    ev += pkt('PACKET_ZC_ITEM_PICKUP_PARTY', AID=a, itemId=1201, identified=1, location=2, itemType=5, refine=5)
    ev += pkt('PACKET_ZC_ITEM_PICKUP_PARTY', AID=a, itemId=501, identified=1, itemType=0)
    ev += pkt('packet_party_leader_changed', prev_leader_aid=ctx.aid, new_leader_aid=a)
    ev += pkt('PACKET_ZC_DELETE_MEMBER_FROM_GROUP', AID=b, characterName='Member2', result=0)
    ev += pkt('PACKET_ZC_DELETE_MEMBER_FROM_GROUP', AID=a, characterName='Member1', result=1)
    ev += pkt('PACKET_ZC_DELETE_MEMBER_FROM_GROUP', AID=ctx.aid, characterName='parity04', result=0)
    ev += pkt('PACKET_ZC_PARTY_JOIN_REQ', GRID=88, groupName='Other Party')
    return ev


def messages(ctx):
    """The server's table, formatted and broadcast messages, as the scripts and the map server send them."""
    ev = []
    # ids clif.c and the scripts send (src/map/messages_main.h)
    for mid in (0x34, 0x297, 0x4c5, 0x4c6, 0x4f2, 0x54d, 0x5ef, 0x6ed, 0x6ee, 0x729, 0x783, 0x78b, 0x82e):
        ev += pkt('ZC_MSG', msgId=mid)
    for mid, v in ((0x746, 30), (0x61e, 40), (0x61f, 10)):
        ev += pkt('ZC_MSG_VALUE', msgId=mid, value=v)
    for skill, mid in ((228, 0x626), (228, 0x627), (228, 0x625)):
        ev += pkt('ZC_MSG_SKILL', skillId=skill, msgId=mid)
    ev += pkt('PACKET_ZC_MSG_COLOR', MessageId=0x6ad, MessageColor=0x00ff00)
    for mid, text in ((0x527, b'Endless Tower\0'), (0x528, b'Endless Tower\0'), (0x529, b'Orc Memory\0'),
                      (0x52a, b'Sealed Shrine\0')):
        ev += pkt('PACKET_ZC_FORMATSTRING_MSG', MessageId=mid, MessageString=text)
    ev += pkt('PACKET_ZC_FORMATSTRING_MSG_COLOR', messageId=0x527, color=0xff0000,
              messageString=b'Endless Tower\0')
    ev += pkt('ZC_BROADCAST2', fontColor=0x00ffff, fontType=400, fontSize=12, fontAlign=0, fontY=0,
              message=b'A coloured broadcast\0')
    ev += pkt('ZC_BROADCAST2', fontColor=0xff00ff, fontType=400, fontSize=12, fontAlign=0, fontY=0,
              message=b'ssss Blue-ish start\0')
    ev += pkt('PACKET_ZC_MVP_GETTING_ITEM', itemId=607)
    ev += pkt('ZC_THROW_MVPITEM')
    ev += pkt('ZC_USER_COUNT', count=42)
    ev += pkt('ZC_NOTIFY_FONT', AID=ctx.aid, font=1)
    ev += pkt('ZC_NOTIFY_FONT', AID=ctx.aid, font=0)
    ev += pkt('PACKET_ZC_DEBUGMSG', color=0xffffff, message=b'debug line\0')
    for t, v in ((0, 1234), (1, 75), (2, 3), (3, 59), (5, -5)):
        ev += pkt('ZC_SHOWDIGIT', type=t, value=v)
    ev += pkt('ZC_SHOWSCRIPT', AID=ctx.npcs[0], message=b'Hello there!\0')
    ev += pkt('ZC_SHOWSCRIPT', AID=ctx.aid, message=b'Over my head\0')
    for t in (0, 1, 2, 3, 4):
        ev += pkt('ZC_ACK_GIVE_MANNER_POINT', type=t)
    for t in (0, 1, 2):
        ev += pkt('ZC_NOTIFY_MANNER_POINT_GIVEN', type=t, name='GameMaster')
    ev += pkt('PACKET_ZC_BROADCASTING_SPECIAL_ITEM_OBTAIN_item', type=0, ItemID=607, len=8, Name=b'Lucky Me',
              boxItemID_len=2, BoxItemID=616)
    ev += pkt('PACKET_ZC_ATTACK_FAILURE_FOR_DISTANCE', targetAID=ctx.npcs[0], targetXPos=ctx.x + 5,
              targetYPos=ctx.y + 5, xPos=ctx.x, yPos=ctx.y, currentAttRange=1)
    return ev


def skills(ctx):
    """Skill packets outside a cast: a skill learned, the ground and spirit effects, and the lists a skill
    asks the player to pick from (Auto Spell, Arrow Crafting, Pharmacy and Cooking, refining, Warp Portal,
    the item composition list), with the answers to them."""
    mob = MOB_ID
    ev = new_mob(ctx, mob)
    ev += pkt('PACKET_ZC_ADD_SKILL', **{'skill.id': 28, 'skill.inf': 16, 'skill.level': 3, 'skill.sp': 22,
                                        'skill.range2': 9, 'skill.name': 'AL_HEAL', 'skill.upFlag': 1})
    ev += pkt('PACKET_ZC_AUTORUN_SKILL', skill_id=26, skill_type=4, skill_lv=1, skill_sp=0, skill_range=0,
              skill_name='AL_TELEPORT', up_flag=0)
    ev += pkt('PACKET_ZC_SPIRITS', AID=ctx.aid, num=5)
    ev += pkt('PACKET_ZC_SPIRITS', AID=ctx.aid, num=0)
    ev += pkt('packet_graffiti_entry', AID=MOB_ID + 50, creatorAID=ctx.aid, xPos=ctx.x + 1, yPos=ctx.y,
              job=0xb0, isVisible=1, isContens=1, msg=b'Graffiti!')
    ev += pkt('ZC_SKILL_UPDATE', AID=MOB_ID + 50)
    ev += pkt('ZC_SKILL_DISAPPEAR', AID=MOB_ID + 50)
    ev += pkt('ZC_DEVOTIONLIST', AID=ctx.aid, dst0=mob, range=7)
    ev += pkt('ZC_DEVOTIONLIST', AID=ctx.aid, range=0)
    ev += pkt('ZC_STARSKILL', mapName=b'prontera', monsterId=0, star=0, result=0)
    ev += pkt('ZC_STARSKILL', mapName=b'', monsterId=1002, star=1, result=10)
    ev += pkt('PACKET_ZC_SKILL_SELECT_REQUEST', flag=0, skillIds=struct.pack('<3h', 14, 19, 20))
    ev += pkt('PACKET_ZC_AUTOSPELLLIST', skills=struct.pack('<4i', 11, 14, 19, 20))
    ev += pkt('PACKET_ZC_MAKINGARROW_LIST', items=b''.join(pack_sub('PACKET_ZC_MAKINGARROW_LIST_sub', itemId=i)
                                                           for i in (1750, 1751, 1752)))
    ev += pkt('PACKET_ZC_MAKABLEITEMLIST', items=b''.join(pack_sub('PACKET_ZC_MAKABLEITEMLIST_sub', itemId=i)
                                                          for i in (501, 503, 545)))
    ev += pkt('PACKET_ZC_MAKINGITEM_LIST', makeItem=1, items=b''.join(pack_sub('PACKET_ZC_MAKINGITEM_LIST_sub',
                                                                               itemId=i) for i in (12041, 12042)))
    ev += pkt('PACKET_ZC_NOTIFY_WEAPONITEMLIST', items=pack_sub('PACKET_ZC_NOTIFY_WEAPONITEMLIST_sub', index=2,
                                                                itemId=1201, refine=3))
    for result in (0, 1, 2, 3):
        ev += pkt('PACKET_ZC_ACK_WEAPONREFINE', result=result, itemId=1201)
    for result in (0, 1, 2, 3):
        ev += pkt('PACKET_ZC_ACK_REQMAKINGITEM', result=result, itemId=501)
    ev += pkt('ZC_ITEMCOMPOSITION_LIST', indexes=struct.pack('<2h', 2, 3))
    ev += pkt('ZC_ITEMCOMPOSITION_LIST', indexes=b'')
    ev += pkt('PACKET_ZC_WARPLIST', skillId=27, maps=b''.join(pack_sub('PACKET_ZC_WARPLIST_sub', map=m) for m in
                                                              (b'prontera.gat', b'geffen.gat', b'')))
    ev += pkt('ZC_ITEMLISTWIN_OPEN', type=1)
    ev += pkt('ZC_ITEMLISTWIN_OPEN', type=0)
    ev += pkt('ZC_NPC_SHOWEFST_UPDATE', AID=mob, option=0x2, level=10, opt3=0)
    ev += pkt('ZC_MONSTER_INFO', job=1002, level=1, size=1, hp=50, race=3, mdef=5, element=21,
              ratios=bytes([100, 100, 90, 100, 100, 100, 100, 100, 100]))
    for flag in (0, 1, 2, 3):
        ev += pkt('ZC_BOSS_INFO', flag=flag, x=150, y=170, hours=1, minutes=5, name=b'Baphomet')
    ev += pkt('ZC_FASTMOVE', AID=mob, x=ctx.x - 3, y=ctx.y + 3)
    ev += pkt('PACKET_ZC_START_CAPTURE')
    ev += pkt('PACKET_ZC_TRYCAPTURE_MONSTER', result=0)
    ev += pkt('PACKET_ZC_TRYCAPTURE_MONSTER', result=1)
    ev += pkt('ZC_SKILL_POSTDELAY_LIST2', skills=b''.join(pack_sub('ZC_SKILL_POSTDELAY_LIST2_skill', skillId=s,
                                                                   total=5000, left=3000) for s in (28, 89)))
    return ev


def quests(ctx):
    """The quest list at login, a quest added with hunting objectives, counts updated, made inactive and
    active, deleted, and quest marks over an NPC."""
    hunt = pack_sub('packet_quest_hunt_sub', huntIdent=1, huntIdent2=1, mobType=0, mob_id=1002, levelMin=0,
                    levelMax=0, huntCount=0, mobName='Poring')
    ev = pkt('ZC_ADD_QUEST_EX2', questID=1000, active=1, quest_svrTime=0, quest_endTime=0, count=1,
             objectives=hunt)
    info = (pack_sub('packet_quest_list_info', questID=1001, active=1, hunting_count=1)
            + pack_sub('packet_mission_info_sub', huntIdent=2, huntIdent2=2, mob_id=1113, maxCount=10, huntCount=3,
                       mobName='Drops')
            + pack_sub('packet_quest_list_info', questID=1002, active=0, hunting_count=0))
    ev += pkt('packet_quest_list_header', questCount=2, quests=info)
    upd = pack_sub('packet_quest_update_hunt', questID=1000, huntIdent=1, huntIdent2=1, maxCount=5, count=2)
    ev += pkt('packet_quest_update_header', count=1, objectives=upd)
    ev += pkt('ZC_ACTIVE_QUEST', questID=1000, active=0)
    ev += pkt('ZC_ACTIVE_QUEST', questID=1000, active=1)
    ev += pkt('ZC_DEL_QUEST', questID=1000)
    ev += pkt('ZC_DEL_QUEST', questID=4242)
    for state, color in ((0, 0), (1, 1), (2, 2), (5, 3), (9999, 0)):
        ev += pkt('ZC_QUEST_NOTIFY_EFFECT', AID=ctx.npcs[1], x=ctx.x, y=ctx.y, state=state, color=color)
    return ev


def npc_script(ctx):
    """What NPC scripts send besides the dialog: a dialog cleared, music and sounds, a progress bar and its
    cancel, the dialog's alignment, a book, the navigation."""
    npc = ctx.npcs[0]
    ev = pkt('packet_script_clear', NpcID=npc)
    ev += pkt('PACKET_ZC_PLAY_NPC_BGM', bgm='01')
    ev += pkt('PACKET_ZC_SOUND', name='effect\\gong.wav', act=0, term=0, AID=npc)
    ev += pkt('PACKET_ZC_SOUND', name='effect\\gong.wav', act=1, term=1000, AID=npc)
    ev += pkt('PACKET_ZC_SOUND', name='effect\\gong.wav', act=2, term=0, AID=npc)
    ev += pkt('ZC_PROGRESS', color=0xff0000, seconds=3)
    ev += pkt('ZC_PROGRESS_CANCEL')
    ev += pkt('PACKET_ZC_DIALOG_TEXT_ALIGN', align=1)
    ev += pkt('PACKET_ZC_DIALOG_TEXT_ALIGN', align=0)
    ev += pkt('ZC_READ_BOOK', bookId=7036, page=1)
    ev += pkt('ZC_NAVIGATION_ACTIVE', type=0, flag=111, hide=0, mapName=b'geffen', x=119, y=59, mobId=0)
    ev += pkt('ZC_NAVIGATION_ACTIVE', type=1, flag=0, hide=1, mapName=b'prontera', x=0, y=0, mobId=0)
    ev += pkt('ZC_NAVIGATION_ACTIVE', type=3, flag=0, hide=0, mapName=b'prt_fild08', x=0, y=0, mobId=1002)
    return ev


def items(ctx):
    """Item packets outside the inventory lists: equipment damaged, the cart's answers, the cart look
    choice, cards, unequip answers, the favourite tab, a bound item, the inventory extension, a failed
    move, rental time."""
    ev = pkt('ZC_EQUIPITEM_DAMAGED', position=2, AID=ctx.aid)
    ev += pkt('ZC_EQUIPITEM_DAMAGED', position=16, AID=ctx.aid)
    for result in (0, 1, 2):
        ev += pkt('packet_cart_additem_ack', result=result)
    ev += pkt('PACKET_ZC_ADD_ITEM_TO_CART', index=2, amount=3, itemId=501, itemType=0, identified=1)
    ev += pkt('PACKET_ZC_ADD_ITEM_TO_CART', index=3, amount=1, itemId=1201, itemType=5, identified=1, refine=4,
              **{'slot.card[0]': 4001})
    ev += pkt('ZC_SELECTCART', AID=ctx.aid, carts=bytes([6, 7, 8, 9]))
    ev += pkt('PACKET_ZC_UPDATE_CARDSLOT', wearState=2, cardSlot=0, itemId=4001, equipFlag=1)
    for result in (0, 1):
        ev += pkt('PACKET_ZC_ACK_TAKEOFF_EQUIP_ALL', result=result)
    ev += pkt('packet_unequipitem_ack', index=2, wearLocation=0, result=1)
    ev += pkt('ZC_INVENTORY_TAB', index=2, favorite=1)
    ev += pkt('ZC_INVENTORY_TAB', index=2, favorite=0)
    ev += pkt('packet_notify_bounditem', index=2)
    for result in (0, 1, 2, 3):
        ev += pkt('PACKET_ZC_ACK_OPEN_MSGBOX_EXTEND_BODYITEM_SIZE', result=result, itemId=25793)
    for result in (0, 1, 2):
        ev += pkt('PACKET_ZC_ACK_EXTEND_BODYITEM_SIZE', result=result)
    ev += pkt('PACKET_ZC_EXTEND_BODYITEM_SIZE', expansionSize=10)
    ev += pkt('PACKET_ZC_MOVE_ITEM_FAILED', itemIndex=2, itemCount=1)
    ev += pkt('PACKET_ZC_CASH_TIME_COUNTER', itemId=1201, seconds=3600)
    ev += pkt('PACKET_ZC_CASH_ITEM_DELETE', index=2, itemId=1201)
    return ev


def marriage(ctx):
    """The partner's name called out, the wedding effect, an adoption request and its answers."""
    other = PC_ID + 50
    ev = new_pc(ctx, other, 'Spouse')
    ev += pkt('ZC_COUPLENAME', name='Spouse')
    ev += pkt('ZC_COUPLENAME', name='')
    ev += pkt('ZC_CONGRATULATION', AID=other)
    ev += pkt('ZC_CONGRATULATION', AID=ctx.aid)
    ev += pkt('ZC_REQ_BABY', AID=other, partyId=PC_ID + 51, name='Spouse')
    for t in (0, 1, 2):
        ev += pkt('ZC_BABYMSG', type=t)
    return ev


def rankings(ctx):
    """PvP rank numbers over the player, and ranking points (blacksmith, alchemist, taekwon)."""
    ev = pkt('ZC_NOTIFY_RANKING', AID=ctx.aid, rank=3, total=10)
    ev += pkt('ZC_NOTIFY_RANKING', AID=ctx.aid, rank=0xffffffff, total=10)
    for t in (0, 1, 2):
        ev += pkt('ZC_UPDATE_RANKING_POINT', type=t, points=1, total=t * 10)
    return ev


def memorial_dungeon(ctx):
    """A memorial dungeon reserved, its queue position, entered with each timer, and its end notices."""
    name = b'Endless Tower'
    ev = pkt('ZC_MEMORIALDUNGEON_SUBSCRIPTION_INFO', name=name, flag=1)
    ev += pkt('ZC_MEMORIALDUNGEON_SUBSCRIPTION_NOTIFY', flag=0)
    ev += pkt('ZC_MEMORIALDUNGEON_INFO', name=name, progress=0, idle=2000000000)
    ev += pkt('ZC_MEMORIALDUNGEON_INFO', name=name, progress=2000000000, idle=0)
    for flag in (0, 1, 2, 3, 4):
        ev += pkt('ZC_MEMORIALDUNGEON_NOTIFY', flag=flag)
    return ev


def buying_store(ctx):
    """A buyer's shop sign, its item list, a sale to it with every result, the sign taken down; the
    player's own buying store refused and its sales; store search answers."""
    buyer = PC_ID + 60
    ev = new_pc(ctx, buyer, 'Buyer', job=5)
    ev += pkt('PACKET_ZC_BUYING_STORE_ENTRY', makerAID=buyer, storeName=b'Buying jellopies')
    items = (pack_sub('PACKET_ZC_ACK_ITEMLIST_BUYING_STORE_sub', price=20, amount=100, itemType=3, itemId=909)
             + pack_sub('PACKET_ZC_ACK_ITEMLIST_BUYING_STORE_sub', price=500, amount=2, itemType=0, itemId=501))
    ev += pkt('PACKET_ZC_ACK_ITEMLIST_BUYING_STORE', AID=buyer, storeId=55, zenyLimit=100000, items=items)
    for result in (3, 4, 5, 6, 7):
        ev += pkt('PACKET_ZC_FAILED_TRADE_BUYING_STORE_TO_SELLER', result=result, itemId=909)
    ev += pkt('PACKET_ZC_DISAPPEAR_BUYING_STORE_ENTRY', makerAID=buyer)
    for result in (1, 2, 8):
        ev += pkt('ZC_FAILED_OPEN_BUYING_STORE_TO_BUYER', result=result, weight=9000)
    for result in (3, 4):
        ev += pkt('ZC_FAILED_TRADE_BUYING_STORE_TO_BUYER', result=result)
    ev += pkt('ZC_ITEM_DELETE_BUYING_STORE', index=2, amount=1, price=20)
    ev += pkt('PACKET_ZC_UPDATE_ITEM_FROM_BUYING_STORE', itemId=909, amount=5, zeny=100, zenyLimit=99900,
              charId=PC_GID + 60, updateTime=0)
    for reason in (0, 1, 2, 3, 4, 5, 6):
        ev += pkt('ZC_SEARCH_STORE_INFO_FAILED', reason=reason)
    ev += pkt('ZC_SSILIST_ITEM_CLICK_ACK', x=ctx.x + 3, y=ctx.y + 3)
    return ev


def cash_shop(ctx):
    """An NPC cash shop's item list, the answers to a purchase from it, and the item mall's."""
    items = b''.join(pack_sub('PACKET_ZC_PC_CASH_POINT_ITEMLIST_sub', price=p, discountPrice=p, itemType=t,
                              itemId=i) for p, t, i in ((100, 0, 501), (2500, 5, 1201), (10, 3, 909)))
    ev = pkt('PACKET_ZC_PC_CASH_POINT_ITEMLIST', cashPoints=1000, kafraPoints=50, items=items)
    for error in (0, 1, 2, 3, 4, 5, 6):
        ev += pkt('ZC_PC_CASH_POINT_UPDATE', cashPoints=900, kafraPoints=50, error=error)
    for result in (0, 1, 2, 3, 4, 5, 6, 7):
        ev += pkt('PACKET_ZC_SE_PC_BUY_CASHITEM_RESULT', itemId=501, result=result, cashPoints=900, kafraPoints=0)
    return ev


def item_services(ctx):
    """The answers of item skills and NPC services: Magnifier's list and result, a card's insertion, a
    refine, Weapon Repair's list and result, item merging, the overweight notice, a cart item taken out,
    an item sold from the player's shop, a refine broadcast."""
    ev = pkt('ZC_ITEMIDENTIFY_LIST', indexes=struct.pack('<3h', 2, 3, 4))
    for result in (0, 1):
        ev += pkt('ZC_ACK_ITEMIDENTIFY', index=2, result=result)
    ev += pkt('ZC_ITEMIDENTIFY_LIST', indexes=b'')
    for result in (0, 1):
        ev += pkt('ZC_ACK_ITEMCOMPOSITION', equipIndex=2, cardIndex=3, result=result)
    for result in (0, 1, 2, 3):
        ev += pkt('ZC_ACK_ITEMREFINING', result=result, index=2, refine=5)
    items = b''.join(pack_sub('REPAIRITEM_INFO2', index=i, itemId=n, refine=r) for i, n, r in ((2, 1201, 0),
                                                                                              (3, 2301, 4)))
    ev += pkt('PACKET_ZC_REPAIRITEMLIST', items=items)
    for result in (0, 1):
        ev += pkt('ZC_ACK_ITEMREPAIR', index=2, result=result)
    ev += pkt('PACKET_ZC_MERGE_ITEM_OPEN', items=b''.join(pack_sub('PACKET_ZC_MERGE_ITEM_OPEN_sub', index=i)
                                                          for i in (2, 3)))
    for reason in (0, 1, 2):
        ev += pkt('PACKET_ZC_ACK_MERGE_ITEM', index=2, amount=10, reason=reason)
    for percent in (50, 90, 0):
        ev += pkt('PACKET_ZC_OVERWEIGHT_PERCENT', percent=percent)
    ev += pkt('ZC_DELETE_ITEM_FROM_CART', index=2, amount=1)
    ev += pkt('ZC_DELETEITEM_FROM_MCSTORE2', index=2, amount=1, GID=PC_GID + 70, date=0, zeny=500)
    for status in (0, 1):
        ev += pkt('PACKET_ZC_BROADCAST_ITEMREFINING_RESULT', name='Smith', itemId=1201, refine_level=10,
                  status=status)
    return ev


def combat_notices(ctx):
    """What a fight shows besides the hits: Blade Stop held and let go, a combo's delay, the MVP effect,
    Gospel's messages, an effect taken off, a Talkie Box, a pet's emotion, another player's manner and
    stats, a sprite changed."""
    mob, pc = MOB_ID + 1, PC_ID + 80
    ev = new_mob(ctx, mob)
    ev += new_pc(ctx, pc, 'Fighter', job=15, dx=-1, dy=-1)
    ev += pkt('ZC_BLADESTOP', srcAID=mob, dstAID=ctx.aid, active=1)
    ev += pkt('ZC_BLADESTOP', srcAID=mob, dstAID=ctx.aid, active=0)
    ev += pkt('ZC_COMBODELAY', AID=ctx.aid, wait=1500)
    ev += pkt('ZC_COMBODELAY', AID=pc, wait=1000)
    ev += pkt('ZC_MVP', AID=ctx.aid)
    ev += pkt('ZC_MVP', AID=pc)
    for t in (0x15, 0x16, 0x17, 0x1a, 0x1f, 0x20, 0x77):
        ev += pkt('ZC_SKILLMSG', type=t)
    ev += pkt('PACKET_ZC_REMOVE_EFFECT', aid=ctx.aid, effectId=1)
    ev += pkt('PACKET_ZC_TALKBOX_CHATCONTENTS', aid=MOB_ID + 51, message=b'Talkie box!')
    ev += pkt('ZC_PET_ACT', AID=mob, param=3)
    ev += pkt('ZC_PAR_CHANGE_USER', AID=pc, type=4, value=-5)
    ev += pkt('ZC_PAR_CHANGE_USER', AID=pc, type=0x18, value=1)
    ev += pkt('ZC_NPCSPRITE_CHANGE', AID=mob, type=0, job=1113)
    ev += pkt('ZC_NPCSPRITE_CHANGE', AID=ctx.npcs[0], type=0, job=111)
    return ev


def map_marks(ctx):
    """The map's PvP and other properties, NPC marks on the minimap (set and taken off), the camera
    script command, the UI openers (bank, stylist, macro detector, tip box, a quest's renewal window) and
    the dress room's."""
    ev = []
    for t in (1, 2, 3, 0):
        ev += pkt('ZC_NOTIFY_MAPPROPERTY2', type=t)
    for kind, mark, color in ((1, 1, 0xff0000), (1, 2, 0x00ff00), (0, 1, 0), (2, 1, 0xffff00)):
        ev += pkt('ZC_COMPASS', npcId=ctx.npcs[2], type=kind, x=ctx.x + 10, y=ctx.y - 10, id=mark, color=color)
    ev += pkt('PACKET_ZC_VIEW_CAMERAINFO', action=1, range=300.0, rotation=45.0, latitude=-50.0)
    ev += pkt('PACKET_ZC_VIEW_CAMERAINFO', action=0, range=0.0, rotation=0.0, latitude=0.0)
    for ui in (0, 1, 3, 5):
        ev += pkt('PACKET_ZC_UI_OPEN', UIType=ui, data=0)
    ev += pkt('PACKET_ZC_UI_OPEN', UIType=6, data=1000)
    ev += pkt('PACKET_ZC_UI_OPEN2', UIType=5, data=0)
    ev += pkt('ZC_DRESSROOM_OPEN', view=1)
    return ev


def captcha_register_ui(ctx):
    """The macro detector's registration window (ZC_UI_OPEN type 2, the answer to CZ_OPEN_UI 2) and the
    answers to an upload from it."""
    ev = pkt('PACKET_ZC_UI_OPEN', UIType=2, data=0)
    for flag in (0, 1):
        ev += pkt('PACKET_ZC_ACK_UPLOAD_MACRO_DETECTOR', captchaKey=b'U\x01\x02\x03', captchaFlag=flag)
    ev += pkt('PACKET_ZC_COMPLETE_UPLOAD_MACRO_DETECTOR_CAPTCHA')
    return ev


def whispers(ctx):
    """The answers to /ex and /in for one name and for everyone, the block list, a name looked up by
    character id (found and not), and @check's status window."""
    ev = []
    for t in (0, 1):
        for result in (0, 1, 2):
            ev += pkt('ZC_SETTING_WHISPER_PC', type=t, result=result)
            ev += pkt('ZC_SETTING_WHISPER_STATE', type=t, result=result)
    names = b''.join(name24(n).ljust(24, b'\0') for n in ('Blocked1', 'Blocked2'))
    ev += pkt('ZC_WHISPER_LIST', names=names)
    ev += pkt('ZC_WHISPER_LIST', names=b'')
    ev += pkt('ZC_ACK_REQNAME_BYGID2', flag=3, GID=PC_GID + 90, name='Looked Up')
    ev += pkt('ZC_ACK_REQNAME_BYGID2', flag=2, GID=PC_GID + 91, name='')
    ev += pkt('ZC_ACK_STATUS_GM', str=50, needStr=6, agi=30, needAgi=4, vit=20, needVit=3, int=9, needInt=2,
              dex=40, needDex=5, luk=1, needLuk=2, atk=120, atk2=30, matkMax=20, matkMin=10, hit=100, flee=90,
              aspd=150)
    return ev


def guild_requests(ctx):
    """A guild invitation and its answers, an alliance request and its answers, an antagonist answer, a
    member's position changed, members logging in and out, members' places on the minimap, the position
    names over players."""
    pc = PC_ID + 100
    ev = new_pc(ctx, pc, 'Guildy', job=7)
    ev += pkt('ZC_REQ_JOIN_GUILD', guildId=55, name='Some Guild')
    for result in (0, 1, 2, 3):
        ev += pkt('ZC_ACK_REQ_JOIN_GUILD', result=result)
    ev += pkt('ZC_REQ_ALLY_GUILD', AID=pc, name='Ally Guild')
    for result in (0, 1, 2, 3, 4):
        ev += pkt('ZC_ACK_REQ_ALLY_GUILD', result=result)
    for result in (0, 1, 2):
        ev += pkt('ZC_ACK_REQ_HOSTILE_GUILD', result=result)
    ev += pkt('ZC_ACK_REQ_CHANGE_MEMBERS', AID=pc, GID=PC_GID + 100, position=3)
    ev += pkt('ZC_UPDATE_CHARSTAT2', AID=pc, GID=PC_GID + 100, status=1, sex=1, hair=3, hairColor=2)
    ev += pkt('ZC_UPDATE_CHARSTAT2', AID=pc, GID=PC_GID + 100, status=0)
    ev += pkt('ZC_NOTIFY_POSITION_TO_GUILDM', AID=pc, x=ctx.x + 20, y=ctx.y)
    ev += pkt('ZC_NOTIFY_POSITION_TO_GUILDM', AID=pc, x=-1, y=-1)
    ev += pkt('PACKET_ZC_GUILD_POSITION', AID=pc, position=b'Master')
    ev += pkt('PACKET_ZC_GUILD_POSITION', AID=ctx.aid, position=b'Recruit')
    return ev


def marriage_requests(ctx):
    """A marriage proposal, the ceremony's start, a divorce."""
    pc = PC_ID + 110
    ev = new_pc(ctx, pc, 'Suitor')
    ev += pkt('ZC_REQ_COUPLE', AID=pc, GID=PC_GID + 110, name='Suitor')
    ev += pkt('ZC_START_COUPLE')
    ev += pkt('ZC_DIVORCE', name='Suitor')
    return ev


def agency_requests(ctx):
    """The adventurer agency: a request to join the player's party and the answers to the player's own."""
    pc = PC_ID + 120
    ev = new_pc(ctx, pc, 'Joiner', job=2)
    ev += pkt('PACKET_ZC_ADVENTURER_AGENCY_JOIN_REQ', GRID=77, AID=pc, groupName='Joiner', level=55, job=2)
    for result in (0, 1, 2, 3, 4, 5, 6, 7):
        ev += pkt('PACKET_ZC_ADVENTURER_AGENCY_JOIN_RESULT', player_name='Leader', party_name='Their Party', AID=pc,
                  result=result)
    return ev


def cash_shop_open(ctx):
    """The item mall opened with points, on each tab."""
    ev = []
    for tab in (0, 1, 7):
        ev += pkt('PACKET_ZC_SE_CASHSHOP_OPEN', cashPoints=1000, kafraPoints=20, tab=tab)
    return ev


def mail_guild_bank(ctx):
    """Answers the player waits on: a whisper that failed for each reason, mail zeny and items taken, a
    mail deleted, an item taken back off a mail being written, guild members expelled and leaving, guild
    position changes and a relation ended, the guild disbanded, bank withdrawals, the stylist's answer."""
    ev = []
    for result in (1, 2, 3, 4):
        ev += pkt('packet_wis_end', result=result, AID=ctx.aid)
    for result in (0, 1, 2):
        ev += pkt('PACKET_ZC_ACK_ZENY_FROM_MAIL', MailID=12345, opentype=0, result=result)
        ev += pkt('PACKET_ZC_ACK_ITEM_FROM_MAIL', MailID=12345, opentype=0, result=result)
    ev += pkt('PACKET_ZC_ACK_DELETE_MAIL', opentype=0, MailID=12345)
    for result in (0, 1):
        ev += pkt('PACKET_ZC_ACK_REMOVE_ITEM_MAIL', result=result, index=2, cnt=1, weight=10)
    ev += pkt('PACKET_ZC_ACK_BAN_GUILD3', reason=b'Too quiet', GID=PC_GID + 130)
    ev += pkt('PACKET_ZC_ACK_LEAVE_GUILD2', GID=PC_GID + 131, reason=b'Moving on')
    ev += pkt('ZC_ACK_CHANGE_GUILD_POSITIONINFO', positionId=3, mode=0x11, ranking=3, payRate=5, name='Officer')
    ev += pkt('ZC_DELETE_RELATED_GUILD', guildId=56, relation=0)
    ev += pkt('ZC_DELETE_RELATED_GUILD', guildId=57, relation=1)
    for result in (0, 1, 2):
        ev += pkt('ZC_ACK_DISORGANIZE_GUILD_RESULT', result=result)
    for reason in (0, 1, 2):
        ev += pkt('packet_banking_withdraw_ack', Reason=reason, Money=50000, Balance=100000)
    for flag in (0, 1):
        ev += pkt('PACKET_ZC_STYLE_CHANGE_RES', flag=flag)
    return ev


def script_extras(ctx):
    """Script effects: a progress bar over a unit, hunting counts, an effect with a value, an item's
    options changed; then the ranking window's lists and a guild castle's answers."""
    npc = ctx.npcs[0]
    ev = pkt('ZC_PROGRESS_ACTOR', GID=npc, color=0x00ff00, time=3)
    ev += pkt('ZC_PROGRESS_ACTOR', GID=ctx.aid, color=0xff0000, time=2)
    ev += pkt('packet_quest_hunt_info', info=pack_sub('packet_quest_hunt_info_sub', questID=1000, mob_id=1002,
                                                      maxCount=10, count=4))
    ev += pkt('PACKET_ZC_NOTIFY_EFFECT3', aid=ctx.aid, effectId=1, num=1000)
    ev += pkt('PACKET_ZC_CHANGE_ITEM_OPTION', index=2, isDamaged=0, refiningLevel=4,
              **{'option_data[0].index': 1, 'option_data[0].value': 15})
    for rank in (0, 1, 2):
        ev += pkt('PACKET_ZC_ACK_RANKING', rankType=rank, myPoints=7,
                  **{'chars[0]': PC_GID + 140, 'points[0]': 100, 'chars[1]': PC_GID + 141, 'points[1]': 50})
    ev += pkt('PACKET_ZC_GUILD_AGIT_INFO', castle_list=bytes([0, 5, 20, 0xff]))
    ev += pkt('PACKET_ZC_REQ_ACK_AGIT_INVESTMENT', castle_id=5, economy=10, defense=3)
    for result in (0, 1):
        ev += pkt('PACKET_ZC_REQ_ACK_MOVE_GUILD_AGIT', result=result)
    return ev


def markets(ctx):
    """An NPC market's list and the answers to a purchase from it, and a barter shop's list."""
    items = b''.join(pack_sub('PACKET_ZC_NPC_MARKET_OPEN_sub', nameid=i, type=t, price=p, qty=q, weight=w,
                              location=loc) for i, t, p, q, w, loc in ((501, 0, 50, 100, 70, 0),
                                                                        (1201, 5, 500, 2, 400, 2),
                                                                        (909, 3, 2, 0, 1, 0)))
    ev = pkt('PACKET_ZC_NPC_MARKET_OPEN', list=items)
    bought = pack_sub('PACKET_ZC_NPC_MARKET_PURCHASE_RESULT_sub', ITID=501, qty=5, price=50)
    ev += pkt('PACKET_ZC_NPC_MARKET_PURCHASE_RESULT', result=1, list=bought)
    ev += pkt('PACKET_ZC_NPC_MARKET_PURCHASE_RESULT', result=0, list=b'')
    barter = b''.join(pack_sub('PACKET_ZC_NPC_BARTER_MARKET_ITEMINFO_sub', nameid=i, type=t, amount=a,
                               currencyNameid=c, currencyAmount=ca, weight=w, index=k, viewSprite=0, location=loc)
                      for i, t, a, c, ca, w, k, loc in ((501, 0, 10, 909, 5, 70, 0, 0), (1201, 5, 1, 909, 50, 400, 1, 2)))
    ev += pkt('PACKET_ZC_NPC_BARTER_MARKET_ITEMINFO', list=barter)
    return ev


def _captcha_image():
    """A 220x90 8-bit bitmap the macro detector shows (two colours in stripes), as the server stores it:
    zlib-compressed."""
    import zlib
    width, height, colours = 220, 90, 2
    stride = (width + 3) & ~3
    palette = bytes([0, 0, 0, 0, 255, 255, 255, 0])
    pixels = b''.join(bytes([(x // 11 + y // 9) & 1 for x in range(width)]) + bytes(stride - width)
                      for y in range(height))
    offset = 54 + len(palette)
    header = (b'BM' + struct.pack('<IHHI', offset + len(pixels), 0, 0, offset)
              + struct.pack('<IiiHHIIiiII', 40, width, height, 1, 8, 0, len(pixels), 0, 0, colours, 0))
    return zlib.compress(header + palette + pixels)


def macro_detector(ctx):
    """The macro detector shown to the player: its picture sent, the request to answer, the answer's
    results, a second request, the window closed as answered. (The client leaves the map server a few
    hundred frames after a detector closes, so the other endings are scripts of their own.)"""
    image = _captcha_image()
    key = b'K\x01\x02\x03'
    ev = pkt('PACKET_ZC_APPLY_MACRO_DETECTOR', imageSize=len(image), captchaKey=key)
    for i in range(0, len(image), 1024):
        ev += pkt('PACKET_ZC_APPLY_MACRO_DETECTOR_CAPTCHA', captchaKey=key, imageData=image[i:i + 1024])
    ev += pkt('PACKET_ZC_REQ_ANSWER_MACRO_DETECTOR', retryCount=3, timeout=60000)
    for status in (0, 1):
        ev += pkt('PACKET_ZC_ACK_APPLY_MACRO_DETECTOR', status=status)
    ev += pkt('PACKET_ZC_REQ_ANSWER_MACRO_DETECTOR', retryCount=2, timeout=30000)
    ev += pkt('PACKET_ZC_CLOSE_MACRO_DETECTOR', status=0)
    return ev


def macro_detector_short(ctx):
    """The macro detector's picture cut short: fewer bytes than announced, then closed."""
    image = _captcha_image()
    key = b'Q\x09\x08\x07'
    ev = pkt('PACKET_ZC_APPLY_MACRO_DETECTOR', imageSize=len(image), captchaKey=key)
    ev += pkt('PACKET_ZC_APPLY_MACRO_DETECTOR_CAPTCHA', captchaKey=key, imageData=image[:100])
    ev += pkt('PACKET_ZC_REQ_ANSWER_MACRO_DETECTOR', retryCount=3, timeout=60000)
    ev += pkt('PACKET_ZC_CLOSE_MACRO_DETECTOR', status=1)
    return ev


def macro_detector_damaged(ctx):
    """The macro detector's picture damaged (its compressed bytes do not inflate), then closed."""
    image = bytearray(_captcha_image())
    image[10:20] = bytes(10)
    key = b'K\x01\x02\x03'
    ev = pkt('PACKET_ZC_APPLY_MACRO_DETECTOR', imageSize=len(image), captchaKey=key)
    ev += pkt('PACKET_ZC_APPLY_MACRO_DETECTOR_CAPTCHA', captchaKey=key, imageData=bytes(image))
    ev += pkt('PACKET_ZC_REQ_ANSWER_MACRO_DETECTOR', retryCount=3, timeout=60000)
    ev += pkt('PACKET_ZC_CLOSE_MACRO_DETECTOR', status=2)
    return ev


def macro_preview(ctx):
    """The macro detector's preview of a registered picture (a GM's), whole and with a key that does not
    match."""
    image = _captcha_image()
    key = b'P\x01\x02\x03'
    ev = pkt('PACKET_ZC_ACK_PREVIEW_MACRO_DETECTOR', captchaFlag=0, imageSize=len(image), captchaKey=key)
    for i in range(0, len(image), 1024):
        ev += pkt('PACKET_ZC_PREVIEW_MACRO_DETECTOR_CAPTCHA', captchaKey=key, imageData=image[i:i + 1024])
    ev += pkt('PACKET_ZC_PREVIEW_MACRO_DETECTOR_CAPTCHA', captchaKey=b'XXXX', imageData=image[:64])
    ev += pkt('PACKET_ZC_ACK_PREVIEW_MACRO_DETECTOR', captchaFlag=1, imageSize=0, captchaKey=key)
    return ev


# Option bits (effectState) Hercules sends for a unit's look (src/map/status.h OPTION_*), the body states
# (bodyState: stone, freeze, stun, sleep, stone wait) and health states (healthState: poison, curse, silence,
# confusion, blind, bleeding, deadly poison).
OPTIONS = [0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000,
           0x10000, 0x20000, 0x40000, 0x80000, 0x20 | 0x8, 0x10 | 0x8]
BODY_STATES = [1, 2, 3, 4, 6]
HEALTH_STATES = [0x1, 0x2, 0x4, 0x8, 0x10, 0x40, 0x80, 0x1 | 0x10]
# Riding classes as Hercules sends them (the Peco Knight, Crusader, Lord Knight and Paladin jobs, and the
# baby ones), and others for the cart and falcon bits (Merchant, Blacksmith, Hunter, Sniper).
MOUNT_JOBS = [13, 21, 4014, 4022, 4036, 4044]
ALL_RIDING = 0x265      # EFST_ALL_RIDING, the mount any class can ride (SC_ALL_RIDING)
OTHER_JOBS = [5, 10, 11, 4012, 4019]


def appearance(ctx):
    """Players' looks from their option bits, body and health states: riding classes entering mounted
    and dismounting, carts, falcons, wedding and costume looks, hiding and cloaking, the any-class mount
    on and off; then the player's own mount, option, body and health states set and cleared."""
    ev = []
    for n, job in enumerate(MOUNT_JOBS):
        ev += new_pc(ctx, PC_ID + 150 + n, f'Rider{n}', job=job, dx=-4 + 2 * (n % 4), dy=-3 + 2 * (n // 4))
        ev[-2]['attrs']['opt'] = (0, 0, 0x20)
    for n, job in enumerate(MOUNT_JOBS):
        ev += pkt('PACKET_ZC_STATE_CHANGE', AID=PC_ID + 150 + n, effectState=0)
        ev += pkt('PACKET_ZC_STATE_CHANGE', AID=PC_ID + 150 + n, effectState=0x20 | 0x8)
    for n, job in enumerate(OTHER_JOBS):
        ev += new_pc(ctx, PC_ID + 160 + n, f'Looks{n}', job=job, dx=3, dy=-3 + 2 * n)
    for n, opt in enumerate(OPTIONS):
        ev += pkt('PACKET_ZC_STATE_CHANGE', AID=PC_ID + 160 + n % len(OTHER_JOBS), effectState=opt)
    for body in BODY_STATES:
        ev += pkt('PACKET_ZC_STATE_CHANGE', AID=PC_ID + 160, bodyState=body)
    for health in HEALTH_STATES:
        ev += pkt('PACKET_ZC_STATE_CHANGE', AID=PC_ID + 161, healthState=health)
    for n, job in enumerate(OTHER_JOBS[:3]):
        ev += pkt('packet_status_change', index=ALL_RIDING, AID=PC_ID + 160 + n, state=1, Total=0xffffffff,
                  Left=0xffffffff, val1=1)
    ev += pkt('packet_status_change_end', index=ALL_RIDING, AID=PC_ID + 160, state=0)
    ev += pkt('packet_status_change', index=ALL_RIDING, AID=ctx.aid, state=1, Total=0xffffffff, Left=0xffffffff,
              val1=1)
    ev += pkt('packet_status_change_end', index=ALL_RIDING, AID=ctx.aid, state=0)
    for opt in (0x20, 0x8, 0x10, 0x1000, 0x10000, 0x2, 0x4, 0):
        ev += pkt('PACKET_ZC_STATE_CHANGE', AID=ctx.aid, effectState=opt)
    for body in BODY_STATES + [0]:
        ev += pkt('PACKET_ZC_STATE_CHANGE', AID=ctx.aid, bodyState=body)
    for health in HEALTH_STATES + [0]:
        ev += pkt('PACKET_ZC_STATE_CHANGE', AID=ctx.aid, healthState=health)
    return ev


# Every effect id Hercules' scripts and item scripts show with specialeffect (npc/, db/pre-re/; EF_ names
# through db/constants.conf).
SPECIAL_EFFECTS = [0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21, 22, 23, 24, 25, 27, 28,
                   29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 47, 49, 50, 52, 53, 54, 55, 56,
                   57, 58, 59, 60, 62, 63, 65, 66, 68, 70, 71, 72, 73, 75, 77, 79, 81, 83, 84, 86, 87, 88, 90, 91,
                   92, 94, 96, 97, 98, 99, 100, 101, 102, 106, 107, 108, 109, 110, 112, 113, 118, 120, 121, 122,
                   124, 125, 127, 131, 133, 134, 135, 139, 141, 144, 148, 152, 159, 169, 171, 172, 174, 175, 179,
                   180, 182, 183, 186, 187, 188, 189, 190, 192, 193, 194, 196, 197, 200, 203, 204, 209, 210, 212,
                   215, 216, 220, 224, 225, 231, 234, 235, 238, 244, 245, 247, 253, 254, 256, 258, 267, 276, 295,
                   296, 298, 304, 305, 306, 307, 312, 313, 314, 316, 317, 320, 322, 324, 328, 335, 336, 338, 342,
                   343, 348, 359, 360, 361, 362, 363, 364, 366, 371, 372, 373, 376, 377, 386, 410, 417, 432, 462,
                   501, 548, 561, 563, 585, 588, 610, 611, 626, 699, 709, 725, 800, 805, 877, 882, 889, 890, 901,
                   934, 995, 1064, 1068]


def special_effects(ctx):
    """Every special effect the server's scripts and items show, by turns on the player and on an NPC, a
    state after every eight."""
    ev = []
    for n, effect in enumerate(SPECIAL_EFFECTS):
        target = ctx.aid if n % 2 == 0 else ctx.npcs[n % 4]
        ev += pkt('ZC_NOTIFY_EFFECT2', _gap=6, _state=(n % 8 == 7), AID=target, effectId=effect)
    ev.append({'k': 'state', 'gap': STATE_GAP})
    return ev


def battleground(ctx):
    """A battleground (the server runs Krieger von Midgard): team chat, the team marks over players, the
    score, team mates on the minimap and their HP, a mate leaving the minimap."""
    mates = [(PC_ID + 170 + n, f'Mate{n}', job) for n, job in enumerate((4008, 4010, 4012))]
    ev = []
    for n, (aid, name, job) in enumerate(mates):
        ev += new_pc(ctx, aid, name, job=job, dx=-3 + 3 * n, dy=4)
    for aid, name, _ in mates[:2]:
        ev += pkt('ZC_BATTLEFIELD_NOTIFY_CAMPINFO', AID=aid, name=name, camp=1)
    ev += pkt('ZC_BATTLEFIELD_NOTIFY_CAMPINFO', AID=mates[2][0], name=mates[2][1], camp=2)
    ev += pkt('ZC_BATTLEFIELD_NOTIFY_CAMPINFO', AID=ctx.aid, name='parity04', camp=1)
    ev += pkt('ZC_BATTLEFIELD_CHAT', AID=mates[0][0], name=mates[0][1], message=b'Mate0 : go left\0')
    ev += pkt('ZC_BATTLEFIELD_CHAT', AID=ctx.aid, name='parity04', message=b'parity04 : on it\0')
    for lion, eagle in ((0, 0), (1, 0), (1, 2)):
        ev += pkt('ZC_BATTLEFIELD_NOTIFY_POINT', lion=lion, eagle=eagle)
    for n, (aid, name, job) in enumerate(mates[:2]):
        ev += pkt('ZC_BATTLEFIELD_NOTIFY_POSITION', AID=aid, name=name, job=job, x=ctx.x + 30 * (n + 1), y=ctx.y - 20)
        ev += pkt('PACKET_ZC_BATTLEFIELD_NOTIFY_HP', AID=aid, hp=3000 - 1000 * n, maxhp=5000)
    ev += pkt('ZC_BATTLEFIELD_NOTIFY_POSITION', AID=mates[1][0], name=b'', job=0, x=-1, y=-1)
    return ev


def party_booking(ctx):
    """Party booking: the answers to registering, searching (with ads) and deleting, and ads inserted,
    updated and deleted while the list is open."""
    ads = b''.join(pack_sub('ZC_PARTY_BOOKING_ACK_SEARCH_ad', index=i, name=n, expire=0x7fffffff, level=lv, mapId=m,
                            job0=j, job1=-1, job2=-1, job3=-1, job4=-1, job5=-1)
                   for i, n, lv, m, j in ((1, 'Booker1', 50, 1, 7), (2, 'Booker2', 80, 3, 15)))
    ev = []
    for result in (0, 1, 2):
        ev += pkt('ZC_PARTY_BOOKING_ACK_REGISTER', result=result)
    ev += pkt('ZC_PARTY_BOOKING_ACK_SEARCH', more=0, ads=ads)
    ev += pkt('ZC_PARTY_BOOKING_ACK_SEARCH', more=1, ads=b'')
    ev += pkt('ZC_PARTY_BOOKING_NOTIFY_INSERT', index=3, name='Booker3', expire=0x7fffffff, level=60, mapId=2, job0=4,
              job1=-1, job2=-1, job3=-1, job4=-1, job5=-1)
    ev += pkt('ZC_PARTY_BOOKING_NOTIFY_UPDATE', index=3, job0=5, job1=8, job2=-1, job3=-1, job4=-1, job5=-1)
    ev += pkt('ZC_PARTY_BOOKING_NOTIFY_DELETE', index=3)
    for result in (0, 1):
        ev += pkt('ZC_PARTY_BOOKING_ACK_DELETE', result=result)
    return ev


def personal_and_range(ctx):
    """The personal information window's bonuses (premium, server and PC room rates) and the players in
    range the macro reporter lists, with an expanded barter shop's list."""
    details = b''.join(pack_sub('PACKET_ZC_PERSONAL_INFOMATION_SUB', type=t, exp=e, death=d, drop=r)
                       for t, e, d, r in ((0, 100, 0, 0), (1, 50, 0, 100), (2, 0, 0, 0)))
    ev = pkt('PACKET_ZC_PERSONAL_INFOMATION', total_exp=150, total_death=0, total_drop=100, details=details)
    pc = PC_ID + 180
    ev += new_pc(ctx, pc, 'Nearby')
    ev += pkt('PACKET_ZC_ACK_PLAYER_AID_IN_RANGE', AID=struct.pack('<2I', pc, ctx.aid))
    ev += pkt('PACKET_ZC_ACK_PLAYER_AID_IN_RANGE', AID=b'')
    items = b''.join(pack_sub('PACKET_ZC_NPC_EXPANDED_BARTER_MARKET_ITEMINFO_sub', nameid=i, type=t, amount=a,
                              weight=w, index=k, zeny=z, viewSprite=0, location=loc, currency_count=1,
                              **{'currencies[0].nameid': 909, 'currencies[0].amount': 10})
                     for i, t, a, w, k, z, loc in ((501, 0, 10, 70, 0, 100, 0), (1201, 5, 1, 400, 1, 0, 2)))
    ev += pkt('PACKET_ZC_NPC_EXPANDED_BARTER_MARKET_ITEMINFO', items_count=2, items=items)
    return ev


def texts(ctx):
    """The packets that carry what players and NPCs say, and names: another player's chat line, the
    player's own, a whisper, broadcasts, guild chat, an NPC's coloured talk, a vendor's shop sign, a
    player's and an NPC's names asked for, a pet's name, the guild notice, an NPC dialog and its menu."""
    pc = PC_ID + 200
    npc = ctx.npcs[0]
    ev = new_pc(ctx, pc, 'Speaker')
    ev += pkt('PACKET_ZC_NOTIFY_CHAT', GID=pc, Message=b'Speaker : hello there\0')
    ev += pkt('PACKET_ZC_NOTIFY_PLAYERCHAT', Message=b'parity04 : my own line\0')
    ev += pkt('PACKET_ZC_WHISPER', senderGID=PC_GID + 200, sender='Speaker', isAdmin=0, message=b'a whisper\0')
    ev += pkt('ZC_BROADCAST', message=b'A broadcast line\0')
    ev += pkt('ZC_BROADCAST', message=b'blueA blue broadcast\0')
    ev += pkt('ZC_GUILD_CHAT', message=b'Member : a guild line\0')
    ev += pkt('ZC_NPC_CHAT', AID=npc, color=0x00ff00, message=b'Guide : coloured talk\0')
    ev += pkt('PACKET_ZC_STORE_ENTRY', makerAID=pc, storeName='Cheap potions')
    ev += pkt('PACKET_ZC_ACK_REQNAMEALL', gid=pc, name='Speaker', party_name='Their Party',
              guild_name='Their Guild', position_name='Member', title_id=0)
    ev += pkt('PACKET_ZC_ACK_REQNAMEALL_NPC', gid=npc, groupId=0, name='Guide', title='The Guide')
    ev += pkt('ZC_DISAPPEAR_ENTRY', AID=pc)
    ev += pkt('PACKET_ZC_PROPERTY_PET', szName='MyPet', bModified=0, nLevel=10, nFullness=50, nRelationship=250,
              ITID=0, job=1002)
    ev += pkt('ZC_GUILD_NOTICE', subject='Notice subject', notice='Notice body')
    ev += pkt('PACKET_ZC_SAY_DIALOG', NpcID=npc, message=b'Hello, traveller.\0')
    ev += pkt('ZC_MENU_LIST', npcId=npc, message=b'Yes:No:Maybe\0')
    ev += pkt('packet_script_clear', NpcID=npc)
    return ev


def ragexe_texts(ctx):
    """The Ragexe Saga server's own packets that carry text: the whisper history replayed at login (a
    whisper received, one sent, the end), a coloured line, and the friends panel's details of a friend
    list."""
    ev = pkt('PACKET_ZC_WHISPER_REPLAY', direction=0, isAdmin=0, gid=PC_GID + 300, name='Whisperer',
             message=b'an old whisper\0')
    ev += pkt('PACKET_ZC_WHISPER_REPLAY', direction=1, isAdmin=0, gid=PC_GID + 300, name='Whisperer',
              message=b'my old answer\0')
    ev += pkt('PACKET_ZC_WHISPER_REPLAY', direction=2, isAdmin=0, gid=0, name='', message=b'\0')
    ev += pkt('PACKET_ZC_COLOR_CHAT', color=0x0000ff, message=b'^ff0000Red^000000 and plain\0')
    fr = [(PC_ID + 310 + i, PC_GID + 310 + i, f'Pal{i}') for i in range(2)]
    ev += pkt('ZC_FRIENDS_LIST', friends=b''.join(pack_sub('ZC_FRIENDS_LIST_friend', AID=a, GID=g, name=n)
                                                  for a, g, n in fr))
    ev += pkt('PACKET_ZC_FRIENDS_INFO', offlineChar=0, offlineAccount=0,
              friends=b''.join(pack_sub('PACKET_ZC_FRIENDS_INFO_sub', AID=a, GID=g, baseLevel=50, jobLevel=20,
                                        job='Swordman', listed=n) for a, g, n in fr))
    return ev


def clan_guild(ctx):
    """A clan's information with an ally and an antagonist, its chat, and the guild's own information."""
    ev = pkt('PACKET_ZC_CLANINFO', ClanID=3, ClanName='Golden Mace Clan', MasterName='Raffam Oranpere',
             Map='prontera', AllyCount=1, AntagonistCount=1, names=name24('Crumbling Moon').ljust(24, b'\0')
             + name24('Sword Clan').ljust(24, b'\0'))
    ev += pkt('PACKET_ZC_NOTIFY_CLAN_CHAT', MemberName='ClanMate', Message=b'ClanMate : hello clan\0')
    ev += pkt('PACKET_ZC_GUILD_INFO', GDID=77, level=10, userNum=3, maxUserNum=16, userAverageLevel=50,
              guildname='Parity Guild', manageLand='prt_gld', masterGID=ctx.gid, masterName='parity04')
    return ev


def names(ctx):
    """The other packets that carry names: a party member's details (party, player and map names), a
    unit's name asked for, the answers to a party invitation, the homunculus, the guild the player belongs
    to and its position names, a mail recipient checked and the write window's answer, a grade enchant
    broadcast, an NPC's cut-in picture shown and closed, another player's equipment shown."""
    pc = PC_ID + 400
    ev = new_pc(ctx, pc, 'Joiner', job=1)
    ev += pkt('PACKET_ZC_ADD_MEMBER_TO_GROUP', AID=pc, GID=PC_GID + 400, leader=1, baseLevel=40, x=ctx.x + 2,
              y=ctx.y + 1, offline=0, partyName='Party One', playerName='Joiner', mapName=ctx.map + '.gat',
              **{'class': 1})
    ev += pkt('packet_reqname_ack', gid=pc, name='Joiner')
    for result in (0, 1, 2, 7):
        ev += pkt('PACKET_ZC_PARTY_JOIN_REQ_ACK', characterName='Invitee', result=result)
    ev += pkt('PACKET_ZC_PROPERTY_HOMUN', name='Vanilmirth', flags=0, level=10, hunger=50, intimacy=250, atk2=30,
              matk=40, hit=50, crit=5, flee=20, amotion=500, hp=800, maxHp=1000, sp=50, maxSp=80,
              exp=10, expNext=100, skillPoints=1, range=1)
    ev += pkt('PACKET_ZC_UPDATE_GDID', guildId=77, emblemVersion=0, mode=0x11, isMaster=1, interSid=0,
              guildName='Parity Guild', masterGID=ctx.gid)
    ev += pkt('PACKET_ZC_POSITION_ID_NAME_INFO',
              posInfo=b''.join(pack_sub('PACKET_ZC_POSITION_ID_NAME_INFO_sub', positionID=i, posName=f'Position {i}')
                               for i in range(20)))
    ev += pkt('PACKET_ZC_CHECKNAME', CharId=PC_GID + 401, Class=4, BaseLevel=60, Name='Receiver')
    ev += pkt('PACKET_ZC_ACK_OPEN_WRITE_MAIL', receiveName='Receiver', result=0)
    ev += pkt('PACKET_ZC_GRADE_ENCHANT_BROADCAST_RESULT', name='Enchanter', itemId=1201, grade=2, status=0)
    ev += pkt('PACKET_ZC_SHOW_IMAGE', image='kafra_01', type=2)
    ev += pkt('PACKET_ZC_SHOW_IMAGE', image='', type=255)
    ev += pkt('PACKET_ZC_EQUIPWIN_MICROSCOPE', characterName='Joiner', job=1, head=2, sex=1, list=b'')
    return ev


SCRIPTS = {f.__name__: f for f in (chat_room_member, chat_room_owner, vending_board, trade, friends, party,
                                   messages, skills, quests, npc_script, items, marriage, rankings,
                                   memorial_dungeon, buying_store, cash_shop, item_services, combat_notices,
                                   map_marks, whispers, guild_requests, marriage_requests, agency_requests,
                                   cash_shop_open, captcha_register_ui, mail_guild_bank, script_extras, markets,
                                   macro_detector, macro_preview, macro_detector_short, macro_detector_damaged,
                                   appearance, special_effects, battleground, party_booking, personal_and_range,
                                   texts, clan_guild, ragexe_texts, names)}


def events(names, prefix):
    ctx = Ctx(prefix)
    out = []
    for n in names:
        out += SCRIPTS[n](ctx)
    return out


def script_ids(name, prefix):
    """The packet ids a script sends."""
    return {int(e['data'][2:4] + e['data'][0:2], 16) for e in events([name], prefix) if e['k'] == 'pkt'}


# ---- choosing scripts by handler ------------------------------------------------------------------

DISPATCHERS = ('src/game/packet_dispatch.c', 'src/game/packet_dispatch2.c')


def header_values(tree):
    """{HEADER_ name: id} from the #defines and enums under src/."""
    out = {}
    for root, _, files in os.walk(os.path.join(tree, 'src')):
        for f in files:
            if f.endswith('.h'):
                text = open(os.path.join(root, f), encoding='utf-8', errors='replace').read()
                for m in re.finditer(r'(HEADER_\w+)\s*(?:=\s*|\s+)(0x[0-9a-fA-F]+)', text):
                    out.setdefault(m.group(1), int(m.group(2), 16))
    return out


def handler_ids(tree):
    """{handler function: {ids}} from the dispatchers' `case HEADER_X: handler(...)` arms."""
    vals = header_values(tree)
    out = {}
    for d in DISPATCHERS:
        text = open(os.path.join(tree, d), encoding='utf-8', errors='replace').read()
        for m in re.finditer(r'((?:case\s+\w+\s*:\s*)+)(\w+)\s*\(', text):
            for h in re.findall(r'case\s+(\w+)\s*:', m.group(1)):
                if h in vals:
                    out.setdefault(m.group(2), set()).add(vals[h])
    return out


def target_ids(targets, tree):
    vals = header_values(tree)
    by_handler = handler_ids(tree)
    ids = set()
    for t in targets:
        if t.startswith('0x'):
            ids.add(int(t, 16))
        elif t in by_handler:
            ids |= by_handler[t]
        elif t in vals:
            ids.add(vals[t])
        elif t in seqgen.LAYOUTS:
            ids.add(seqgen.LAYOUTS[t][0])
        else:
            raise KeyError(f'{t}: not a handler, HEADER_ name, layout or id')
    return ids


def scripts_for(targets, tree, prefix):
    """The scripts that send one of the targets' packets, in SCRIPTS order, and the target ids no script
    sends."""
    want = target_ids(targets, tree)
    chosen, sent = [], set()
    for name in SCRIPTS:
        ids = script_ids(name, prefix)
        if ids & want:
            chosen.append(name)
            sent |= ids
    return chosen, sorted(want - sent)
