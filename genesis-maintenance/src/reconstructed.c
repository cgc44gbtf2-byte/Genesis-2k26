/*
 * Conservative replacement mechanics for abilities with no surviving effects.
 * These are new rules, not recovered upstream behavior. See
 * ABILITY_RECONSTRUCTION.md for costs, timing, limitations and compatibility.
 */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"

static void gift_affect(CHAR_DATA *ch, long sn, long location, long modifier,
                        long duration, long bits)
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

static bool gift_caster(CHAR_DATA *ch, bool wolf)
{
    if (IS_NPC(ch) || ch->in_room == NULL)
        return FALSE;
    if (wolf ? !IS_WEREWOLF(ch) : !IS_VAMPIRE(ch))
    {
        stc("Huh?\n\r", ch);
        return FALSE;
    }
    if (is_affected(ch, gsn_lycaon))
    {
        stc("Lycaon's curse prevents you from channeling this power.\n\r", ch);
        return FALSE;
    }
    return TRUE;
}

static bool gift_mana(CHAR_DATA *ch, long cost)
{
    if (ch->mana < cost)
    {
        stc("You do not have enough mana.\n\r", ch);
        return FALSE;
    }
    return TRUE;
}

/* Additional explicit PK protection keeps these new mechanics conservative,
 * even where the legacy combat helper permits an existing fight to continue.
 */
static bool gift_enemy(CHAR_DATA *ch, CHAR_DATA *victim, long sn)
{
    if (victim == NULL || victim == ch || victim->in_room != ch->in_room
        || IS_BUI(victim) || is_same_group(ch, victim)
        || is_affected(victim, sn))
        return FALSE;
    if (IS_SET(ch->in_room->room_flags, ROOM_SAFE))
        return FALSE;
    if (!IS_NPC(victim) && (!CAN_PK(ch) || !CAN_PK(victim)))
        return FALSE;
    return !is_safe(ch, victim);
}

static CHAR_DATA *gift_target(CHAR_DATA *ch, char *argument, long sn)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    one_argument(argument, arg);
    victim = arg[0] == '\0' ? ch->fighting : get_char_room(ch, arg);
    if (victim == NULL)
    {
        stc("Choose a target here.\n\r", ch);
        return NULL;
    }
    if (!gift_enemy(ch, victim, sn))
    {
        stc("That target is protected, grouped with you, or already affected.\n\r", ch);
        return NULL;
    }
    return victim;
}

static bool gift_resisted(CHAR_DATA *ch, CHAR_DATA *victim, bool mental)
{
    if ((mental && is_affected(victim, gsn_maskempathy))
        || saves_spell(75, victim))
    {
        act("$N resists your gift.", ch, NULL, victim, TO_CHAR);
        stc("You resist the hostile gift.\n\r", victim);
        return TRUE;
    }
    return FALSE;
}

static bool gift_attempt(CHAR_DATA *ch, CHAR_DATA *victim, long cost, bool mental)
{
    if (!gift_mana(ch, cost) || !hurt_fighting(ch, victim))
        return FALSE;
    ch->mana -= cost;
    WAIT_STATE(ch, 12);
    return !gift_resisted(ch, victim, mental);
}

void do_disquiet(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    if (!gift_caster(ch, TRUE)) return;
    if (get_breed(ch, BREED_HOMID) < 4)
    {
        stc("You require Homid 4 to use Disquiet.\n\r", ch);
        return;
    }
    if ((victim = gift_target(ch, argument, gsn_disquiet)) == NULL
        || !gift_attempt(ch, victim, 1000, TRUE)) return;
    gift_affect(victim, gsn_disquiet, APPLY_HITROLL, -15, 1, 0);
    gift_affect(victim, gsn_disquiet, APPLY_DAMROLL, -10, 1, 0);
    act("You shatter $N's confidence with Disquiet.", ch, NULL, victim, TO_CHAR);
    stc("Doubt clouds your attacks.\n\r", victim);
}

static void gift_area(CHAR_DATA *ch, long sn, long cost, bool mental)
{
    CHAR_DATA *victim, *next;
    bool any = FALSE;
    if (IS_SET(ch->in_room->room_flags, ROOM_SAFE))
    {
        stc("You cannot channel a hostile area gift in a safe room.\n\r", ch);
        return;
    }
    if (is_affected(ch, gsn_gift_recovery))
    {
        stc("You are still recovering from your last area gift.\n\r", ch);
        return;
    }
    if (!gift_mana(ch, cost)) return;
    for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
        if (gift_enemy(ch, victim, sn)) { any = TRUE; break; }
    if (!any)
    {
        stc("There are no eligible enemies here.\n\r", ch);
        return;
    }
    ch->mana -= cost;
    WAIT_STATE(ch, 24);
    gift_affect(ch, gsn_gift_recovery, APPLY_NONE, 0, 1, 0);
    stc(mental ? "You unleash the Scream of Gaia.\n\r"
               : "You call spirit quicksand beneath your enemies.\n\r", ch);
    for (victim = ch->in_room->people; victim; victim = next)
    {
        next = victim->next_in_room;
        if (!gift_enemy(ch, victim, sn) || !hurt_fighting(ch, victim)
            || gift_resisted(ch, victim, mental))
            continue;
        if (mental)
        {
            gift_affect(victim, sn, APPLY_HITROLL, -10, 0, 0);
            WAIT_STATE(victim, 8);
            stc("Gaia's scream leaves you reeling.\n\r", victim);
        }
        else
        {
            gift_affect(victim, sn, APPLY_NONE, 0, 1, 0);
            stc("Spirit quicksand slows your movements.\n\r", victim);
        }
    }
}

void do_quicksand(CHAR_DATA *ch, char *argument)
{
    int sector;
    if (!gift_caster(ch, TRUE)) return;
    if (get_tribe(ch, TRIBE_RED_TALONS) < 4)
    {
        stc("You require Red Talons 4 to use Quicksand.\n\r", ch);
        return;
    }
    sector = ch->in_room->sector_type;
    if (IS_SET(ch->in_room->room_flags, ROOM_INDOORS)
        || sector == SECT_INSIDE || (sector >= SECT_WATER_SWIM && sector <= SECT_AIR))
    {
        stc("Quicksand requires solid ground outside.\n\r", ch);
        return;
    }
    gift_area(ch, gsn_quicksand, 1500, FALSE);
}

void do_scream(CHAR_DATA *ch, char *argument)
{
    if (!gift_caster(ch, TRUE)) return;
    if (get_tribe(ch, TRIBE_GET_OF_FENRIS) < 4)
    {
        stc("You require Get of Fenris 4 to use Scream of Gaia.\n\r", ch);
        return;
    }
    gift_area(ch, gsn_scream, 2000, TRUE);
}

void do_lycaon(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    if (!gift_caster(ch, TRUE)) return;
    if (get_tribe(ch, TRIBE_RED_TALONS) < 5)
    {
        stc("You require Red Talons 5 to use Lycaon.\n\r", ch);
        return;
    }
    if ((victim = gift_target(ch, argument, gsn_lycaon)) == NULL) return;
    if (IS_NPC(victim))
    {
        stc("Lycaon only affects player characters.\n\r", ch);
        return;
    }
    if (IS_AFFECTED(victim, AFF_POLYMORPH) || victim->polyaff != 0
        || IS_VAMPAFF(victim, VAM_DISGUISED) || victim->morph[0] != '\0')
    {
        stc("Their existing form must return to normal first.\n\r", ch);
        return;
    }
    if (!gift_attempt(ch, victim, 2000, FALSE)) return;
    gift_affect(victim, gsn_lycaon, APPLY_HITROLL, -10, 1, 0);
    gift_affect(victim, gsn_lycaon, APPLY_POLY, POLY_LYCAON, 1, AFF_POLYMORPH);
    free_string(victim->morph);
    victim->morph = str_dup("a spirit-cursed wolf");
    act("You bind $N in Lycaon's wolf form.", ch, NULL, victim, TO_CHAR);
    stc("Lycaon's curse wraps you in the form of a wolf.\n\r", victim);
}

void do_totemgift(CHAR_DATA *ch, char *argument)
{
    static const char *totems[] = {
        "Pegasus", "Rat", "Unicorn", "Stag", "Fenris", "Cockroach", "Griffin",
        "Grandfather Thunder", "Owl", "Falcon", "Chimera", "Uktena", "Wendigo"
    };
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    long tribe;
    if (!gift_caster(ch, TRUE)) return;
    one_argument(argument, arg);
    if (!str_cmp(arg, "dismiss"))
    {
        affect_strip(ch, gsn_totemgift);
        stc("You dismiss your tribal totem.\n\r", ch);
        return;
    }
    if (get_breed(ch, BREED_METIS) < 5)
    {
        stc("You require Metis 5 to use Totemgift.\n\r", ch);
        return;
    }
    if (is_affected(ch, gsn_totemgift))
    {
        stc("Your tribal totem is already present.\n\r", ch);
        return;
    }
    tribe = ch->pcdata->tgift[2];
    if (tribe < 0 || tribe >= (long)(sizeof(totems) / sizeof(totems[0]))
        || get_tribe(ch, tribe) < 1)
    {
        stc("You must know a gift of your selected tribe first.\n\r", ch);
        return;
    }
    if (!gift_mana(ch, 1500)) return;
    ch->mana -= 1500;
    WAIT_STATE(ch, 12);
    gift_affect(ch, gsn_totemgift, APPLY_AC, 20, 2, 0);
    gift_affect(ch, gsn_totemgift, APPLY_HITROLL, 15, 2, 0);
    format_buffer(buf, sizeof(buf), "You summon %s as a protective spirit totem.\n\r", totems[tribe]);
    stc(buf, ch);
    act("A protective tribal spirit appears beside $n.", ch, NULL, NULL, TO_ROOM);
}

void do_horridreality(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    if (!gift_caster(ch, FALSE)) return;
    if (get_disc(ch, DISC_CHIMERSTRY) < 7)
    {
        stc("You require Chimerstry 7 to use Horridreality.\n\r", ch);
        return;
    }
    if ((victim = gift_target(ch, argument, gsn_horridreality)) == NULL
        || !gift_attempt(ch, victim, 2000, TRUE)) return;
    gift_affect(victim, gsn_horridreality, APPLY_HITROLL, -25, 1, 0);
    gift_affect(victim, gsn_horridreality, APPLY_DAMROLL, -15, 1, 0);
    WAIT_STATE(victim, 8);
    act("You trap $N in a horrid reality of imagined wounds.", ch, NULL, victim, TO_CHAR);
    stc("Imagined wounds make your attacks falter.\n\r", victim);
}

void do_maskempathy(CHAR_DATA *ch, char *argument)
{
    if (!gift_caster(ch, FALSE)) return;
    if (is_affected(ch, gsn_maskempathy))
    {
        affect_strip(ch, gsn_maskempathy);
        stc("You lower your emotional mask.\n\r", ch);
        return;
    }
    if (get_disc(ch, DISC_PRESENCE) < 7)
    {
        stc("You require Presence 7 to use Maskempathy.\n\r", ch);
        return;
    }
    if (!gift_mana(ch, 1000)) return;
    ch->mana -= 1000;
    WAIT_STATE(ch, 12);
    gift_affect(ch, gsn_maskempathy, APPLY_NONE, 0, 2, 0);
    stc("You veil your aura behind an emotional mask.\n\r", ch);
}
