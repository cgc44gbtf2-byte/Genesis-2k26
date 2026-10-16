#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"

/* Standalone probe does not pass through boot_db's RNG initialization. */
extern void init_mm(void);

static void invoke(char *name, long level, CHAR_DATA *ch, void *target)
{
    long sn = skill_lookup(name);
    assert(sn > 0 && skill_table[sn].spell_fun != NULL);
    skill_table[sn].spell_fun(sn, level, ch, target);
}

static void strip(char *name, CHAR_DATA *ch)
{
    affect_strip(ch, skill_lookup(name));
}

static void alignment_output(CHAR_DATA *actor, CHAR_DATA *target, char *expected)
{
    DESCRIPTOR_DATA descriptor;
    char output[4096];
    memset(&descriptor, 0, sizeof(descriptor));
    output[0] = '\0';
    descriptor.outbuf = output;
    descriptor.outsize = sizeof(output);
    descriptor.fcommand = TRUE;
    descriptor.character = actor;
    actor->desc = &descriptor;
    invoke("know alignment", 20, actor, target);
    assert(strstr(output, expected) != NULL);
    actor->desc = NULL;
}

int main(void)
{
    CHAR_DATA actor, target;
    PC_DATA actor_pc, target_pc;
    MOB_INDEX_DATA good, evil;
    ROOM_INDEX_DATA room, other;
    OBJ_DATA obj;
    DESCRIPTOR_DATA target_desc;
    char target_output[4096];
    long i, before, original;
    char *armor[] = {"armor", "shield", "stone skin"};
    long bonuses[] = {20, 20, 30};
    char *detection[] = {"detect evil", "detect hidden", "detect magic"};
    memset(&actor, 0, sizeof(actor));
    memset(&target, 0, sizeof(target));
    memset(&actor_pc, 0, sizeof(actor_pc));
    memset(&target_pc, 0, sizeof(target_pc));
    memset(&good, 0, sizeof(good));
    memset(&evil, 0, sizeof(evil));
    memset(&room, 0, sizeof(room));
    memset(&other, 0, sizeof(other));
    memset(&obj, 0, sizeof(obj));
    init_mm();
    for (i = 0; i < MAX_SKILL && skill_table[i].name != NULL; i++)
        if (skill_table[i].pgsn != NULL) *skill_table[i].pgsn = i;
    actor.pcdata = &actor_pc;
    target.pcdata = &target_pc;
    actor.act = target.act = ACT_IS_NPC;
    actor.name = "Actor"; target.name = "Target";
    actor.short_descr = "the actor"; target.short_descr = "the target";
    actor.morph = target.morph = "";
    actor.pIndexData = &good; target.pIndexData = &evil;
    good.alignment = 500; evil.alignment = -1000;
    actor.in_room = target.in_room = &room;
    actor.next_in_room = &target; room.people = &actor;
    actor.position = target.position = POS_STANDING;
    actor.affected_by = target.affected_by = AFF_INFRARED;
    actor.level = target.level = LEV_SLE;
    actor.style[0] = target.style[0] = STYLE_UNARMED;
    actor.hit = target.hit = actor.max_hit = target.max_hit = 1000000;
    actor.mana = target.mana = actor.max_mana = target.max_mana = 10000;
    actor.stats[STAT_STR] = target.stats[STAT_STR] = 5;

    for (i = 0; i < 3; i++)
    {
        invoke(armor[i], 99999, &actor, &actor);
        assert(actor.armor == bonuses[i] && actor.affected->duration == 12);
        invoke(armor[i], 99999, &actor, &actor);
        assert(actor.armor == bonuses[i]);
        strip(armor[i], &actor);
        assert(actor.armor == 0);
    }
    /* Players gain avoidance at each250 armor; NPC avoidance is level based.
     * Verify actual player defense, not just an affect with a plausible sign.
     */
    actor.act = 0;
    actor.armor = 240;
    for (i = 0; i < 3; i++)
    {
        before = avoidblow(&actor);
        invoke(armor[i], 20, &actor, &actor);
        assert(actor.armor == 240 + bonuses[i] && avoidblow(&actor) > before);
        strip(armor[i], &actor);
        assert(actor.armor == 240 && avoidblow(&actor) == before);
    }
    actor.armor = 0;
    actor.act = ACT_IS_NPC;
    invoke("bless", 20, &actor, &target);
    assert(target.hitroll == 2 && target.saving_throw == -2);
    invoke("bless", 20, &actor, &target);
    assert(target.hitroll == 2 && target.saving_throw == -2);
    strip("bless", &target);
    assert(target.hitroll == 0 && target.saving_throw == 0);
    original = actor.stats[STAT_STR];
    before = char_stat(&actor, STAT_STR);
    invoke("giant strength", 20, &actor, &actor);
    assert(char_stat(&actor, STAT_STR) == before + 1);
    assert(actor.stats[STAT_STR] == original);
    strip("giant strength", &actor);
    assert(char_stat(&actor, STAT_STR) == before);
    actor.sex = SEX_MALE;
    invoke("change sex", 20, &actor, &actor);
    assert(actor.sex != SEX_MALE && actor.sex >= 0 && actor.sex <= 2);
    before = actor.sex;
    invoke("change sex", 20, &actor, &actor);
    assert(actor.sex == before);
    strip("change sex", &actor);
    assert(actor.sex == SEX_MALE);
    puts("Armor, Bless, Strength and reversible Sex: bounded, nonstacking, removable");

    for (i = 0; i < 3; i++)
    {
        invoke(detection[i], -100, &actor, &actor);
        assert(is_affected(&actor, skill_lookup(detection[i])));
        assert(actor.affected->duration == 2);
    }
    SET_BIT(target.affected_by, AFF_HIDE);
    assert(can_see(&actor, &target));
    strip("detect hidden", &actor);
    assert(!can_see(&actor, &target));
    REMOVE_BIT(target.affected_by, AFF_HIDE);
    strip("detect evil", &actor); strip("detect magic", &actor);
    assert(legacy_is_evil(&target) && !legacy_is_evil(&actor));
    target.act = 0; assert(!legacy_is_evil(&target)); target.act = ACT_IS_NPC;
    invoke("protection", 20, &actor, &actor);
    assert(legacy_protected_damage(&target, &actor, 100) == 80);
    assert(legacy_protected_damage(&actor, &actor, 100) == 100);
    strip("protection", &actor);
    assert(legacy_protected_damage(&target, &actor, 100) == 100);
    invoke("pass door", 20, &actor, &actor);
    assert(is_affected(&actor, gsn_pass_door));
    assert(!IS_AFFECTED(&actor, AFF_PASS_DOOR));
    SET_BIT(actor.affected_by, AFF_PASS_DOOR);
    strip("pass door", &actor);
    assert(IS_AFFECTED(&actor, AFF_PASS_DOOR));
    REMOVE_BIT(actor.affected_by, AFF_PASS_DOOR);
    puts("Detection, NPC alignment, Protection and independent Pass Door ownership passed");

    actor.mana = 9990;
    invoke("refresh", 99999, &actor, &actor); assert(actor.mana == 10000);
    actor.mana = 0;
    invoke("refresh", 99999, &actor, &actor); assert(actor.mana == 25);
    obj.item_type = ITEM_DRINK_CON; obj.value[0] = 10; obj.value[3] = 1;
    invoke("create water", 99999, &actor, &obj);
    assert(obj.value[1] == 10 && obj.value[2] == LIQ_WATER && obj.value[3] == 1);
    obj.value[1] = 5; obj.value[2] = 1;
    invoke("create water", 10, &actor, &obj);
    assert(obj.value[1] == 5 && obj.value[2] == 1);
    obj.name = "test drink"; obj.short_descr = "a test drink";
    actor.carrying = &obj; obj.carried_by = &actor;
    invoke("detect poison", 20, &actor, &obj);
    obj_cast_spell(skill_lookup("detect poison"), 20, &actor, &actor, NULL);
    actor.carrying = NULL;
    puts("Refresh caps, water capacity/liquid/poison preservation and poison potion dispatch passed");

    room.room_flags = ROOM_SAFE;
    before = target.hit;
    invoke("dispel evil", 100, &actor, &target);
    invoke("weaken", 100, &actor, &target);
    assert(target.hit == before && target.affected == NULL);
    room.room_flags = 0;
    target.in_room = &other;
    invoke("dispel evil", 100, &actor, &target);
    invoke("weaken", 100, &actor, &target);
    assert(target.hit == before && target.affected == NULL);
    target.in_room = &room; evil.alignment = 0;
    invoke("dispel evil", 100, &actor, &target); assert(target.hit == before);
    evil.alignment = -1000;
    invoke("dispel evil", 100, &actor, &target);
    assert(target.hit < before && target.hit >= before - 200);
    stop_fighting(&actor, TRUE);
    for (i = 0; i < 64 && !is_affected(&target, skill_lookup("weaken")); i++)
        invoke("weaken", 100, &actor, &target);
    assert(is_affected(&target, skill_lookup("weaken")));
    assert(target.stats[STAT_STR] == 5 && char_stat(&target, STAT_STR) == 3);
    invoke("weaken", 100, &actor, &target);
    assert(char_stat(&target, STAT_STR) == 3);
    strip("weaken", &target); assert(char_stat(&target, STAT_STR) == 5);
    puts("Dispel Evil and Weaken: bounded effects, saves, room and combat protection passed");

    evil.alignment = -350;
    alignment_output(&actor, &target, "evil (-350)");
    evil.alignment = -349;
    alignment_output(&actor, &target, "neutral (-349)");
    evil.alignment = 349;
    alignment_output(&actor, &target, "neutral (349)");
    evil.alignment = 350;
    alignment_output(&actor, &target, "good (350)");
    evil.alignment = 99999;
    alignment_output(&actor, &target, "good (1000)");
    target.act = 0;
    alignment_output(&actor, &target, "no NPC alignment");
    target.act = ACT_IS_NPC;
    SET_BIT(target.affected_by, AFF_HIDE);
    alignment_output(&actor, &target, "cannot read");
    REMOVE_BIT(target.affected_by, AFF_HIDE);
    target.in_room = &other;
    alignment_output(&actor, &target, "cannot read");
    target.in_room = &room;
    assert(actor.affected == NULL && target.affected == NULL);
    puts("Know Alignment: output, thresholds, clamp, visibility and NPC-only scope passed");

    original = target.enrage_timer;
    invoke("frenzy", 99999, &actor, &target);
    assert(target.hitroll == 1 && target.damroll == 1);
    assert(target.affected->duration == 12);
    assert(target.fighting == NULL && target.enrage_timer == original);
    invoke("frenzy", 1, &actor, &target);
    assert(target.hitroll == 1 && target.damroll == 1);
    assert(target.affected->duration == 12);
    strip("frenzy", &target);
    assert(target.hitroll == 0 && target.damroll == 0);

    invoke("darkblessing", 20, &actor, &actor);
    assert(actor.affected == NULL);
    evil.alignment = -1000;
    target.act = 0;
    invoke("darkblessing", 20, &actor, &target);
    assert(target.affected == NULL);
    target.act = ACT_IS_NPC;
    evil.alignment = -349;
    invoke("darkblessing", 20, &actor, &target);
    assert(target.affected == NULL);
    evil.alignment = -350;
    invoke("darkblessing", -100, &actor, &target);
    assert(target.hitroll == 1 && target.damroll == 1 && target.armor == 10);
    assert(char_ac(&target)>0);
    assert(target.affected->duration == 2);
    invoke("darkblessing", 99999, &actor, &target);
    assert(target.hitroll == 1 && target.damroll == 1 && target.armor == 10);
    assert(target.affected->duration == 2);
    invoke("frenzy", 20, &actor, &target);
    assert(target.hitroll == 2 && target.damroll == 2 && target.armor == 10);
    strip("darkblessing", &target);
    assert(target.hitroll == 1 && target.damroll == 1 && target.armor == 0);
    invoke("darkblessing", 20, &actor, &target);
    invoke("dispel magic", 20, &target, &target);
    assert(target.affected == NULL && target.hitroll == 0
           && target.damroll == 0 && target.armor == 0);
    invoke("frenzy", 20, &actor, NULL);
    invoke("darkblessing", 20, &actor, NULL);
    alignment_output(&actor, NULL, "cannot read");
    assert(skill_table[skill_lookup("know alignment")].skill_level == 99);
    assert(skill_table[skill_lookup("frenzy")].skill_level == 99);
    assert(skill_table[skill_lookup("darkblessing")].skill_level == 99);
    puts("Frenzy/Darkblessing: NPC eligibility, caps, nonstacking, independent removal and dispel passed");

    actor.act = 0;
    actor.home = -1;
    invoke("word of recall", 20, &actor, &actor);
    assert(actor.in_room == &room);
    actor.home = 0;
    actor.fight_timer = 10;
    invoke("word of recall", 20, &actor, &actor);
    assert(actor.in_room == &room);
    actor.fight_timer = 0;
    room.room_flags=0;
    stop_fighting(&actor,TRUE); stop_fighting(&target,TRUE);
    target.hit=target.max_hit=1000000;
    target.affected_by=AFF_INFRARED|AFF_FLYING;
    before=target.hit;
    invoke("earthquake",20,&actor,NULL);
    assert(target.hit==before);
    REMOVE_BIT(target.affected_by,AFF_FLYING);
    SET_BIT(actor.affected_by,AFF_FLYING);
    invoke("earthquake",20,&actor,NULL);
    assert(target.hit<before);
    REMOVE_BIT(actor.affected_by,AFF_FLYING);
    stop_fighting(&actor,TRUE); stop_fighting(&target,TRUE);
    actor.act=ACT_IS_NPC; target.act=0; target.class=CLASS_DRAGON;
    actor.position=target.position=POS_STANDING;
    memset(&target_desc,0,sizeof(target_desc));
    target_output[0]=0; target_desc.outbuf=target_output;
    target_desc.outsize=sizeof(target_output); target_desc.character=&target;
    target.desc=&target_desc;
    assert(!is_safe(&actor,&target));
    target.pcdata->dragskills=DRAG_WINGSOUT; before=target.hit;
    invoke("earthquake",20,&actor,NULL); assert(target.hit==before);
    target.pcdata->dragskills=0; target.class=CLASS_VAMPIRE;
    SET_BIT(target.vampaff,VAM_FLYING);
    invoke("earthquake",20,&actor,NULL); assert(target.hit==before);
    REMOVE_BIT(target.vampaff,VAM_FLYING); target.class=CLASS_NONE;
    invoke("earthquake",20,&actor,NULL); assert(target.hit<before);
    stop_fighting(&actor,TRUE); stop_fighting(&target,TRUE);
    actor.act=0; target.act=ACT_IS_NPC; target.desc=NULL;
    puts("Earthquake: spell/Dragon/Vampire flight spares targets, "
        "flying casters still damage grounded foes, both NPC/player target directions passed");
    invoke("ventriloquate", 20, &actor, &actor);
    assert(actor.affected == NULL);
    puts("Recall refuses invalid/blocked travel; Ventriloquate safely ignores an absent speech target");
    return 0;
}
