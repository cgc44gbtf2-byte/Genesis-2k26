#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"

/* Linked with the real command, spell, combat and skill tables; the two
 * characters have no descriptors, so messages are intentionally discarded.
 */
int main(void)
{
    CHAR_DATA actor, target;
    PC_DATA actor_pc, target_pc;
    ROOM_INDEX_DATA room;
    int i, names = 0;
    const char *stances[] = {
        "viper", "crane", "crab", "mongoose", "bull",
        "mantis", "dragon", "tiger", "monkey", "swallow"
    };
    memset(&actor, 0, sizeof(actor));
    memset(&target, 0, sizeof(target));
    memset(&actor_pc, 0, sizeof(actor_pc));
    memset(&target_pc, 0, sizeof(target_pc));
    memset(&room, 0, sizeof(room));
    actor.pcdata = &actor_pc;
    target.pcdata = &target_pc;
    actor.name = actor.morph = "Actor";
    target.name = target.morph = "Victim";
    actor.level = target.level = LEV_SLE;
    actor.in_room = target.in_room = &room;
    room.sector_type = SECT_INSIDE;
    room.people = &actor;
    actor.next_in_room = &target;
    actor.position = target.position = POS_STANDING;
    actor.style[0] = target.style[0] = STYLE_UNARMED;
    actor.affected_by = target.affected_by = AFF_INFRARED;
    actor.max_mana = actor.mana = 10000;

    for (i = 0; i < MAX_SKILL && skill_table[i].name != NULL; i++)
        names++;
    assert(names == 182);
    /* Existing indices stay stable as class effects are appended. */
    assert(skill_lookup("gift recovery") == 119);
    assert(skill_lookup("phoenix ember") == 120);
    assert(skill_lookup("parent hindrance") == 133);
    assert(skill_lookup("parent chill") == 138);
    assert(skill_lookup("cyber mine") == 143);
    assert(skill_lookup("parent companion") == 144);
    assert(skill_lookup("parent legacy") == 145);
    assert(skill_lookup("parent shadow") == 154);
    assert(skill_lookup("crafted recovery") == 155);
    assert(skill_lookup("crafted doom") == 166);
    assert(skill_lookup("lineage recovery") == 167);
    assert(skill_lookup("lineage refuge") == names - 1);
    assert(sn_lookup(MAX_SKILL) == 0);
    assert(sn_lookup(names) == 0); /* unused, unnamed table capacity */
    assert(slot_lookup(25) == skill_lookup("energy drain"));
    for (i = 0; i < 5; i++)
    {
        do_stance(&actor, (char *)stances[i]);
        assert(actor.stance[0] == i + 1);
    }
    do_stance(&actor, "mantis");
    assert(actor.stance[0] == STANCE_BULL); /* prerequisites enforced */
    for (i = 1; i <= 5; i++)
        actor.stance[i] = 200;
    for (i = 0; i < 10; i++)
    {
        do_stance(&actor, (char *)stances[i]);
        assert(actor.stance[0] == i + 1);
        do_autostance(&actor, (char *)stances[i]);
        assert(actor.stance[11] == i + 1);
        actor.stance[0] = STANCE_NONE;
        autostance(&actor);
        assert(actor.stance[0] == i + 1);
    }
    do_stance(&actor, "");
    do_autostance(&actor, "none");
    assert(actor.stance[0] == STANCE_NONE && actor.stance[11] == STANCE_NONE);
    for (i = STANCE_CRAB; i <= STANCE_SWALLOW; i++)
    {
        if (i != STANCE_CRAB && i != STANCE_DRAGON && i != STANCE_SWALLOW)
            continue;
        target.stance[0] = i;
        target.stance[i] = 200;
        assert(dambonus(&actor, &target, 300) == 100);
        actor.stance[0] = STANCE_VIPER;
        actor.stance[STANCE_VIPER] = 0;
        assert(dambonus(&actor, &target, 300) == 100);
        actor.stance[0] = STANCE_NONE;
    }
    target.style[0] = 999;
    do_skill(&actor, "Victim"); /* must not index style[999] */
    target.style[0] = STYLE_UNARMED;
    target.stance[0] = 999;
    do_skill(&actor, "Victim"); /* must not index stance[999] */
    target.stance[0] = STANCE_NONE;
    puts("Stance selection, defense, skill display and index guards passed");

    i = skill_lookup("detect invis");
    assert(i > 0 && skill_table[i].minimum_position == POS_STANDING);
    actor.position = POS_FIGHTING;
    interpret(&actor, "cast 'detect invis'");
    assert(actor.mana == 10000 && !IS_AFFECTED(&actor, AFF_DETECT_INVIS));
    actor.position = POS_STANDING;
    interpret(&actor, "cast 'detect invis'");
    assert(actor.mana == 9995 && IS_AFFECTED(&actor, AFF_DETECT_INVIS));
    actor.position = POS_FIGHTING;
    interpret(&actor, "cast 'energy drain' Actor");
    assert(actor.mana == 9995); /* self-drain refused before charging */
    target.mana = 9000;
    target.hit = 10000;
    /* A link-dead, nonfighting player is protected by is_safe. Direct
     * invocation must not transfer mana before the damage safety check.
     */
    spell_energy_drain(skill_lookup("energy drain"), 45, &actor, &target);
    assert(target.mana == 9000 && target.hit == 10000
           && actor.mana == 9995);
    actor.position = POS_STANDING;
    interpret(&actor, "cast 'control weather' neither");
    assert(actor.mana == 9995 && weather_info.change == 0);
    interpret(&actor, "cast 'control weather' better");
    assert(actor.mana == 9970 && weather_info.change == 1);
    interpret(&actor, "cast 'control weather' worse");
    assert(actor.mana == 9945 && weather_info.change == 0);
    puts("Cast positions, invalid-input cost, weather change and self-drain passed");
    return 0;
}
