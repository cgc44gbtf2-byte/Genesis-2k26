#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "../src/merc.h"

static int roll;
long gsn_lycaon = 1;
/* This lightweight probe does not link db.c's skill globals. */
long gsn_phoenix_wings = 2;
/* Focused resolver harness doesn't link db.c. New real flight ownership is
 * exercised separately by the full linked new-class probe.
 */
long gsn_parent_wings = 149;
long gsn_crafted_wings=161, gsn_lineage_wings=173;
char *str_dup(const char *text)
{
    char *result = malloc(strlen(text) + 1);
    assert(result);
    strcpy(result, text);
    return result;
}
void free_string(char *text) { free(text); }
int number_percent(void) { return roll; }
void bug(const char *text, long value)
{
    (void) text;
    (void) value;
    assert(0 && "Unexpected engine bug");
}

int main(void)
{
    CHAR_DATA ch;
    PC_DATA pc;
    AFFECT_DATA af;
    AFFECT_DATA template_af, instance_af;
    OBJ_DATA obj;
    OBJ_INDEX_DATA index;
    int i;
    static const long locations[] = {
        APPLY_STR, APPLY_DEX, APPLY_CON, APPLY_INT, APPLY_WIS
    };
    memset(&ch, 0, sizeof(ch));
    memset(&pc, 0, sizeof(pc));
    ch.pcdata = &pc;
    ch.class = CLASS_WEREWOLF;
    ch.stats[STAT_WIS] = 8;
    /* Level 55: base chance is 40; Philodox 2 makes it 50. */
    roll = 45;
    pc.auspice[AUSPICE_PHILODOX] = 1;
    assert(!saves_spell(55, &ch));
    pc.auspice[AUSPICE_PHILODOX] = 2;
    assert(saves_spell(55, &ch));
    ch.class = CLASS_VAMPIRE;
    assert(!saves_spell(55, &ch));
    ch.class = CLASS_WEREWOLF;
    ch.stats[STAT_WIS] = 100;
    roll = 90;
    assert(!saves_spell(1, &ch)); /* Existing 85% cap retained. */
    puts("Strength of Purpose deterministic regression passed");

    memset(&af, 0, sizeof(af));
    af.type = gsn_lycaon;
    af.location = APPLY_POLY;
    af.modifier = POLY_LYCAON;
    af.bitvector = AFF_POLYMORPH;
    ch.morph = str_dup("a spirit-cursed wolf");
    affect_modify(&ch, &af, TRUE);
    assert(ch.polyaff == POLY_LYCAON);
    assert(IS_AFFECTED(&ch, AFF_POLYMORPH));
    affect_modify(&ch, &af, FALSE);
    assert(ch.polyaff == 0 && ch.morph[0] == '\0');
    assert(!IS_AFFECTED(&ch, AFF_POLYMORPH));
    free_string(ch.morph);

    /* A staff replacement may overwrite, rather than add to, the form flags.
     * Curse removal must never subtract its missing bit into a negative value.
     */
    ch.polyaff = POLY_PHOENIXFORM;
    ch.morph = str_dup("a phoenix");
    affect_modify(&ch, &af, FALSE);
    assert(ch.polyaff == POLY_PHOENIXFORM);
    assert(IS_AFFECTED(&ch, AFF_POLYMORPH));
    assert(!strcmp(ch.morph, "a phoenix"));
    affect_modify(&ch, &af, FALSE);
    assert(ch.polyaff == POLY_PHOENIXFORM);
    free_string(ch.morph);
    puts("Lycaon deterministic form-ownership regression passed");

    memset(&af, 0, sizeof(af));
    ch.class = CLASS_VAMPIRE;
    ch.level = 2;
    ch.polyaff = ch.affected_by = 0;
    ch.affected = &af;
    for (i = STAT_STR; i <= STAT_WIS; i++)
    {
        ch.stats[i] = 8;
        af.location = locations[i];
        af.modifier = -5;
        assert(char_stat(&ch, i) == 3);
        assert(ch.stats[i] == 8);
        af.modifier = -50;
        assert(char_stat(&ch, i) == 1);
        af.modifier = 2;
        assert(char_stat(&ch, i) == 10);
    }
    af.location = APPLY_STR;
    af.modifier = -5;
    memset(&obj, 0, sizeof(obj));
    memset(&index, 0, sizeof(index));
    memset(&template_af, 0, sizeof(template_af));
    memset(&instance_af, 0, sizeof(instance_af));
    obj.pIndexData = &index;
    index.affected = &template_af;
    obj.affected = &instance_af;
    template_af.location = instance_af.location = APPLY_STR;
    template_af.modifier = 2;
    instance_af.modifier = 3;
    obj.wear_loc = WEAR_BODY;
    ch.carrying = &obj;
    assert(char_stat(&ch, STAT_STR) == 8); /* 8 - 5 + 2 + 3 */
    obj.wear_loc = WEAR_NONE;
    assert(char_stat(&ch, STAT_STR) == 3);
    ch.affected = NULL;
    assert(char_stat(&ch, STAT_STR) == 8);
    ch.carrying = NULL;
    puts("Derived attributes passed: all five stats, floor, buffs, gear, removal, base preservation");

    roll = 45;
    ch.saving_throw = 0;
    assert(!saves_spell(50, &ch));
    ch.saving_throw = -10;
    assert(saves_spell(50, &ch));
    ch.saving_throw = 10;
    assert(!saves_spell(50, &ch));
    ch.saving_throw = -1000;
    roll = 90;
    assert(!saves_spell(50, &ch));
    ch.saving_throw = 1000;
    roll = 10;
    assert(saves_spell(50, &ch)); /* Existing lower cap retained too. */
    ch.saving_throw = 0;
    roll = 40;
    assert(saves_spell(50, &ch));
    af.location = APPLY_WIS;
    af.modifier = -2;
    ch.affected = &af;
    assert(!saves_spell(50, &ch));
    ch.affected = NULL;
    puts("Saving modifiers passed: both signs, both caps, derived WIS, Philodox independence");

    ch.armor = 240;
    ch.hit = ch.max_hit = 1000;
    af.type = 2;
    af.location = APPLY_AC;
    af.modifier = 20;
    assert(char_ac(&ch) / 250 == 0);
    affect_modify(&ch, &af, TRUE);
    assert(char_ac(&ch) == 260 && char_ac(&ch) / 250 == 1);
    affect_modify(&ch, &af, FALSE);
    assert(char_ac(&ch) == 240);
    puts("Protective armor passed: +20 increases the actual avoidance term and removes cleanly");
    return 0;
}
