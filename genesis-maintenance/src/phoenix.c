/* New standalone Phoenix rules, not a recovered upstream implementation. */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"

long phoenix_rank(CHAR_DATA *ch)
{
    if (!IS_PHOENIX(ch) || ch->pcdata == NULL) return 0;
    return URANGE(0, ch->pcdata->powers[0], 8);
}

static void phoenix_affect(CHAR_DATA *ch, long sn, long location,
                           long modifier, long duration, long bits)
{
    AFFECT_DATA af;
    memset(&af, 0, sizeof(af));
    af.type = sn;
    af.location = location;
    af.modifier = modifier;
    af.duration = duration;
    af.bitvector = bits;
    affect_to_char(ch, &af);
}

void phoenix_clear(CHAR_DATA *ch)
{
    affect_strip(ch, gsn_phoenix_ward);
    affect_strip(ch, gsn_phoenix_wings);
    affect_strip(ch, gsn_phoenix_ascend);
    affect_strip(ch, gsn_phoenix_recovery);
}

static int phoenix_ready(CHAR_DATA *ch, long rank, long cost)
{
    if (phoenix_rank(ch) < rank) {
        stc("You have not mastered that Phoenix power.\n\r", ch);
        return 0;
    }
    if (is_affected(ch, gsn_lycaon)) {
        stc("Lycaon's curse prevents Phoenix powers.\n\r", ch);
        return 0;
    }
    if (is_affected(ch, gsn_phoenix_recovery)) {
        stc("Your Phoenix flame is still recovering.\n\r", ch);
        return 0;
    }
    if (ch->mana < cost) {
        stc("You do not have enough mana.\n\r", ch);
        return 0;
    }
    return 1;
}

static void phoenix_commit(CHAR_DATA *ch, long cost, long recovery)
{
    ch->mana -= cost;
    WAIT_STATE(ch, 12);
    combat_recovery_start(ch, gsn_phoenix_recovery, recovery);
}

static CHAR_DATA *phoenix_enemy(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim = argument[0] == '\0' ? ch->fighting
                                          : get_char_room(ch, argument);
    if (victim == NULL || victim == ch || victim->in_room != ch->in_room
        || !can_see(ch, victim) || IS_BUI(victim) || is_same_group(ch, victim)
        || IS_SET(ch->in_room->room_flags, ROOM_SAFE)
        || (!IS_NPC(victim) && (!CAN_PK(ch) || !CAN_PK(victim)))
        || is_safe(ch, victim)) {
        stc("That is not a legal Phoenix combat target here.\n\r", ch);
        return NULL;
    }
    return victim;
}

static void phoenix_attack(CHAR_DATA *ch, char *argument, int flare)
{
    CHAR_DATA *victim;
    long cost = flare ? 1500 : 500;
    long rank = flare ? 6 : 1;
    long amount;
    if (!phoenix_ready(ch, rank, cost)) return;
    victim = phoenix_enemy(ch, argument);
    if (victim == NULL || !hurt_fighting(ch, victim)) return;
    amount = flare ? 200 + phoenix_rank(ch) * 60
                   : 300 + phoenix_rank(ch) * 100;
    phoenix_commit(ch, cost, 1);
    if (saves_spell(75, victim)) amount /= 2;
    else if (flare && !IS_AFFECTED(victim, AFF_BLIND)
             && !is_affected(victim, gsn_blindness))
        phoenix_affect(victim, gsn_blindness, APPLY_HITROLL, -4, 1, AFF_BLIND);
    act(flare ? "You release a solar flare at $N."
              : "You cast an ember bolt at $N.", ch, NULL, victim, TO_CHAR);
    /* Never access either combatant after damage: extraction may occur. */
    damage(ch, victim, amount, TYPE_SPELL, flare ? "solar flare" : "ember bolt");
}

static long phoenix_restore(long current, long maximum, long amount)
{
    if (maximum <= 0 || current >= maximum || amount <= 0) return 0;
    /* A target cannot receive these support powers at zero/negative health. */
    if (current < 0) return 0;
    return UMIN(amount, maximum - current);
}

static void phoenix_tears(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim = argument[0] == '\0' ? ch : get_char_room(ch, argument);
    long heal;
    if (!phoenix_ready(ch, 5, 1000)) return;
    if (victim == NULL || victim->in_room != ch->in_room
        || !can_see(ch, victim) || victim->hit <= 0
        || (victim != ch && !is_same_group(ch, victim))) {
        stc("Phoenix Tears require a living, visible group ally here.\n\r", ch);
        return;
    }
    heal = phoenix_restore(victim->hit, victim->max_hit,
                           UMIN(3000, victim->max_hit / 10));
    if (heal <= 0) {
        stc("They need no Phoenix healing.\n\r", ch);
        return;
    }
    phoenix_commit(ch, 1000, 2);
    victim->hit += heal;
    update_pos(victim);
    act("Your Phoenix Tears mend $N's wounds.", ch, NULL, victim, TO_CHAR);
}

static void phoenix_renew(CHAR_DATA *ch)
{
    long heal;
    if (!phoenix_ready(ch, 7, 2000)) return;
    if (ch->fighting != NULL || ch->fight_timer > 0) {
        stc("Renewal requires peace and no fight timer.\n\r", ch);
        return;
    }
    heal = phoenix_restore(ch->hit, ch->max_hit, UMIN(6000, ch->max_hit / 5));
    if (heal <= 0) {
        stc("Your health needs no renewal.\n\r", ch);
        return;
    }
    phoenix_commit(ch, 2000, 3);
    ch->hit += heal;
    update_pos(ch);
    stc("Your flame rekindles, renewing your health.\n\r", ch);
}

static void phoenix_ashwalk(CHAR_DATA *ch)
{
    ROOM_INDEX_DATA *before = ch->in_room;
    ROOM_INDEX_DATA *home;
    if (!phoenix_ready(ch, 4, 1500)) return;
    if (ch->home <= 0) {
        stc("You cannot Ashwalk without a valid home.\n\r", ch);
        return;
    }
    home = get_room_index(ch->home);
    if (home == NULL || home == before || ch->fighting != NULL
        || ch->fight_timer > 0 || IS_AFFECTED(ch, AFF_CURSE)
        || IS_AFFECTED(ch, AFF_WEBBED)
        || IS_SET(before->room_flags, ROOM_NO_RECALL | ROOM_NO_TELEPORT)
        || IS_SET(home->room_flags, ROOM_NO_TELEPORT | ROOM_PRIVATE | ROOM_SOLITARY)) {
        stc("You cannot Ashwalk to your home from here.\n\r", ch);
        return;
    }
    /* Preserve the existing recall engine; failed travel is free. */
    do_recall(ch, "");
    if (ch->in_room != before) {
        phoenix_commit(ch, 1500, 3);
        stc("You gather your ashes at home.\n\r", ch);
    }
}

static void phoenix_buff(CHAR_DATA *ch, int which)
{
    long sn, rank, cost;
    if (which == 0) {
        sn = gsn_phoenix_ward; rank = 2; cost = 750;
    } else if (which == 1) {
        sn = gsn_phoenix_wings; rank = 3; cost = 1000;
    } else {
        sn = gsn_phoenix_ascend; rank = 8; cost = 3000;
    }
    if (!phoenix_ready(ch, rank, cost)) return;
    if (is_affected(ch, sn) || (which == 1 && IS_AFFECTED(ch, AFF_FLYING))) {
        stc("That Phoenix power is already active.\n\r", ch);
        return;
    }
    phoenix_commit(ch, cost, 1);
    if (which == 0)
        phoenix_affect(ch, sn, APPLY_AC, 20, 3, 0);
    else if (which == 1)
        phoenix_affect(ch, sn, APPLY_NONE, 0, 3, AFF_FLYING);
    else {
        phoenix_affect(ch, sn, APPLY_HITROLL, 10, 3, 0);
        phoenix_affect(ch, sn, APPLY_DAMROLL, 10, 3, 0);
    }
    stc(which == 0 ? "A feather ward protects you.\n\r"
        : which == 1 ? "Flaming wings lift you.\n\r"
                     : "You assume your ascendant Phoenix aspect.\n\r", ch);
}

void do_phoenix(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], target[MAX_INPUT_LENGTH], extra[MAX_INPUT_LENGTH];
    long rank, cost;
    char buf[MAX_STRING_LENGTH];
    if (IS_NPC(ch) || ch->pcdata == NULL) return;
    if (argument == NULL || strlen(argument) >= sizeof(arg)) {
        stc("Syntax: phoenix <power> [target]\n\r", ch);
        return;
    }
    /* Preserve the old Elemental abbreviation, without relaxing its position. */
    if (IS_ELEM(ch)) {
        if (is_affected(ch, gsn_lycaon))
            stc("You cannot change form until Lycaon's curse is lifted.\n\r", ch);
        else if (ch->position < POS_STANDING)
            stc("You must be standing to use Phoenix Form.\n\r", ch);
        else do_phoenixform(ch, argument);
        return;
    }
    if (!IS_PHOENIX(ch)) {
        stc("Only the Phoenix class can use these powers.\n\r", ch);
        return;
    }
    if (ch->in_room == NULL || ch->position < POS_FIGHTING || ch->hit <= 0) {
        stc("Phoenix powers require a living, conscious caster in a room.\n\r", ch);
        return;
    }
    argument = one_argument(argument, arg);
    argument = one_argument(argument, target);
    one_argument(argument, extra);
    if (extra[0] != '\0') {
        stc("Syntax: phoenix <power> [target]\n\r", ch);
        return;
    }
    rank = phoenix_rank(ch);
    if (arg[0] == '\0') {
        sprintf(buf, "Phoenix mastery: %li/8. Use phoenix train to advance.\n\r", rank);
        stc(buf, ch);
        class_training_header(ch,"PHOENIX TRAINING");
        class_training_row(ch,"phoenix train",rank,8,
            class_training_cost(rank,8,50000,TRUE),"XP",ch->exp,
            ch->fighting || ch->fight_timer>0 ? "In combat" : NULL);
        stc("Command                Rank  Target / effect\n\r"
            "---------------------  ----  -------------------------------\n\r"
            "ember [enemy]             1  Fire attack\n\r"
            "ward                      2  Defensive ward\n\r"
            "wings                     3  Flight\n\r"
            "ashwalk                   4  Bounded travel home\n\r"
            "tears [group ally]        5  Healing\n\r"
            "flare [enemy]             6  Solar attack\n\r"
            "renew                     7  Self restoration\n\r"
            "ascend                    8  Phoenix aspect (not ascension)\n\r"
            "Prefix every command with phoenix. HELP PHOENIX.\n\r", ch);
        return;
    }
    if (strcmp(arg, "ember") && strcmp(arg, "tears") && strcmp(arg, "flare")
        && target[0] != '\0') {
        stc("That Phoenix power takes no target.\n\r", ch);
        return;
    }
    if (!strcmp(arg, "train")) {
        if (IS_BUI(ch)) {
            stc("Staff characters cannot train Phoenix mastery; use a mortal tester.\n\r",ch);
            return;
        }
        if (ch->fighting != NULL || ch->fight_timer > 0) {
            stc("You cannot train Phoenix mastery during combat.\n\r", ch);
            return;
        }
        if (ch->position<POS_STANDING || IS_MORE(ch,MORE_HEAD)
            || IS_EXTRA(ch,EXTRA_OSWITCH) || is_affected(ch,gsn_lycaon)) {
            stc("Phoenix training requires standing in your living, unpossessed body;\n\r"
                "Lycaon's curse must be lifted first.\n\r",ch);
            return;
        }
        if (rank >= 8) {
            stc("Your Phoenix mastery is already complete.\n\r", ch);
            return;
        }
        cost = 50000 * (rank + 1) * (rank + 1);
        if (ch->exp < cost) {
            sprintf(buf, "You need %li experience for the next Phoenix rank.\n\r", cost);
            stc(buf, ch);
            return;
        }
        ch->exp -= cost;
        ch->pcdata->powers[0] = rank + 1;
        stc("Your Phoenix mastery advances.\n\r", ch);
    } else if (!strcmp(arg, "ember")) phoenix_attack(ch, target, 0);
    else if (!strcmp(arg, "ward")) phoenix_buff(ch, 0);
    else if (!strcmp(arg, "wings")) phoenix_buff(ch, 1);
    else if (!strcmp(arg, "ashwalk")) phoenix_ashwalk(ch);
    else if (!strcmp(arg, "tears")) phoenix_tears(ch, target);
    else if (!strcmp(arg, "flare")) phoenix_attack(ch, target, 1);
    else if (!strcmp(arg, "renew")) phoenix_renew(ch);
    else if (!strcmp(arg, "ascend")) phoenix_buff(ch, 2);
    else stc("Unknown Phoenix power. Use phoenix to see the kit.\n\r", ch);
}
