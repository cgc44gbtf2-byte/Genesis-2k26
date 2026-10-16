/* Exercise the real kill/multi_hit path with stale selected styles.
 * No player records or running server are used.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"

extern void init_mm(void);

static void ready(CHAR_DATA *ch, CHAR_DATA *victim, DESCRIPTOR_DATA *desc)
{
    stop_fighting(ch, TRUE);
    ch->fight_timer = victim->fight_timer = 0;
    ch->position = victim->position = POS_STANDING;
    ch->wait = 0;
    desc->outtop = 0;
    desc->outbuf[0] = '\0';
}

static void attacked(CHAR_DATA *ch, CHAR_DATA *victim, DESCRIPTOR_DATA *desc,
                     long expected)
{
    do_kill(ch, "lizard");
    assert(ch->style[0] == expected);
    assert(ch->fighting == victim && victim->fighting == ch);
    assert(strstr(desc->outbuf, "bug alert") == NULL);
}

int main(void)
{
    CHAR_DATA actor, target;
    PC_DATA pc;
    ROOM_INDEX_DATA room;
    AREA_DATA area;
    MOB_INDEX_DATA mob;
    DESCRIPTOR_DATA desc;
    OBJ_DATA weapon, hold;
    char output[65536];
    long stale[] = {0, STYLE_ONEHAND, STYLE_TWOHAND, STYLE_TWOWEAPON,
                    STYLE_NATURAL, 999};
    size_t n;
    int i;

    init_mm();
    for (i = 0; i < MAX_SKILL && skill_table[i].name != NULL; ++i)
        if (skill_table[i].pgsn != NULL) *skill_table[i].pgsn = i;
    memset(&actor, 0, sizeof(actor)); memset(&target, 0, sizeof(target));
    memset(&pc, 0, sizeof(pc)); memset(&room, 0, sizeof(room));
    memset(&area, 0, sizeof(area)); memset(&mob, 0, sizeof(mob));
    memset(&desc, 0, sizeof(desc)); memset(&weapon, 0, sizeof(weapon));
    memset(&hold, 0, sizeof(hold)); memset(output, 0, sizeof(output));
    actor.pcdata = &pc;
    actor.name = actor.short_descr = "Tester";
    target.name = target.short_descr = "lizard";
    actor.morph = target.morph = str_empty;
    actor.description = target.description = str_empty;
    actor.long_descr = target.long_descr = str_empty;
    actor.class = CLASS_NONE; actor.level = LEV_SLE;
    target.act = ACT_IS_NPC; target.level = 1; target.pIndexData = &mob;
    actor.in_room = target.in_room = &room;
    actor.hit = actor.max_hit = target.hit = target.max_hit = 1000000;
    actor.mana = actor.max_mana = 100000;
    for (i = 0; i < 5; ++i) actor.stats[i] = target.stats[i] = 5;
    room.area = &area; area.nplayer = 1; room.people = &actor;
    room.name = "test room"; room.sector_type = SECT_FIELD;
    actor.next_in_room = &target; actor.next = &target; char_list = &actor;
    weather_info.sunlight = SUN_LIGHT;
    desc.outbuf = output; desc.outsize = sizeof(output); actor.desc = &desc;

    for (n = 0; n < sizeof(stale) / sizeof(stale[0]); ++n) {
        ready(&actor, &target, &desc);
        actor.style[0] = stale[n];
        attacked(&actor, &target, &desc, STYLE_UNARMED);
    }
    puts("Unclassed kill repairs stale/invalid unarmed styles");

    /* Repair subsequent combat rounds, not only the initial kill command. */
    ready(&actor, &target, &desc);
    actor.style[0] = STYLE_ONEHAND;
    multi_hit(&actor, &target);
    assert(actor.style[0] == STYLE_UNARMED && actor.fighting == &target);

    weapon.item_type = hold.item_type = ITEM_WEAPON;
    weapon.short_descr = "test sword"; hold.short_descr = "offhand sword";
    weapon.value[1] = weapon.value[2] = hold.value[1] = hold.value[2] = 1;
    weapon.value[3] = hold.value[3] = TYPE_SLASH;
    weapon.wear_loc = WEAR_WEAPON; hold.wear_loc = WEAR_HOLD;
    actor.carrying = &weapon;
    ready(&actor, &target, &desc); actor.style[0] = STYLE_UNARMED;
    attacked(&actor, &target, &desc, STYLE_ONEHAND);
    ready(&actor, &target, &desc); actor.style[0] = STYLE_TWOHAND;
    attacked(&actor, &target, &desc, STYLE_TWOHAND);
    weapon.next_content = &hold;
    ready(&actor, &target, &desc); actor.style[0] = STYLE_UNARMED;
    attacked(&actor, &target, &desc, STYLE_TWOWEAPON);
    hold.item_type = ITEM_ARMOR;
    ready(&actor, &target, &desc); actor.style[0] = STYLE_TWOWEAPON;
    attacked(&actor, &target, &desc, STYLE_SHIELD);
    actor.carrying = &hold;
    ready(&actor, &target, &desc); actor.style[0] = STYLE_ONEHAND;
    attacked(&actor, &target, &desc, STYLE_SHIELD);
    hold.item_type = ITEM_WEAPON;
    ready(&actor, &target, &desc); actor.style[0] = STYLE_UNARMED;
    attacked(&actor, &target, &desc, STYLE_ONEHAND);
    assert(hold.wear_loc == WEAR_WEAPON);
    puts("Equipment styles, two-handed grip, shields and offhand promotion preserved");

    actor.carrying = NULL;
    actor.class = CLASS_VAMPIRE; SET_BIT(actor.vampaff, VAM_CLAWS);
    ready(&actor, &target, &desc); actor.style[0] = STYLE_ONEHAND;
    attacked(&actor, &target, &desc, STYLE_NATURAL);
    actor.class = CLASS_NONE; REMOVE_BIT(actor.vampaff, VAM_CLAWS);
    ready(&actor, &target, &desc);
    SET_BIT(room.room_flags, ROOM_SAFE);
    do_kill(&actor, "lizard");
    assert(actor.fighting == NULL && target.fighting == NULL);
    REMOVE_BIT(room.room_flags, ROOM_SAFE);
    SET_BIT(target.act, ACT_NOKILL);
    do_kill(&actor, "lizard");
    assert(actor.fighting == NULL && target.fighting == NULL);
    REMOVE_BIT(target.act, ACT_NOKILL);
    multi_hit(NULL, &target);
    multi_hit(&actor, NULL);
    puts("Natural weapons, safe-room/NPC protections and null guards passed");
    return 0;
}
