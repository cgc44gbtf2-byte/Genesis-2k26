#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include "merc.h"

long spellpower(CHAR_DATA *ch, long highest_sphere);
void add_paradox(CHAR_DATA *ch, long paradox, bool botch);
void backlash(CHAR_DATA *ch);
long correspondence_level(CHAR_DATA* ch, CHAR_DATA* victim);
void hearts_blood(CHAR_DATA* ch, long quint);

const char * sphere_name [9] =
{
    "Correspondence",	"Life",
    "Prime",		"Entropy",
    "Mind",		"Spirit",
    "Forces",		"Matter",
    "Time"
};

/*Powers in other files:
Forces 1: Nightsight: clan.c
Life 1: Life scan: clan.c
Mind 1: Heighten: clan.c
Mind 2: Scry: clan.c
Mind 3: Shield: clan.c
Matter 1: Analyze: warlock.c
*/

//mana costs

#define MANA_ENCHANT		5000
#define MANA_HOLY_SMOTE		4000
#define MANA_PARADOX_WARD	5000
#define MANA_INFERNO		5000
#define MANA_ELECTRICAL_CHAOS	3500
#define MANA_FRICTION_CURSE	3500
#define MANA_MAGE_HEAL		2500
#define MANA_BETTER_BODY	4900
#define MANA_BEGINNERS_LUCK	3500
#define MANA_INVIS		5000
#define MANA_ALTER_WEIGHT	6000
#define MANA_BRITTLE_BONES	2000
#define MANA_MUTATE_EPHEMERA    8000 
#define MANA_POLYMORPH_OTHER   13000
#define MANA_CONTINGENCY	2000
#define MANA_DOUBLE_CAST	3000
#define MANA_RIFT		 500
#define MANA_QUINT		 300


/* Validate integers before touching resources; retain cost/duration headroom. */
static bool mage_quintessence(CHAR_DATA *ch, char *text, long minimum, long *quint)
{
    char *end;
    long value;
    if (text[0] == '\0' && minimum == 0)
    {
        *quint = 0;
        return TRUE;
    }
    errno = 0;
    value = strtol(text, &end, 10);
    if (text == end || *end != '\0' || errno == ERANGE || value < minimum
        || value > (LONG_MAX - 1000000L) / 1000000L)
    {
        stc("Quintessence must be a valid nonnegative integer within range.\n\r", ch);
        return FALSE;
    }
    *quint = value;
    return TRUE;
}

/* Check all costs before spending anything. Heart's Blood supplies at most
 * five missing points, never a negative BODY balance or a mid-command death.
 */
/* Validate optional numeric arguments before spellpower can add paradox,
 * Heart's Blood can hurt the caster, or legacy target/cost code can run. */
static bool mage_numeric_input(CHAR_DATA *ch,char *argument,int position)
{
    char value[MAX_INPUT_LENGTH];
    long quint;
    int i;
    if (!ch || IS_NPC(ch) || !ch->pcdata || !ch->in_room
        || ch->hit<=0 || ch->position<POS_RESTING || IS_BUI(ch) || !argument
        || strlen(argument)>=MAX_INPUT_LENGTH) return FALSE;
    value[0]=0;
    for(i=0;i<position;++i) argument=one_argument(argument,value);
    if (*argument)
    {
        stc("Too many spell arguments.\n\r",ch);
        return FALSE;
    }
    return mage_quintessence(ch,value,0,&quint);
}

static bool mage_pay_cost(CHAR_DATA *ch, long quint, long base, long per_quint)
{
    long available = UMAX(0, ch->quint[BODY]);
    long missing = UMAX(0, quint - available);
    long cost = base + quint * per_quint;
    long damage = 0, i;
    char buf[MAX_STRING_LENGTH];
    if (missing > 0 && (!IS_XTRA(ch, XTRA_HEARTS_BLOOD) || missing > 5))
    {
        stc("You do not have enough quintessence; Heart's Blood supplies at most 5 missing points.\n\r", ch);
        return FALSE;
    }
    if (ch->mana < cost)
    {
        stc("You don't have enough mana to cast this spell.\n\r", ch);
        return FALSE;
    }
    for (i = 0; i < missing; i++)
        damage += number_range(200, 500);
    if (damage >= ch->hit)
    {
        stc("You do not have enough lifeforce for Heart's Blood.\n\r", ch);
        return FALSE;
    }
    if (damage > 0)
    {
        format_buffer(buf, sizeof(buf),
            "You channel your lifeforce into your spell using heart's blood. [#r%li#n]\n\r", damage);
        stc(buf, ch);
        hurt_person(ch, ch, damage);
    }
    ch->quint[BODY] = available - UMIN(available, quint);
    ch->mana -= cost;
    return TRUE;
}

/* A safety probe must never leave the caster in the target's room.
 * Support casts require self/group consent, not combat PK eligibility.
 * Hostile casts retain native protection and explicit PK/group checks.
 */
static bool mage_target_ready(CHAR_DATA *ch, CHAR_DATA *victim, bool support)
{
    ROOM_INDEX_DATA *origin, *destination;
    bool blocked;
    if (!ch || !victim || !ch->in_room || !victim->in_room
        || IS_BUI(ch) || IS_BUI(victim)) return FALSE;
    origin=ch->in_room; destination=victim->in_room;
    if (origin!=destination && IS_SET(origin->room_flags|destination->room_flags,
        ROOM_PRIVATE|ROOM_SOLITARY|ROOM_NO_TELEPORT))
    {
        stc("Remote magic cannot cross that room's protections.\n\r",ch);
        return FALSE;
    }
    if (support)
    {
        if ((victim!=ch && !is_same_group(ch,victim))
            || ((ch->affected_by^victim->affected_by)
                & (AFF_ETHEREAL|AFF_SHADOWPLANE|AFF_DEEPUMBRA)))
        {
            stc("Support magic requires yourself or a group member on your plane.\n\r",ch);
            return FALSE;
        }
        return TRUE;
    }
    if (victim==ch || is_same_group(ch,victim)
        || IS_SET(origin->room_flags|destination->room_flags,ROOM_SAFE)
        || (!IS_NPC(victim) && (!CAN_PK(ch) || !CAN_PK(victim))))
    {
        stc("That target is protected from hostile magic.\n\r",ch);
        return FALSE;
    }
    if (origin!=destination) { char_from_room(ch); char_to_room(ch,destination); }
    blocked=is_safe(ch,victim);
    if (ch->in_room!=origin) { char_from_room(ch); char_to_room(ch,origin); }
    return !blocked;
}

static CHAR_DATA *mage_attack_target(CHAR_DATA *ch, char *name)
{
    return name[0] ? get_char_world(ch,name) : ch->fighting;
}

static bool mage_roll(CHAR_DATA *ch, long sphere, long *power)
{
    ROOM_INDEX_DATA *origin=ch->in_room;
    *power=spellpower(ch,sphere);
    /* Backlash can incapacitate or relocate the caster. Do not continue a
     * paid cast from a corpse, replacement body or resurrection room.
     */
    return ch->in_room==origin && ch->hit>0 && ch->position>=POS_RESTING
        && !IS_BUI(ch);
}

typedef struct mage_time_spell {
    const char *name;
    void (*cast)(CHAR_DATA *,char *);
    int sphere,level,modes;
    long mana;
} MAGE_TIME_SPELL;
/* modes:1 Contingency,2 Doublecast. Preserve the original Doublecast list;
 * restore the two explicitly disabled Contingency damage selections.
 */
static const MAGE_TIME_SPELL mage_time_spells[]={
    {"brittlebones",do_brittle_bones,MENT,3,1,MANA_BRITTLE_BONES},
    {"mutateephemera",do_mutate_ephemera,MENT,5,3,MANA_MUTATE_EPHEMERA},
    {"electricalchaos",do_electrical_chaos,MFOR,2,3,MANA_ELECTRICAL_CHAOS},
    {"frictioncurse",do_friction_curse,MFOR,3,1,MANA_FRICTION_CURSE},
    {"inferno",do_inferno,MFOR,5,3,MANA_INFERNO},
    {"polymorphother",do_polymorph_other,MLIF,5,1,MANA_POLYMORPH_OTHER},
    {"alterweight",do_alterweight,MMAT,5,3,MANA_ALTER_WEIGHT},
    {"mageheal",do_mage_heal,MLIF,2,2,MANA_MAGE_HEAL}
};

static const MAGE_TIME_SPELL *mage_time_find(const char *name,int mode)
{
    size_t i;
    if (!name) return NULL;
    for(i=0;i<sizeof(mage_time_spells)/sizeof(mage_time_spells[0]);++i)
        if ((mage_time_spells[i].modes&mode) && !str_cmp(name,mage_time_spells[i].name))
            return &mage_time_spells[i];
    return NULL;
}

static bool mage_time_ready(CHAR_DATA *ch,const MAGE_TIME_SPELL *spell,
    char *argument,long *cost)
{
    CHAR_DATA *v;
    char name[MAX_INPUT_LENGTH],number[MAX_INPUT_LENGTH];
    char *rest;
    long quint,cor;
    if (!spell || ch->spheres[spell->sphere]<spell->level) return FALSE;
    if (spell->cast==do_inferno)
    {
        if (!mage_numeric_input(ch,argument,1)
            ) return FALSE;
        one_argument(argument,number);
        if (!mage_quintessence(ch,number,0,&quint)) return FALSE;
        if (quint>UMAX(0,ch->quint[BODY]) && (!IS_XTRA(ch,XTRA_HEARTS_BLOOD)
            || quint-UMAX(0,ch->quint[BODY])>5)) return FALSE;
        for(v=ch->in_room->people;v;v=v->next_in_room)
            if (v!=ch && !is_same_group(ch,v) && mage_target_ready(ch,v,FALSE)) break;
        if (!v) return FALSE;
        *cost=spell->mana+quint*MANA_QUINT;
        return TRUE;
    }
    if (!mage_numeric_input(ch,argument,2)) return FALSE;
    rest=one_argument(argument,name); one_argument(rest,number);
    if (!mage_quintessence(ch,number,0,&quint)) return FALSE;
    v=(spell->cast==do_mage_heal && !name[0]) ? ch : mage_attack_target(ch,name);
    if (!v || !v->in_room) return FALSE;
    cor=correspondence_level(ch,v);
    if (cor>ch->spheres[MCOR] || !mage_target_ready(ch,v,spell->cast==do_mage_heal))
        return FALSE;
    if (spell->cast==do_mage_heal && v->hit>=v->max_hit) return FALSE;
    if (spell->cast==do_polymorph_other
        && (IS_NPC(v) || is_affected(v,gsn_lycaon) || IS_POLYAFF(v,POLY_PIG))) return FALSE;
    if (spell->cast==do_mutate_ephemera && IS_XTRA(v,XTRA_MUTATE_EPHEMERA)) return FALSE;
    if (spell->cast==do_brittle_bones && is_affected(v,skill_lookup("brittle bones"))) return FALSE;
    if (spell->cast==do_friction_curse && is_affected(v,skill_lookup("friction curse"))) return FALSE;
    if ((spell->cast==do_alterweight
            && ((!IS_NPC(v) && cor==5) || (IS_NPC(v) && v->level>800)))
        || (spell->cast==do_electrical_chaos && cor==5
            && (!IS_NPC(v) || v->level>800))) return FALSE;
    if (quint>UMAX(0,ch->quint[BODY])
        && (!IS_XTRA(ch,XTRA_HEARTS_BLOOD) || quint-UMAX(0,ch->quint[BODY])>5)) return FALSE;
    *cost=spell->mana+cor*cor*100+quint*MANA_QUINT;
    return TRUE;
}

/* Matter 1: identify a carried object, like the existing Diagnostics gift. */
void do_analyze(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    long sn;

    if (!IS_MAGE(ch))
    {
        stc("Huh?\n\r", ch);
        return;
    }
    if (ch->spheres[MMAT] < 1)
    {
        stc("You require Matter 1 to use Analyze.\n\r", ch);
        return;
    }
    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        stc("Analyze which carried object?\n\r", ch);
        return;
    }
    if ((obj = get_obj_carry(ch, arg)) == NULL)
    {
        stc("You don't have that.\n\r", ch);
        return;
    }
    if ((sn = skill_lookup("identify")) < 0)
    {
        stc("Analyze is unavailable: the Identify spell is missing.\n\r", ch);
        return;
    }
    spell_identify(sn, 50, ch, obj);
}

//Correspondence 4: Rift
void do_rift( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    long power, quintessence;
    OBJ_DATA *in_gate;
    OBJ_DATA *to_gate;
    OBJ_INDEX_DATA *portal;
    ROOM_INDEX_DATA *pIn;
    ROOM_INDEX_DATA *pTo;


    if (!mage_numeric_input(ch,argument,2)) return;
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if (!IS_MAGE(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->spheres[MCOR] < 4)
    {
        send_to_char("You do not have enough knowledge of correspondence to use rift.\n\r",ch);
        return;
    }

    if ( arg1[0] == '\0' )
    {
        stc("Who do you wish to open a rift to?\n\r",ch);
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        stc("You cannot find them.\n\r",ch);
        return;
    }

    if (IS_NPC(victim) && (!victim->pIndexData || victim->level > 600
        || victim->pIndexData->vnum == MOB_VNUM_EYE || victim->pIndexData->vnum == MOB_VNUM_FROG
        || victim->pIndexData->vnum == MOB_VNUM_RAVEN || victim->pIndexData->vnum == MOB_VNUM_CAT
        || victim->pIndexData->vnum == MOB_VNUM_DOG))
    {
        act("You attempt to open an astral gate to $N, but fail.",ch,NULL,victim,TO_CHAR);
        act("$n attempts to open an astral gate, but fails.",ch,NULL,victim,TO_ROOM);
        return;
    }

    if ( !IS_NPC(victim) && !IS_EXTRA(victim, EXTRA_SUMMON) )
    {
        act("You attempt to open an astral gate to $N, but the rift quickly closes.",ch,NULL,victim,TO_CHAR);
        act("$n creates a magical rift before you, which quickly disappears.",ch,NULL,victim,TO_VICT);
        act("$n attempts to open an astral gate, but fails.",ch,NULL,victim,TO_ROOM);
        return;
    }

    if (victim->wizard == ch) 
    {  
  	  send_to_char("You can't open a rift to that!", ch);
        return;
    }
    if (!victim->in_room || victim->hit<=0 || IS_BUI(victim)
        || ch->in_room==victim->in_room || ch->fighting || victim->fighting
        || ch->fight_timer>0 || victim->fight_timer>0
        || ch->embraced!=ARE_NONE || IS_EXTRA(ch,TIED_UP)
        || IS_AFFECTED(ch,AFF_CURSE|AFF_WEBBED|AFF_SHADOWPLANE|AFF_DEEPUMBRA|AFF_ETHEREAL)
        || IS_AFFECTED(victim,AFF_SHADOWPLANE|AFF_DEEPUMBRA|AFF_ETHEREAL)
        || is_affected(ch,gsn_lineage_root)
        || IS_SET(ch->in_room->room_flags|victim->in_room->room_flags,
            ROOM_SAFE|ROOM_PRIVATE|ROOM_SOLITARY|ROOM_NO_RECALL|ROOM_NO_TELEPORT))
    {
        stc("Combat, restraint or room/plane protections block that Rift.\n\r",ch);
        return;
    }

    if (!mage_quintessence(ch,arg2,0,&quintessence)) return;
    portal=NULL;
    if (quintessence>0 && !(portal=get_obj_index(OBJ_VNUM_PORTAL)))
    {
        stc("The Rift portal template is unavailable.\n\r",ch); return;
    }
    if (!mage_pay_cost(ch,quintessence,MANA_RIFT,0) || !mage_roll(ch,4,&power)) return;
    if (power <= 0)
    {
        stc("You failed to cast Rift.\n\r", ch);
        return;
    }
    power += quintessence;
   
    if (quintessence == 0)
    {
	
	act("You create a small rent in the Tapestry to $N and step through it.",ch,NULL,victim,TO_CHAR);
        act("A small tear in reality pops into existance in front of you and $n steps out from it.",ch,NULL,victim,TO_VICT); 
        act("$n creates a small tear in reality and steps through it.",ch,NULL,victim,TO_ROOM);
	char_from_room(ch);
	char_to_room(ch, victim->in_room);
	do_look(ch, "");
    }
    else
    {
	act("You create a small rent in the Tapestry to $N.",ch,NULL,victim,TO_CHAR);
        act("A small tear in reality pops into existance in front of you.",ch,NULL,victim,TO_VICT); 
        act("$n creates a small tear in reality.",ch,NULL,victim,TO_ROOM);

	pIn = ch->in_room;
	pTo = victim->in_room;

    in_gate = create_object( portal, 0 );
	obj_to_room( in_gate, pIn );
	in_gate->value[0] = victim->in_room->vnum;
	in_gate->value[1] = 1;
	in_gate->value[3] = ch->in_room->vnum;
	in_gate->timer = power;
	in_gate->item_type = ITEM_WGATE;

    to_gate = create_object( portal, 0 );
	obj_to_room( to_gate, pTo );
	to_gate->value[0] = ch->in_room->vnum;
	to_gate->value[1] = 2;
	to_gate->value[3] = victim->in_room->vnum;
	to_gate->timer = power;
	to_gate->item_type = ITEM_WGATE;
	

    }
  
}


//Entropy 2: Beginner's luck

void do_beginners_luck(CHAR_DATA* ch, char * argument)
{
    char       arg [MAX_INPUT_LENGTH];
    long power, quintessence;

    if (!IS_MAGE(ch))
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (ch->spheres[MENT] < 2)
    {
        send_to_char("You do not have enough knowledge of entropy to use beginners luck.\n\r",ch);
        return;
    }

    if (ch->beginners_luck > 0)
    {
	send_to_char("You are already affected by beginners luck.\n\r", ch);
	return;
    }

    one_argument( argument, arg );

    if (!mage_quintessence(ch, arg, 0, &quintessence)
        || !mage_pay_cost(ch, quintessence, MANA_BEGINNERS_LUCK, 0))
        return;

    power = spellpower(ch, 2); //Beginner's luck uses lvl 2 entropy

    if (power <= 0)
    {
        send_to_char("You failed to cast Beginner's Luck.\n\r", ch);
        return;
    }

    power += quintessence;


	send_to_char("You are now effected by beginner's luck.\n\r", ch);
	ch->beginners_luck = power;
}

//Entropy 3: Brittle Bones
void do_brittle_bones(CHAR_DATA * ch, char * argument)
{

    CHAR_DATA * victim;
    AFFECT_DATA af;
    char arg1[MIL];
    char arg2[MIL];
    long power, quint, sn, correspondence;
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *to_room;
    bool protected;

    if (!ch || !ch->in_room) return;
    if (!mage_numeric_input(ch,argument,2)) return;
    quint = 0;
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (!IS_MAGE(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->spheres[MENT] < 3)
    {
        send_to_char("You do not have enough knowledge of the entropy sphere to use brittle bones.\n\r", ch);
        return;
    }


    if (!mage_quintessence(ch,arg2,0,&quint)) return;

   // if (victim != ch) power -= 1; //costs 1 success to target someone else

    if ((victim = mage_attack_target(ch,arg1)) == NULL)
    {
        stc("They aren't here.\n\r", ch);
        return;
    }

    if (!victim->in_room) return;
    correspondence = correspondence_level(ch, victim);

    if (correspondence > ch->spheres[MCOR])
    {
	send_to_char("You don't not have enough knowledge of correspondence to target them.\n\r", ch);
	return;
    }

    sn=skill_lookup("brittle bones");
    if (sn<0 || is_affected(victim,sn)) {
        stc("Brittle Bones is unavailable or already affects that target.\n\r",ch); return;
    }
    to_room =  victim->in_room;
    location = ch->in_room;
    if (IS_BUI(victim) || (victim!=ch && is_same_group(ch,victim))
        || (victim!=ch && !IS_NPC(victim) && (!CAN_PK(ch) || !CAN_PK(victim)))
        || IS_SET(location->room_flags|to_room->room_flags,
            ROOM_SAFE|ROOM_PRIVATE|ROOM_SOLITARY|ROOM_NO_TELEPORT)) return;
    protected=FALSE;
    if (victim!=ch) {
        if (location!=to_room) { char_from_room(ch); char_to_room(ch,to_room); }
        protected=is_safe(ch,victim);
        if (ch->in_room!=location) { char_from_room(ch); char_to_room(ch,location); }
    }
    if (protected || !mage_pay_cost(ch,quint,
        MANA_BRITTLE_BONES+correspondence*correspondence*100,MANA_QUINT)) return;
    power=spellpower(ch,3)+quint-correspondence;
    if (power<=0) { stc("Brittle Bones fails to take effect.\n\r",ch); return; }

    if (!IS_NPC(victim) && victim->pcdata->atm >= number_percent())
    {
	stc("They resisted your spell.\n\r", ch);
	return;
    }

        act("You make $N's bones become weak and fragile.",ch,NULL,victim,TO_CHAR);
        act("$n makes $N's bones become weak and fragile.",ch,NULL,victim,TO_NOTVICT);
        act("$n makes your bones become weak and fragile.",ch,NULL,victim,TO_VICT);

    memset(&af,0,sizeof(af));
    af.type = sn;
    af.location = APPLY_NONE;
    af.modifier = power * 2.5;
    af.duration = power;
    af.bitvector = 0;

    affect_to_char( victim, &af );
    WAIT_STATE(ch, 8);
}

//Entropy 5: Mutate Ephemera
void do_mutate_ephemera(CHAR_DATA* ch, char * argument)
{

char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
    CHAR_DATA * victim;
    long power, quint, correspondence;

    if (!mage_numeric_input(ch,argument,2)) return;
    quint = 0;
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (!IS_MAGE(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->spheres[MENT] < 5)
    {
        send_to_char("You do not have enough knowledge of the entropy sphere to use mutate ephemera.\n\r", ch);
        return;
    }

    if (!mage_quintessence(ch,arg2,0,&quint)) return;

   // if (victim != ch) power -= 1; //costs 1 success to target someone else

    if ((victim = mage_attack_target(ch,arg1)) == NULL)
    {
        stc("They aren't here.\n\r", ch);
        return;
    }

    correspondence = correspondence_level(ch, victim);

    if (correspondence > ch->spheres[MCOR])
    {
        send_to_char("You don't not have enough knowledge of correspondence to target them.\n\r", ch);
        return;
    }

    if (IS_XTRA(victim,XTRA_MUTATE_EPHEMERA))
    {
        stc("They are already affected by Mutate Ephemera.\n\r",ch);
        return;
    }
    if (!mage_target_ready(ch,victim,FALSE) || !mage_pay_cost(ch,quint,
        MANA_MUTATE_EPHEMERA+correspondence*correspondence*100,MANA_QUINT)) return;
    if (!mage_roll(ch,5,&power)) return;
    power+=quint+2-correspondence;

    if (!IS_NPC(victim) && victim->pcdata->atm >= number_percent())
    {
        stc("They resisted your spell.\n\r", ch);
        return;
    }

        act("You twist fate against $N.",ch,NULL,victim,TO_CHAR);
        act("$n twists fate against $N.",ch,NULL,victim,TO_NOTVICT);
        act("$n twists fate against you.",ch,NULL,victim,TO_VICT);

	if (power < 1) power = 1;

	if (!IS_XTRA(victim, XTRA_MUTATE_EPHEMERA)) SET_BIT(victim->xtra, XTRA_MUTATE_EPHEMERA);
	victim->ephemera_timer = power * 4; //need to find something that lasts decently, but not too long....

        WAIT_STATE( victim, 25 );
        WAIT_STATE( ch, 15 );

    return;

}

//Matter 2: Destroy Structures
void do_destroy_sculpture(CHAR_DATA* ch, char * argument)
{
    OBJ_DATA  *obj;
    EXIT_DATA *pexit;
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *to_room;
    char       arg [MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    long       direction, rev;

one_argument( argument, arg );

 if (!IS_MAGE(ch))
    {
        send_to_char("Huh?\n\r",ch);
        return;
    }

    if (ch->spheres[MMAT] < 2)
    {
        send_to_char("You do not have enough knowledge of matter to use sculpture.\n\r",ch);
        return;
    }

    if ( arg[0] == '\0')
    {
	//check all directions for walls if no choice is made
	direction = 6; //to check if there isn't a wall

        if ( ( pexit = ch->in_room->exit[DIR_NORTH] ) != NULL )
	if (IS_SET(pexit->exit_info, EX_WALL_WATER) ||
        IS_SET(pexit->exit_info, EX_WALL_GRANITE) ||
        IS_SET(pexit->exit_info, EX_WALL_WIND) ||
        IS_SET(pexit->exit_info, EX_WALL_FLAME)) direction = DIR_NORTH;

	if ( ( pexit = ch->in_room->exit[DIR_SOUTH] ) != NULL )
        if (IS_SET(pexit->exit_info, EX_WALL_WATER) ||
        IS_SET(pexit->exit_info, EX_WALL_GRANITE) ||
        IS_SET(pexit->exit_info, EX_WALL_WIND) ||
        IS_SET(pexit->exit_info, EX_WALL_FLAME)) direction = DIR_SOUTH;

	if ( ( pexit = ch->in_room->exit[DIR_EAST] ) != NULL ) 
        if (IS_SET(pexit->exit_info, EX_WALL_WATER) ||
        IS_SET(pexit->exit_info, EX_WALL_GRANITE) ||
        IS_SET(pexit->exit_info, EX_WALL_WIND) ||
        IS_SET(pexit->exit_info, EX_WALL_FLAME)) direction = DIR_EAST;

	if ( ( pexit = ch->in_room->exit[DIR_WEST] ) != NULL ) 
        if (IS_SET(pexit->exit_info, EX_WALL_WATER) ||
        IS_SET(pexit->exit_info, EX_WALL_GRANITE) ||
        IS_SET(pexit->exit_info, EX_WALL_WIND) ||
        IS_SET(pexit->exit_info, EX_WALL_FLAME)) direction = DIR_WEST;

	if ( ( pexit = ch->in_room->exit[DIR_UP] ) != NULL )
        if (IS_SET(pexit->exit_info, EX_WALL_WATER) ||
        IS_SET(pexit->exit_info, EX_WALL_GRANITE) ||
        IS_SET(pexit->exit_info, EX_WALL_WIND) ||
        IS_SET(pexit->exit_info, EX_WALL_FLAME)) direction = DIR_UP;

	if ( ( pexit = ch->in_room->exit[DIR_DOWN] ) != NULL )
        if (IS_SET(pexit->exit_info, EX_WALL_WATER) ||
        IS_SET(pexit->exit_info, EX_WALL_GRANITE) ||
        IS_SET(pexit->exit_info, EX_WALL_WIND) ||
        IS_SET(pexit->exit_info, EX_WALL_FLAME)) direction = DIR_DOWN;


	if (direction == 6)
	{
	    send_to_char("There are no walls that you can destory in this room.\n\r", ch);
	    return;
	}
    }

	//making sure a valid direction was selected
       if (!str_cmp(arg,"north") || !str_cmp(arg,"n"))
        {direction = DIR_NORTH; rev = DIR_SOUTH;}
    else if (!str_cmp(arg,"south") || !str_cmp(arg,"s"))
        {direction = DIR_SOUTH; rev = DIR_NORTH;}
    else if (!str_cmp(arg,"east")  || !str_cmp(arg,"e"))
        {direction = DIR_EAST;  rev = DIR_WEST;}
    else if (!str_cmp(arg,"west")  || !str_cmp(arg,"w"))
        {direction = DIR_WEST;  rev = DIR_EAST;}
    else if (!str_cmp(arg,"up")    || !str_cmp(arg,"u"))
        {direction = DIR_UP;    rev = DIR_DOWN;}
    else if (!str_cmp(arg,"down")  || !str_cmp(arg,"d"))
        {direction = DIR_DOWN;  rev = DIR_UP;}
    else
    {
        send_to_char( "Select direction: North, South, East, West, Up or Down.\n\r", ch );
        return;
    }

switch (direction)
    {
        default:        strcpy(arg,"north"); break;
        case DIR_NORTH: strcpy(arg,"north"); break;
        case DIR_SOUTH: strcpy(arg,"south"); break;
        case DIR_EAST:  strcpy(arg,"east");  break;
        case DIR_WEST:  strcpy(arg,"west");  break;
        case DIR_UP:    strcpy(arg,"up");    break;
        case DIR_DOWN:  strcpy(arg,"down");  break;
    }

    if ( ( pexit = ch->in_room->exit[direction] ) == NULL )
    {
        send_to_char("Nothing happens.\n\r",ch);
        return;
    }

    if ( ( location = ch->in_room ) == NULL )
    {
        send_to_char("Nothing happens.\n\r",ch);
        return;
    }

    if ( ( to_room = pexit->to_room ) == NULL )
    {
        send_to_char("Nothing happens.\n\r",ch);
        return;
    }

    if (!IS_SET(pexit->exit_info, EX_WALL_WATER) &&
        !IS_SET(pexit->exit_info, EX_WALL_GRANITE) &&
        !IS_SET(pexit->exit_info, EX_WALL_WIND) &&
        !IS_SET(pexit->exit_info, EX_WALL_FLAME))
    {
        send_to_char("There is no a wall blocking that exit.\n\r",ch);
        return;
    }

    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
	if (obj->item_type == ITEM_WALL && obj->in_room == ch->in_room && obj->value[0] == direction)
	extract_obj(obj);
	else
	continue;
   }


    char_from_room(ch);
    char_to_room(ch, to_room);
    
   for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if (obj->item_type == ITEM_WALL && obj->in_room == ch->in_room && obj->value[0] == rev)
        extract_obj(obj);
        else
        continue;
   }

    char_from_room(ch);
    char_to_room(ch, location);
    format_buffer(buf, sizeof(buf), "You destroy the wall to the %s.\n\r", arg); stc(buf, ch);
    format_buffer(buf, sizeof(buf), "$n shatters the wall to the %s.", arg);
    act(buf, ch, NULL, NULL, TO_ROOM);


}


//Mattert 3: Scrupture
void do_sculpture( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA  *obj;
    EXIT_DATA *pexit;
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *to_room;
    char       arg [MAX_INPUT_LENGTH];
    char       buf [MAX_INPUT_LENGTH];
    long        duration = number_range(30,90);
    long       direction, rev;

    one_argument( argument, arg );

    if (IS_NPC(ch)) return;

    if (!IS_MAGE(ch))
    {
        send_to_char("Huh?\n\r",ch);
        return;
    }

    if (ch->spheres[MMAT] < 3)
    {
        send_to_char("You do not have enough knowledge of matter to use sculpture.\n\r",ch);
        return;
    }

    if ( arg[0] == '\0')
    {
        send_to_char( "Which direction do you wish to make the wall?\n\r", ch );
        return;
    }

         if (!str_cmp(arg,"north") || !str_cmp(arg,"n"))
        {direction = DIR_NORTH; rev = DIR_SOUTH;}
    else if (!str_cmp(arg,"south") || !str_cmp(arg,"s"))
        {direction = DIR_SOUTH; rev = DIR_NORTH;}
    else if (!str_cmp(arg,"east")  || !str_cmp(arg,"e"))
        {direction = DIR_EAST;  rev = DIR_WEST;}
    else if (!str_cmp(arg,"west")  || !str_cmp(arg,"w"))
        {direction = DIR_WEST;  rev = DIR_EAST;}
    else if (!str_cmp(arg,"up")    || !str_cmp(arg,"u"))
        {direction = DIR_UP;    rev = DIR_DOWN;}
    else if (!str_cmp(arg,"down")  || !str_cmp(arg,"d"))
        {direction = DIR_DOWN;  rev = DIR_UP;}
    else
    {
        send_to_char( "Select direction: North, South, East, West, Up or Down.\n\r", ch );
        return;
    }
switch (direction)
    {
        default:        strcpy(arg,"north"); break;
        case DIR_NORTH: strcpy(arg,"north"); break;
        case DIR_SOUTH: strcpy(arg,"south"); break;
        case DIR_EAST:  strcpy(arg,"east");  break;
        case DIR_WEST:  strcpy(arg,"west");  break;
        case DIR_UP:    strcpy(arg,"up");    break;
        case DIR_DOWN:  strcpy(arg,"down");  break;
    }

    if ( ( pexit = ch->in_room->exit[direction] ) == NULL )
    {
        send_to_char("Nothing happens.\n\r",ch);
        return;
    }

    if ( ( location = ch->in_room ) == NULL )
    {
        send_to_char("Nothing happens.\n\r",ch);
        return;
    }

    if ( ( to_room = pexit->to_room ) == NULL )
    {
        send_to_char("Nothing happens.\n\r",ch);
        return;
    }

    if (IS_SET(pexit->exit_info, EX_WALL_WATER) ||
        IS_SET(pexit->exit_info, EX_WALL_GRANITE) ||
        IS_SET(pexit->exit_info, EX_WALL_WIND) ||
        IS_SET(pexit->exit_info, EX_WALL_FLAME))
    {
        send_to_char("There is already a wall blocking that exit.\n\r",ch);
        return;
    }

    act( "A look of concentration crosses $n's face.", ch, NULL, NULL, TO_ROOM );
obj = create_object( get_obj_index( OBJ_VNUM_PORTAL ), 0 );
    obj->item_type = ITEM_WALL;
    obj->value[0] = direction;
    obj->timer = duration;
    free_string(obj->name);
    obj->name = str_dup("wall stone granite");
    free_string(obj->short_descr);
    obj->short_descr = str_dup("a wall of granite");
    format_buffer(buf, sizeof(buf), "A wall of solid granite blocks your exit %s.",arg);
    free_string(obj->description);
    obj->description = str_dup(buf);
    obj_to_room( obj, ch->in_room );

    SET_BIT(pexit->exit_info, EX_WALL_GRANITE);

    if (direction == DIR_UP)
        sprintf(buf,"Stone erupts from the ground and forms a wall above you.");
    else if (direction == DIR_DOWN)
        sprintf(buf,"Stone erupts from the ground and forms a wall below you.");
    else
        format_buffer(buf, sizeof(buf), "Stone erupts from the ground and forms a wall to the %s.",arg);
    act( buf, ch, NULL, NULL, TO_CHAR );
    act( buf, ch, NULL, NULL, TO_ROOM );
    WAIT_STATE(ch,12);

    if (location == to_room) return;

    char_from_room(ch);
    char_to_room(ch, to_room);

    if ( ( pexit = ch->in_room->exit[rev] ) == NULL ) return;
    if (IS_SET(pexit->exit_info, EX_WALL_WATER) ||
        IS_SET(pexit->exit_info, EX_WALL_GRANITE) ||
        IS_SET(pexit->exit_info, EX_WALL_WIND) ||
        IS_SET(pexit->exit_info, EX_WALL_FLAME)) return;
switch (rev)
    {
        default:        strcpy(arg,"north"); break;
        case DIR_NORTH: strcpy(arg,"north"); break;
        case DIR_SOUTH: strcpy(arg,"south"); break;
        case DIR_EAST:  strcpy(arg,"east");  break;
        case DIR_WEST:  strcpy(arg,"west");  break;
        case DIR_UP:    strcpy(arg,"up");    break;
        case DIR_DOWN:  strcpy(arg,"down");  break;
    }

    obj = create_object( get_obj_index( OBJ_VNUM_PORTAL ), 0 );
    obj->item_type = ITEM_WALL;
    obj->value[0] = rev;
    obj->timer = duration;
    free_string(obj->name);
    obj->name = str_dup("wall stone granite");
    free_string(obj->short_descr);
    obj->short_descr = str_dup("a wall of granite");
    format_buffer(buf, sizeof(buf), "A wall of solid granite blocks your exit %s.",arg);
    free_string(obj->description);
    obj->description = str_dup(buf);
    obj_to_room( obj, ch->in_room );

    SET_BIT(pexit->exit_info, EX_WALL_GRANITE);

    if (rev == DIR_UP)
        sprintf(buf,"Stone erupts from the ground and forms a wall above you.");
    else if (rev == DIR_DOWN)
        sprintf(buf,"Stone erupts from the ground and forms a wall below you.");
    else
        format_buffer(buf, sizeof(buf), "Stone erupts from the ground and forms a wall to the %s.",arg);
    act( buf, ch, NULL, NULL, TO_ROOM );
    char_from_room(ch);
    char_to_room(ch, location);

    return;
}

//Matter 5: Alter Weight
void do_alterweight( CHAR_DATA *ch, char * argument)
{

    char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH],  buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],buf3[MAX_STRING_LENGTH];
    CHAR_DATA * victim;
    AFFECT_DATA af;
    long dam, power, quint, sn, correspondence;

    if (!mage_numeric_input(ch,argument,2)) return;
    dam = 0;
    quint = 0;
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (!IS_MAGE(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->spheres[MMAT] < 5)
    {
        send_to_char("You do not have enough knowledge of the matter sphere to use alter weight.\n\r", ch);
        return;
    }

    if (!mage_quintessence(ch,arg2,0,&quint)) return;

   // if (victim != ch) power -= 1; //costs 1 success to target someone else

    if ((victim = mage_attack_target(ch,arg1)) == NULL)
    {
        stc("They aren't here.\n\r", ch);
        return;
    }

    correspondence = correspondence_level(ch, victim);

    if (correspondence > ch->spheres[MCOR])
    {
        send_to_char("You don't not have enough knowledge of correspondence to target them.\n\r", ch);
        return;
    }

    if ((!IS_NPC(victim) && correspondence == 5) || (IS_NPC(victim) && victim->level > 800))
    {
        stc("You may not target them globally.\n\r", ch);
        return;
    }
    if (!mage_target_ready(ch,victim,FALSE) || !mage_pay_cost(ch,quint,
        MANA_ALTER_WEIGHT+correspondence*correspondence*100,MANA_QUINT)) return;
    if (!mage_roll(ch,5,&power)) return;
    power=UMAX(1,power-correspondence+quint);

    if (!IS_NPC(victim) && victim->pcdata->atm >= number_percent())
    {
        stc("They resisted your spell.\n\r", ch);
        return;
    }

    dam = number_range(900 * power, 1400 * power);

    if (ch->fighting != victim && ch != victim && victim->fighting == NULL)  set_fighting(victim, ch);

    sprintf( buf1, "$n's increases the gravity around $N crushing them under their own weight." );
    sprintf( buf2, "You increase the gravity around $N crushing them under their own weight.[#g%li#n]", dam );
    sprintf( buf3, "$n has increased the gravity around you crushing you under your own weight.[#r%li#n]", dam );

    act( buf1, ch, NULL, victim, TO_NOTVICT );
    act( buf2, ch, NULL, victim, TO_CHAR );
    act( buf3, ch, NULL, victim, TO_VICT );


    hurt_person(ch, victim, dam);
    WAIT_STATE(ch, 8);

    if ((sn = skill_lookup("Alter Weight")) < 0)
    {
        stc("Buggyness! So umm, time to go yell at Dude... ;)\n\r",ch);
        return;
    }

    if ( is_affected (victim, sn ) ) return;

    af.type = sn;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.duration = power * 10;
    af.bitvector = 0;

    affect_to_char( victim, &af );




}

//Mind 5: Invis
void do_mage_invis( CHAR_DATA *ch, char * argument)
{
    char arg[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
    AFFECT_DATA af;
    long power, sn, quintessence;

    one_argument( argument, arg);

    quintessence = 0;

    if (!IS_MAGE(ch))
    {
    send_to_char("Huh?\n\r", ch);
    return;
    }

    if (ch->spheres[MMIN] < 5)
    {
        send_to_char("You don't have enough knoweldge of the mind sphere to use invisability.\n\r", ch);
        return;
    }

    if ((sn = skill_lookup("mage invis")) < 0 || is_affected(ch, sn))
    {
        stc("Mage invisibility is unavailable or already active.\n\r", ch);
        return;
    }
    if (!mage_quintessence(ch, arg, 0, &quintessence)
        || !mage_pay_cost(ch, quintessence, MANA_INVIS, MANA_QUINT))
        return;

    power = spellpower(ch, 5); //Invisability uses lvl 5 mind

    if (power <= 0)
    {
        send_to_char("You failed to cast Invisability.\n\r", ch);
        return;
    }

    power += quintessence;

    af.type = sn;
    af.location = APPLY_NONE;
    af.modifier = quintessence;
    af.duration = power * 3;
    af.bitvector = 0;

    affect_to_char( ch, &af );

    sprintf( buf1, "$n fades out of sight." );
    sprintf( buf2, "You block your vision out of other people's minds.");

    act( buf1, ch, NULL, ch, TO_NOTVICT );
    act( buf2, ch, NULL, ch, TO_CHAR );

    WAIT_STATE(ch, 12);
}


//Spirit 1: Umbral vision
void do_spirit_sense( CHAR_DATA *ch, char *argument )
{
    char      arg [MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );

    if (IS_NPC(ch)) return;
    if (!IS_MAGE(ch))
    {
        send_to_char("Huh?\n\r",ch);
        return;
    }

     if (ch->spheres[MSPI] < 1)
    {
        send_to_char("You don't have enough knoweldge of the spirit sphere to use spirit sense.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch,AFF_SHADOWSIGHT) )
    {
        send_to_char("You can no longer see between planes.\n\r",ch);
        REMOVE_BIT(ch->affected_by, AFF_SHADOWSIGHT);
        return;
    }
    send_to_char("You can now see between planes.\n\r",ch);
    SET_BIT(ch->affected_by, AFF_SHADOWSIGHT);
    return;
}


//Spirit 2: Spirit caress
void do_spirit_caress(CHAR_DATA* ch, char* argument)
{

    if(!IS_MAGE(ch))
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (ch->spheres[MSPI] < 2)
    {
        send_to_char("You don't have enough knoweldge of the spirit sphere to use spirit caress.\n\r", ch);
        return;
    }

    if (IS_SET(ch->xtra, XTRA_MAGE_CARESS))
    {
	send_to_char("You no longer reach into the umbra.\n\r", ch);
	REMOVE_BIT(ch->xtra, XTRA_MAGE_CARESS);
    }
    else
    {
	send_to_char("You now reach into the umbra.\n\r", ch);
        SET_BIT(ch->xtra, XTRA_MAGE_CARESS);
    }

}

//Spirit 3: Step Sideways
void do_step_sideways(CHAR_DATA* ch, char * argument)
{
    char arg[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
    long power, quint, dam, loop;

    if (!mage_numeric_input(ch,argument,1)) return;
    one_argument( argument, arg);

    if (!IS_MAGE(ch))
    {
    send_to_char("Huh?\n\r", ch);
    return;
    }

    if (ch->spheres[MSPI] < 3)
    {
        send_to_char("You don't have enough knoweldge of the spirit sphere to step sideways.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_DEEPUMBRA))
    {
	send_to_char("You must leave the deep umbra first.\n\r", ch);
	return;
    }

    if (!mage_quintessence(ch,arg,0,&quint)) return;
    if (IS_SET(ch->in_room->room_flags,ROOM_NO_TELEPORT))
    {
        stc("This room prevents planar travel.\n\r",ch);
        return;
    }
    if (!mage_pay_cost(ch,quint,0,0) || !mage_roll(ch,3,&power)) return;
    if (power <= 0)
    {
	send_to_char("You failed to step through the gauntlet.\n\r", ch);
	return;
    }

    power = quint;
    dam = 0;

    for(loop = 0; loop < (ch->arete + ch->paradox - power); loop++)
    {
	if (number_range(1, 10) >= 6)	dam += number_range(1, 1000);
    }

    if (!IS_AFFECTED(ch, AFF_SHADOWPLANE))
    {
	send_to_char("You step sideways through the gauntlet.\n\r",ch);
	act("$n steps sideways through the gauntlet.",ch,NULL,NULL,TO_ROOM);
	SET_BIT(ch->affected_by, AFF_SHADOWPLANE);
	do_look(ch,"auto");
    }

    else
    {
	REMOVE_BIT(ch->affected_by, AFF_SHADOWPLANE);
	send_to_char("You step sideways back into the real world.\n\r",ch);
	act("$n step sideways through the gauntlet and back into existance.",ch,NULL,NULL,TO_ROOM);
	do_look(ch,"auto");
    }
    sprintf(buf, "As you step sideways the gauntlet batters you harshly. [#r%li#n]\n\r", dam);
    send_to_char(buf, ch);
    hurt_person(ch, ch, dam);
    update_pos(ch);
    return;
}

//Spirit 5: Breaking the dreamshell
void do_break_dreamshell(CHAR_DATA* ch, char * argument)
{
    char arg[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
    long power, quint, dam, loop;

    if (!mage_numeric_input(ch,argument,1)) return;
    one_argument( argument, arg);

    if (!IS_MAGE(ch))
    {
    send_to_char("Huh?\n\r", ch);
    return;
    }

    if (ch->spheres[MSPI] < 5)
    {
        send_to_char("You don't have enough knoweldge of the spirit sphere to break the dreamshell.\n\r", ch);
        return;
    }

    if (!IS_AFFECTED(ch, AFF_SHADOWPLANE))
    {
	send_to_char("You must been in the umbra before you can attempt to travel to the deep umbra.\n\r", ch);
	return;
    }

    if (!mage_quintessence(ch,arg,0,&quint)) return;
    if (IS_SET(ch->in_room->room_flags,ROOM_NO_TELEPORT))
    {
        stc("This room prevents planar travel.\n\r",ch);
        return;
    }
    if (!mage_pay_cost(ch,quint,0,0) || !mage_roll(ch,5,&power)) return;
    if (power <= 0)
    {
        send_to_char("You failed to step through the dreamshell.\n\r", ch);
        return;
    }

    power = quint;
    dam = 0;

    for(loop = 0; loop < (ch->arete + ch->paradox - power); loop++)
    {
        if (number_range(1, 10) >= 6)   dam += number_range(500, 2000);
    }

    if (!IS_AFFECTED(ch, AFF_DEEPUMBRA))
    {
        send_to_char("You break through the dreamshell into the deep umbra.\n\r",ch);
        act("$n breaks through the dreamshell into the deep umbra.",ch,NULL,NULL,TO_ROOM);
        SET_BIT(ch->affected_by, AFF_DEEPUMBRA);
        do_look(ch,"auto");
    }

    else
    {
        REMOVE_BIT(ch->affected_by, AFF_DEEPUMBRA);
        send_to_char("You break back through the dreamshell to the near umbra\n\r",ch);
        act("$n breaks back through the dreamshell to the near umbra.",ch,NULL,NULL,TO_ROOM);
        do_look(ch,"auto");
    }
    sprintf(buf, "As you break through the dreamshell you are battered you harshly. [#r%li#n]\n\r", dam);
    send_to_char(buf, ch);
    hurt_person(ch, ch, dam);
    update_pos(ch);
    return;
}



long correspondence_level(CHAR_DATA* ch, CHAR_DATA* victim)
{
    long level;
    if (!ch || !victim || !ch->in_room || !victim->in_room) return 7;
    
    if (ch == victim) level = 1;
    else if (ch->in_room == victim->in_room) level = 2;
    else if (ch->in_room->area == victim->in_room->area) level = 5;
    else  level = 6;
    return level;
}

void hearts_blood(CHAR_DATA *ch, long quint)
{
    char buf[MAX_STRING_LENGTH];
    long heart, loop, dam;

    heart = quint - ch->quint[BODY];

    dam = 0;

    for(loop = 0; loop < heart; loop++)
    {
        dam += number_range(200, 500);
    }
  
    sprintf(buf, "You channel your lifeforce into your spell using heart's blood. [#r%li#n]\n\r", dam);
    stc(buf, ch);
    hurt_person(ch, ch, dam);

    return;
}

//Prime 1: Heart's Blood
void do_hearts_blood( CHAR_DATA *ch, char * argument)
{


    if (!IS_MAGE(ch))
    {
    send_to_char("Huh?\n\r", ch);
    return;
    }

    if (ch->spheres[MPRI] < 1)
    {
        send_to_char("You don't have enough knoweldge of the prime sphere to use heart's blood.\n\r", ch);
        return;
    }


    if (IS_XTRA(ch, XTRA_HEARTS_BLOOD))
    {
	send_to_char("You no longer use heart's blood to power your spells.\n\r", ch);
	REMOVE_BIT(ch->xtra, XTRA_HEARTS_BLOOD);
	return;
    }

    SET_BIT(ch->xtra, XTRA_HEARTS_BLOOD);
    send_to_char("You now power your spells with heart's blood.\n\r", ch);
}

//Prime 2: Holy Smote
void do_holy_smote(CHAR_DATA *ch, char * argument)
{

    long power, quint;
    if (!mage_numeric_input(ch,argument,0)) return;

    if (!IS_MAGE(ch))
    {
    send_to_char("Huh?\n\r", ch);
    return;
    }

    if (ch->spheres[MPRI] < 2)
    {
        send_to_char("You don't have enough knoweldge of the prime sphere to use holy smote.\n\r", ch);
        return;
    }

    if (IS_XTRA(ch, XTRA_HOLY_SMOTE))
    {
	send_to_char("You holy sword fades out of existance.\n\r", ch);
	act("$n's holy sword slows fades out of existance.", ch, NULL, NULL, TO_ROOM);
	REMOVE_BIT(ch->xtra, XTRA_HOLY_SMOTE);
	return;
    }
    
    quint = 2;

    if (!mage_pay_cost(ch,quint,MANA_HOLY_SMOTE,0)
        || !mage_roll(ch,2,&power)) return;

    if (power <= 0)
    {
        send_to_char("You failed to cast Holy Smote.\n\r", ch);
        return;
    }

    send_to_char("You create a floating holy sword out of quintessence to aid you.\n\r", ch);
    act("$n summons a floating holy sword.", ch, NULL, NULL, TO_ROOM);
    SET_BIT(ch->xtra, XTRA_HOLY_SMOTE);

}

//Prime 3: Enchant Weapon
void do_mageenchant(CHAR_DATA *ch, char * argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA * obj;
    long power, quintessence;
    /* One carried-object argument, no numeric or trailing options. */
    if (!ch || IS_NPC(ch) || !ch->pcdata || !ch->in_room || !argument
        || ch->hit<=0 || ch->position<POS_RESTING || IS_BUI(ch)
        || strlen(argument)>=MAX_INPUT_LENGTH) return;

    if (!IS_MAGE(ch))
    {
    send_to_char("Huh?\n\r", ch);
    return;
    }

    if (ch->spheres[MPRI] < 3)
    {
        send_to_char("You don't have enough knoweldge of the prime sphere to use enchant.\n\r", ch);
        return;
    }

     argument = one_argument( argument, arg );
    if (*argument) { stc("Use mageenchant <carried weapon>.\n\r",ch); return; }

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
        stc("You are not carrying that object.\n\r",ch);
        return;
    }

    if ( obj->item_type != ITEM_WEAPON )
    {
        act("$p is not a weapon.",ch,obj,NULL,TO_CHAR);
        return;
    }

    if ( IS_SET(obj->quest, QUEST_AGGRAVATED))
    {
        send_to_char("This weapon has already been enchanted.\n\r", ch);
        return;
    }

    if ( IS_SET(obj->quest, QUEST_ARTIFACT) || IS_SET(obj->quest, QUEST_RELIC) )
    {
        send_to_char("You are unable to enchant this weapon.\n\r", ch);
        return;
    }

   quintessence = 5;

    if (!mage_pay_cost(ch,quintessence,MANA_ENCHANT,0)
        || !mage_roll(ch,3,&power)) return;

    if (power <= 0)
    {
        send_to_char("You failed to cast Enchant.\n\r", ch);
        return;
    }

    SET_BIT(obj->quest, QUEST_AGGRAVATED);
    send_to_char("You begin to channel quintessence and enchant the weapon.\n\r", ch);

}

//Prime 5: Paradox Ward
void do_paradox_ward(CHAR_DATA *ch, char * argument)
{
    char arg[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
    long power, quintessence;

    one_argument( argument, arg);

    if (!IS_MAGE(ch))
    {
    send_to_char("Huh?\n\r", ch);
    return;
    }

    if (ch->spheres[MPRI] < 5)
    {
	send_to_char("You don't have enough knoweldge of the prime sphere to use paradox ward.\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
    {
	send_to_char("How much quintessense do you wish to put into Paradox Ward?\n\r", ch);
	return;
    }

    if (!mage_quintessence(ch, arg, 1, &quintessence))
        return;

    if (quintessence < 1)
    {
	send_to_char("You must infuse your paradox ward with quintessence.\n\r", ch);
	return;
    }

    if (!mage_pay_cost(ch, quintessence, MANA_PARADOX_WARD, MANA_QUINT))
        return;

    power = spellpower(ch, 5); //Paradox ward uses lvl 5 prime

    if (power <= 0)
    {
	send_to_char("You failed to cast Paradox Ward.\n\r", ch);
	return;
    }

    power += quintessence;

    ch->paradox_ward = power;

    sprintf( buf1, "$n creates a glowing shield around $mself." );
    sprintf( buf2, "You create a ward against paradox around yourself.");

    act( buf1, ch, NULL, ch, TO_NOTVICT );
    act( buf2, ch, NULL, ch, TO_CHAR );

    WAIT_STATE(ch, 12);
}

void do_arete(CHAR_DATA *ch, char * argument)
{
    char buf[MAX_STRING_LENGTH];
    long cost;

    if (!IS_MAGE(ch))
    {
	stc("Huh?\n\r", ch);
	return;
    }

    if (ch->arete >= 10)
    {
	send_to_char("You already have the maximum Arete.\n\r", ch);
	return;
    }

    cost = (ch->arete +1) * 500000;

    if (ch->exp < cost)
    {
	sprintf(buf, "It costs %liexp to raise your arete.\r\n", cost);
	send_to_char(buf, ch);
	return;
    }

    send_to_char("Your Arete has increased!\r\n", ch);
    ch->arete++;
    ch->exp -= cost;
}

void backlash(CHAR_DATA *ch)
{
    long dam;
    char buf[MAX_STRING_LENGTH];

    dam = 0;

    ch->paradox -= ch->paradox_ward;
    ch->paradox_ward = 0;

    while(ch->paradox > 0)
    {
	dam += number_range(250, 750);
	ch->paradox--;
    }

    if (!IS_FLAG(ch, FLAG_PK)) dam /= 2; //making paradox not as bad for non-pks
	
    sprintf(buf, "You have been burned by the spirits of paradox! [#r%li#n]\n\r", dam);
    stc(buf, ch);
    act("$n has been burned by the spirits of paradox!" , ch, ch, NULL, TO_NOTVICT);

    hurt_person(ch, ch, dam);

    update_pos(ch);

	//add flaws, quiet, and all the really nasty stuff in a bit

}

void add_paradox(CHAR_DATA *ch, long highest_sphere, bool botch)
{
    CHAR_DATA *ich, *fch;
    long paradox;
    bool vulgar, witness;

    vulgar = FALSE;
    witness = FALSE;

    paradox = 0;

    if (ch->fighting != NULL)
	vulgar = TRUE;

    for ( ich = ch->in_room->people; ich != NULL; ich = fch )
    {
	fch = ich->next_in_room;

	if (IS_NPC(ich))
 	    witness = TRUE;
	else if (ich->level <= LEV_SLE)
	    witness = TRUE;

    }

    //add checks for strong effects and for global damage stuffs

    if (vulgar && !witness && !botch)
	paradox = highest_sphere;

    else if (vulgar && witness && !botch)
	paradox = highest_sphere + 1;

    else if (!vulgar && botch)
	paradox = highest_sphere;

    else if (vulgar && !witness && botch)
	paradox = highest_sphere +1;

    else if (vulgar && witness && botch)
	paradox = 2 * highest_sphere + 2;

   if (!IS_AFFECTED(ch, AFF_DEEPUMBRA))    ch->paradox += paradox;

    if ((paradox > 5 || botch || ch->paradox > 40) && !IS_AFFECTED(ch, AFF_DEEPUMBRA))
	backlash(ch);
}


long spellpower(CHAR_DATA *ch, long highest_sphere)
{

    long rolls, difficulty, success, loop, die;
    bool botch, possible_botch;

    botch = TRUE;
    possible_botch = FALSE;

    success = 0;
    rolls = ch->arete;
    difficulty = 3 + highest_sphere; //will add vulgar later

    for( loop = 0; loop < rolls; loop++)
    {
	die = number_range(1,10);
	
	if (die >= difficulty)
	{
	    success++;
	    botch = FALSE;
	}
		
	if (die == 1)
	{
	    success--;
	    possible_botch = TRUE;
	}
    }

    if (!possible_botch)
	botch = FALSE;

    if(botch)
	send_to_char("#yBotch!!!\n\r", ch);

    add_paradox(ch, highest_sphere, botch);
	
    if (success < 0)
	success = 0;

    if (success > 0) success++; //counteracting the cost for correspondence 1

    if (IS_AFFECTED(ch, AFF_DEEPUMBRA)) success *=2;

    return success;

}
//Life 2: healing
void do_mage_heal(CHAR_DATA *ch, char*argument)
{

    char arg [MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    long healing, power, loop, quint, correspondence;

    if (!mage_numeric_input(ch,argument,2)) return;
    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg2);
    healing = 0;
    quint = 0;

    if (!IS_MAGE(ch))
    {
	stc("Huh?\n\r", ch);
	return;
    }

    if (ch->spheres[MLIF] < 2)
    {
	stc("You do not have enough knowledge of the life sphere to use healing.\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
	victim = ch;

    else if ((victim = get_char_world(ch, arg)) == NULL)
    {
	stc("They aren't here.\n\r", ch);
	return;
    }

    correspondence = correspondence_level(ch, victim);

    if (correspondence > ch->spheres[MCOR])
    {
        stc("You don't not have enough knowledge of correspondence to target them.\n\r", ch);
        return;
    }

    if (victim == ch && ch->hit <= 0)
    {
	stc("You are too hurt for that.\n\r", ch);
	return;
    }

    if (!mage_quintessence(ch,arg2,0,&quint)) return;
    if (victim->hit>=victim->max_hit)
    {
        stc("They are already fully healed.\n\r",ch);
        return;
    }
    if (!mage_target_ready(ch,victim,TRUE) || !mage_pay_cost(ch,quint,
        MANA_MAGE_HEAL+correspondence*correspondence*100,MANA_QUINT)) return;
    if (!mage_roll(ch,2,&power)) return;
    power=UMAX(0,power-correspondence+quint);

    for (loop = 0; loop < power; loop++)
	healing += number_range(600, 1200);

    sprintf( buf1, "$n heals $N." );
    sprintf( buf2, "You heal $N.[#L%li#n]", healing );
    sprintf( buf3, "$n has healed you.[#L%li#n]", healing );

    act( buf1, ch, NULL, victim, TO_NOTVICT );
    act( buf2, ch, NULL, victim, TO_CHAR );
    act( buf3, ch, NULL, victim, TO_VICT );

    victim->hit += healing;

    if (victim->hit > victim->max_hit)
	victim->hit = victim->max_hit;
    update_pos(victim);

    WAIT_STATE(ch, 8);
}

//Life 3: Better Body
void do_better_body(CHAR_DATA *ch, char *argument)
{

    CHAR_DATA* victim;
    char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH], arg3[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
    long quint, power, correspondence;

    if (!mage_numeric_input(ch,argument,3)) return;
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);

   quint = 0;

    if (!IS_MAGE(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->spheres[MLIF] < 3)
    {
        send_to_char("You do not have enough knowledge of the life sphere to use better body.\n\r", ch);
        return;
    }

    if (str_cmp(arg2,"str") && str_cmp(arg2,"dex") && str_cmp(arg2,"con")
        && str_cmp(arg2,"int") && str_cmp(arg2,"wis"))
    {
        stc("Use betterbody <target> <str|dex|con|int|wis> [quintessence].\n\r",ch);
        return;
    }
    if (!mage_quintessence(ch,arg3,0,&quint)) return;

   // if (victim != ch) power -= 1; //costs 1 success to target someone else

    if (( victim = get_char_world(ch, arg1)) == NULL)
    {
        stc("They aren't here.\n\r", ch);
        return;
    }

    correspondence = correspondence_level(ch, victim);

    if (correspondence > ch->spheres[MCOR])
    {
        send_to_char("You don't not have enough knowledge of correspondence to target them.\n\r", ch);
        return;
    }

	if (IS_XTRA(victim, XTRA_BETTER_STR) || IS_XTRA(victim, XTRA_BETTER_DEX) ||IS_XTRA(victim, XTRA_BETTER_CON) ||IS_XTRA(victim, XTRA_BETTER_INT) ||IS_XTRA(victim, XTRA_BETTER_WIS))
	{
	    send_to_char("They are already affected by better body.\n\r", ch);
	    return;
	}
    if (!mage_target_ready(ch,victim,TRUE) || !mage_pay_cost(ch,quint,
        MANA_BETTER_BODY+correspondence*correspondence*100,MANA_QUINT)) return;
    if (!mage_roll(ch,3,&power)) return;
    power=UMAX(1,power-correspondence+quint+2);

	if (!str_cmp(arg2, "str")) SET_BIT(victim->xtra, XTRA_BETTER_STR);
	else if(!str_cmp(arg2, "dex")) SET_BIT(victim->xtra, XTRA_BETTER_DEX);
	else if(!str_cmp(arg2, "con")) SET_BIT(victim->xtra, XTRA_BETTER_CON);
	else if(!str_cmp(arg2, "int")) SET_BIT(victim->xtra, XTRA_BETTER_INT);
	else if(!str_cmp(arg2, "wis")) SET_BIT(victim->xtra, XTRA_BETTER_WIS);
	else
	{
	    send_to_char("You must select from str, dex, con, int, or wis.\n\r", ch);
	    return;
	}

	victim->better_body_timer = power * 50;

	sprintf(buf, "You use better body on %s.\n\r", victim->name); stc(buf, ch);
	sprintf(buf, "%s uses better body on you.\n\r", ch->name); stc(buf, victim);

}

//Life 5: Polymorph other
void do_polymorph_other(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA * victim;
    OBJ_DATA* obj;
    long power, quint, correspondence;

    if (!mage_numeric_input(ch,argument,2)) return;
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    quint = 0;

    if (!IS_MAGE(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->spheres[MLIF] < 5)
    {
        send_to_char("You do not have enough knowledge of the life sphere to use polymorph other.\n\r", ch);
        return;
    }

    if (!mage_quintessence(ch,arg2,0,&quint)) return;

   // if (victim != ch) power -= 1; //costs 1 success to target someone else

    if ((victim = mage_attack_target(ch,arg1)) == NULL)
    {
        stc("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC(victim))
    {
	stc("Not on NPCs.\n\r", ch);
	return;
    }

    if (is_affected(victim, gsn_lycaon) || IS_POLYAFF(victim,POLY_PIG))
    {
        stc("Lift Lycaon's curse before applying another form.\n\r", ch);
        return;
    }

    correspondence = correspondence_level(ch, victim);

    if (correspondence > ch->spheres[MCOR])
    {
        send_to_char("You don't not have enough knowledge of correspondence to target them.\n\r", ch);
        return;
    }

    if (!mage_target_ready(ch,victim,FALSE) || !mage_pay_cost(ch,quint,
        MANA_POLYMORPH_OTHER+correspondence*correspondence*100,MANA_QUINT)) return;
    if (!mage_roll(ch,5,&power)) return;
    power+=quint;
    if (power<=0) { stc("You failed to cast polymorph other.\n\r",ch); return; }
    power-=correspondence;

    do_stance(victim, ""); //can't stance while in polymorph

    if ((obj = get_eq_char(victim,WEAR_HOLD)) != NULL)
    {
        obj_from_char( obj );
        obj_to_char( obj, victim);
    }

    if ((obj = get_eq_char(victim,WEAR_WEAPON)) != NULL)
    {
        obj_from_char( obj );
	obj_to_char( obj, victim);
    }

    //First remove the target from any current polymorph affect
    if (IS_WEREWOLF(victim) && IS_POLYAFF(victim,POLY_MASK)) do_doppleganger(victim,"self");
    if (IS_POLYAFF(victim,POLY_MASK) ) do_mask(victim,"self");
    if (IS_POLYAFF(victim,POLY_TRUE) ) do_truedisguise(victim,"self");
    if (IS_CRAFT(victim, CRAFT_MALLEABLE)) do_malleable(victim, "self");
    if (IS_VAMPAFF(victim,VAM_CHANGED) ) do_change(victim,"human");
    if (IS_POLYAFF(victim,POLY_SERPENT) ) do_serpent(victim,"");
    if (IS_POLYAFF(victim,POLY_SHADOW) ) do_shadowbody(victim,"");

    //remove from chrinos
    if(IS_WEREWOLF(victim) && IS_SET(victim->act, PLR_WOLFMAN))
    {
    victim->pcdata->wolf = 0;
    do_unwerewolf(victim,"");
    }

    //remove from zulo
    if (IS_VAMPIRE(victim) && IS_POLYAFF(victim ,POLY_ZULO))
    {

            free_string(victim->morph);
            victim->morph = str_dup( "" );
            REMOVE_BIT(victim->affected_by, AFF_POLYMORPH);
            REMOVE_BIT(victim->vampaff, VAM_DISGUISED);
            REMOVE_BIT(victim->vampaff, VAM_CLAWS);

        REMOVE_BIT(victim->vampaff, VAM_NIGHTSIGHT);
        REMOVE_BIT(victim->polyaff, POLY_ZULO);
    }

    //remove from dragon form
    if (IS_POLYAFF(victim,POLY_DRAGONFORM))
    {
	if (IS_DRAGAFF(victim, DRAG_HORNSOUT)) do_horns(victim, "");
	if (IS_DRAGAFF(victim, DRAG_WINGSOUT)) do_wings(victim, "");
        if (IS_VAMPAFF(victim,VAM_CLAWS)) do_claws(victim,"");
        if (IS_VAMPAFF(victim,VAM_NIGHTSIGHT)) do_nightsight(victim,"");
        if (IS_VAMPAFF(victim,VAM_FANGS)) do_fangs(victim,"");
        free_string(victim->morph);
        victim->morph = str_dup("");
        REMOVE_BIT(victim->affected_by, AFF_POLYMORPH);
        REMOVE_BIT(victim->vampaff, VAM_DISGUISED);
        REMOVE_BIT(victim->polyaff, POLY_DRAGONFORM);
        victim->pcdata->atm = 0;
    }

    if (IS_MONK(victim) && IS_POLYAFF(victim, POLY_CHIMERA))
    {
        free_string(victim->morph);
        victim->morph = str_dup("");
        REMOVE_BIT(victim->affected_by, AFF_POLYMORPH);
        REMOVE_BIT(victim->vampaff, VAM_DISGUISED);
        REMOVE_BIT(victim->polyaff, POLY_CHIMERA);
    }

    if (power < 0) power = 0;

    power += 2; //countering local correspondence cost

    sprintf(buf, "You polymorph %s into a pig.\n\r", victim->name); stc(buf, ch);

    //morph into a pig
    send_to_char("Your form shrinks and shifts into the shape of a pig.\n\r",victim);
    act("$n's body moves and shifts into the shape of a pig.",victim,NULL,NULL,TO_ROOM);
    SET_BIT(victim->affected_by, AFF_POLYMORPH);
    SET_BIT(victim->polyaff, POLY_PIG);
    sprintf(buf, "#PA small pink pig[#p%s#P]#n", victim->name);
    free_string(victim->morph);
    victim->morph = str_dup(buf);
    victim->polymorph_timer = power*2;
    update_style(victim);
    return;
}

//Forces 3: Friction Curse
void do_friction_curse(CHAR_DATA *ch, char *argument)
{

    char arg[MAX_STRING_LENGTH], arg2[MAX_INPUT_LENGTH], extra[MAX_INPUT_LENGTH], buf1[MAX_STRING_LENGTH],buf2[MAX_STRING_LENGTH],buf3[MAX_STRING_LENGTH];
    CHAR_DATA * victim;
    AFFECT_DATA af;
    long power, sn, correspondence, quint;
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *to_room;
    bool protected;

    if (!mage_numeric_input(ch,argument,2)) return;
    argument=one_argument(argument,arg);
    argument=one_argument(argument,arg2);
    one_argument(argument,extra);
    if (extra[0]) { stc("Use friction <target> [quintessence].\n\r",ch); return; }


    quint = 0;

    if (!IS_MAGE(ch))
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (ch->spheres[MFOR] < 3)
    {
	send_to_char("You don't have enough knoweldge of the forces sphere to use friction curse.\n\r", ch);
	return;
    }

    if ((victim = mage_attack_target(ch,arg)) == NULL)
    {
    	send_to_char("They aren't here.\n\r", ch);
    	return;
    }

    if (!victim || !victim->in_room) return;
    correspondence = correspondence_level(ch, victim);

    if (correspondence > ch->spheres[MCOR])
    {
        send_to_char("You don't not have enough knowledge of correspondence to target them.\n\r", ch);
        return;
    }

    if (!mage_quintessence(ch,arg2,0,&quint)) return;
    sn=skill_lookup("friction curse");
    if (sn<0 || is_affected(victim,sn)) {
        stc("Friction Curse is unavailable or already affects that target.\n\r",ch); return;
    }
    location=ch->in_room; to_room=victim->in_room;
    if (!to_room || IS_BUI(victim) || (victim!=ch && is_same_group(ch,victim))
        || (victim!=ch && !IS_NPC(victim) && (!CAN_PK(ch) || !CAN_PK(victim)))
        || IS_SET(location->room_flags|to_room->room_flags,
            ROOM_SAFE|ROOM_PRIVATE|ROOM_SOLITARY|ROOM_NO_TELEPORT)) return;
    /* Native is_safe needs colocation. Always return before any exit or cost. */
    protected=FALSE;
    if (victim!=ch) {
        if (location!=to_room) { char_from_room(ch); char_to_room(ch,to_room); }
        protected=is_safe(ch,victim);
        if (ch->in_room!=location) { char_from_room(ch); char_to_room(ch,location); }
    }
    if (protected || !mage_pay_cost(ch,quint,
        MANA_FRICTION_CURSE+correspondence*correspondence*100,MANA_QUINT)) return;
    power=spellpower(ch,3)-correspondence+quint;

    if (power <= 0)
    {
	send_to_char("You failed to cast Friction Curse.\n\r", ch);
	return;
    }

    if (!IS_NPC(victim) && victim->pcdata->atm >= number_percent())
    {
        stc("They resisted your spell.\n\r", ch);
        return;
    }

    if ((sn = skill_lookup("Friction Curse")) < 0)
    {
	stc("Buggyness! So umm, time to go yell at Dude... ;)\n\r",ch);
	return;
    }

    if ( is_affected (victim, sn ) )
    {
	 stc("They are already affected by friction curse!\n\r", ch);
	 return;
    }

    memset(&af,0,sizeof(af));
    af.type = sn;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.duration = power * 10;
    af.bitvector = 0;
        
    affect_to_char( victim, &af );

    sprintf(buf1, "$n places a curse on $N.");
    sprintf(buf2, "You put a friction curse on $N.");
    sprintf(buf3, "$n has placed a curse on you.");

    act(buf1, ch, NULL, victim, TO_NOTVICT);
    act(buf2, ch, NULL, victim, TO_CHAR);
    act(buf3, ch, NULL, victim, TO_VICT);

    WAIT_STATE(ch, 10);
}

void do_electrical_chaos( CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH],  buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],buf3[MAX_STRING_LENGTH];
    CHAR_DATA * victim;
    long dam, power, quint, correspondence;

    if (!mage_numeric_input(ch,argument,2)) return;
    dam = 0;
    quint = 0;
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (!IS_MAGE(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->spheres[MFOR] < 2)
    {
        send_to_char("You do not have enough knowledge of the forces sphere to use electrical chaos.\n\r", ch);
        return;
    }

    if (!mage_quintessence(ch,arg2,0,&quint)) return;

   // if (victim != ch) power -= 1; //costs 1 success to target someone else

    if ((victim = mage_attack_target(ch,arg1)) == NULL)
    {
        stc("They aren't here.\n\r", ch);
	return;
    }

    if (ch == victim) return;

    correspondence = correspondence_level(ch, victim);

    if (correspondence > ch->spheres[MCOR])
    {
        send_to_char("You don't not have enough knowledge of correspondence to target them.\n\r", ch);
        return;
    }

    if ((!IS_NPC(victim) && correspondence == 5) || (IS_NPC(victim) && victim->level > 800 && correspondence == 5))
    {
	stc("You may not target them globally.\n\r", ch);
	return;
    }
    if (!mage_target_ready(ch,victim,FALSE) || !mage_pay_cost(ch,quint,
        MANA_ELECTRICAL_CHAOS+correspondence*correspondence*100,MANA_QUINT)) return;
    if (!mage_roll(ch,2,&power)) return;
    power+=quint+1-correspondence;
    if (power<=0)
    {
        stc("The electrical current fizzles out.\n\r",ch);
        WAIT_STATE(ch,8);
        return;
    }

    if (!IS_NPC(victim) && victim->pcdata->atm >= number_percent())
    {
        stc("They resisted your spell.\n\r", ch);
        return;
    }

    dam = number_range(1000 * power, 1200 * power); //was 900-1200, trying to to balance for weapon styles

    if (ch->fighting != victim && ch != victim && victim->fighting == NULL)  set_fighting(victim, ch);

    if (IS_NPC(victim)) dam *=2;

    sprintf( buf1, "$n's unleashes an electrical current into $N." );
    sprintf( buf2, "You unleash a high voltage electrical current into $N.[#g%li#n]", dam );
    sprintf( buf3, "$n has unleashed a high voltage electrical current into you.[#r%li#n]", dam );

    act( buf1, ch, NULL, victim, TO_NOTVICT );
    act( buf2, ch, NULL, victim, TO_CHAR );
    act( buf3, ch, NULL, victim, TO_VICT );

    hurt_person(ch, victim, dam);

    WAIT_STATE(ch, 8);    
}


//Forces 5: Inferno
void do_inferno( CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], 
    buf3[MAX_STRING_LENGTH];
    CHAR_DATA * victim, * vch_next;
    long dam, power, quint, victims;
    bool botch;

    if (!mage_numeric_input(ch,argument,1)) return;
    dam = 0;
    quint = 0;
    argument = one_argument(argument, arg1);

    if (arg1[0] != '\0' && !mage_quintessence(ch,arg1,0,&quint)) return;

    if (!IS_MAGE(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->spheres[MFOR] < 5)
    {
        send_to_char("You do not have enough knowledge of the forces sphere to use inferno.\n\r", ch);
        return;
    }

    victims = 0;

    for ( victim = ch->in_room->people; victim != NULL; victim = vch_next )
    {
	vch_next = victim->next_in_room;
        if (victim!=ch && !is_same_group(ch,victim)
            && mage_target_ready(ch,victim,FALSE)) victims++;
    }
    if (!victims) { stc("There are no legal targets for Inferno here.\n\r",ch); return; }
    if (!mage_pay_cost(ch,quint,MANA_INFERNO,MANA_QUINT)
        || !mage_roll(ch,5,&power)) return;
    botch=(power<=0);
    power+=quint+3;
    power -= victims/2;
    if (power<=0)
    {
        if (botch) { stc("Your Inferno fizzles out.\n\r",ch); WAIT_STATE(ch,12); return; }
        power=1;
    }

    dam = number_range(900 * power, 1300 * power);

    for ( victim = ch->in_room->people; victim != NULL; victim = vch_next )
    {
        vch_next = victim->next_in_room;
	if (is_same_group(ch, victim)) continue;
	if (!IS_NPC(victim) && !CAN_PK(victim)) continue;
     	if (victim != ch && mage_target_ready(ch,victim,FALSE))
	{
            if (victim->fighting != ch && ch != victim && victim->fighting == NULL)  set_fighting(victim, ch);

	     if (!IS_NPC(victim) && victim->pcdata->atm >= number_percent())
	    {
    	        stc("They resisted your spell.\n\r", ch);
    	        continue;
    	    }


	    if (IS_NPC(victim)) dam *=2;

	    sprintf( buf1, "$n's unleashes a burning inferno upon $N." );
	    sprintf( buf2, "You unleash a burning inferno upon $N.[#g%li#n]", dam );
	    sprintf( buf3, "$n has unleashed a burning inferno upon you.[#r%li#n]", dam );

	    act( buf1, ch, NULL, victim, TO_NOTVICT );
  	    act( buf2, ch, NULL, victim, TO_CHAR );
	    act( buf3, ch, NULL, victim, TO_VICT );

  	    hurt_person(ch, victim, dam);


	    if (IS_NPC(victim)) dam /= 2; //so the damage doesn't keep multiplying...

	    if (!IS_AFFECTED(victim, AFF_FLAMING))  	SET_BIT(victim->affected_by, AFF_FLAMING);
	}
    }

    WAIT_STATE(ch, 12);
}

//Time 3: Contingency
void do_contingency( CHAR_DATA* ch, char * argument)
{
    char key[MAX_INPUT_LENGTH];
    const MAGE_TIME_SPELL *spell;
    long cost;
    if (!ch || !ch->pcdata || IS_NPC(ch) || !argument
        || strlen(argument)>=sizeof(key) || !IS_MAGE(ch)
        || !mage_numeric_input(ch,"",0)) return;
    argument=one_argument(argument,key);
    if (*argument) { stc("Use contingency [spell|clear].\n\r",ch); return; }
    if (ch->spheres[MTIM]<3) { stc("Contingency requires Time 3.\n\r",ch); return; }
    if (!str_cmp(key,"clear"))
    {
        free_string(ch->contingency); ch->contingency=str_dup("");
        stc("Your contingency is cleared.\n\r",ch); return;
    }
    if (!key[0] && !ch->fighting)
    {
        stc("Prepared contingency: ",ch);
        stc(ch->contingency && ch->contingency[0] ? ch->contingency : "none",ch);
        stc(".\n\r",ch); return;
    }
    spell=mage_time_find(key[0] ? key : ch->contingency,1);
    if (!spell || ch->spheres[spell->sphere]<spell->level)
    {
        stc("Choose a mastered spell: brittlebones, mutateephemera, electricalchaos, frictioncurse, inferno, polymorphother or alterweight.\n\r",ch);
        return;
    }
    if (key[0])
    {
        if (ch->contingency && !str_cmp(ch->contingency,spell->name)) return;
        if (!mage_pay_cost(ch,0,MANA_CONTINGENCY,0)) return;
        free_string(ch->contingency); ch->contingency=str_dup(spell->name);
        stc("Your contingency has been set.\n\r",ch); return;
    }
    if (!mage_time_ready(ch,spell,"",&cost) || ch->mana<MANA_CONTINGENCY+cost) return;
    if (!mage_pay_cost(ch,0,MANA_CONTINGENCY,0)) return;
    /* Disarm before the child can re-enter set_fighting. */
    free_string(ch->contingency); ch->contingency=str_dup("");
    spell->cast(ch,"");
}

//Time 5: Double cast
void do_double_cast( CHAR_DATA* ch, char * argument)
{
    char key[MAX_INPUT_LENGTH];
    const MAGE_TIME_SPELL *spell;
    ROOM_INDEX_DATA *origin;
    long power,cost;
    if (!ch || !ch->pcdata || IS_NPC(ch) || !argument
        || strlen(argument)>=sizeof(key) || !IS_MAGE(ch)
        || !mage_numeric_input(ch,"",0)) return;
    argument=one_argument(argument,key);
    if (ch->spheres[MTIM]<5) { stc("Doublecast requires Time 5.\n\r",ch); return; }
    if (ch->doublecast_timer>0) { stc("Your Doublecast has not recovered.\n\r",ch); return; }
    spell=mage_time_find(key,2);
    if (!spell)
    {
        stc("Choose inferno, mutateephemera, electricalchaos, mageheal or alterweight.\n\r",ch);
        return;
    }
    if (!mage_time_ready(ch,spell,argument,&cost)) return;
    if (ch->mana<MANA_DOUBLE_CAST+cost)
    {
        stc("You need enough mana for Doublecast and its first spell attempt.\n\r",ch);
        return;
    }
    if (!mage_pay_cost(ch,0,MANA_DOUBLE_CAST,0) || !mage_roll(ch,5,&power)) return;
    ch->doublecast_timer=UMAX(100,600-power*100);
    origin=ch->in_room;
    spell->cast(ch,argument);
    if (ch->in_room==origin && ch->hit>0 && ch->position>=POS_RESTING && !IS_BUI(ch))
        spell->cast(ch,argument);
}


void do_spheres( CHAR_DATA *ch, char *argument )
 {
  char buf[MAX_STRING_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  long max,disc,cost;
  int menu_sphere;
  cost = 0;
  max = 4;
  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );
  if (!IS_MAGE(ch))
  {
    stc("Huh?\n\r", ch);
    return;
  }
  if ( arg1[0] == '\0' )
  {
    class_training_header(ch,"MAGE SPHERES AND ARETE");
    for(menu_sphere=0;menu_sphere<9;++menu_sphere) {
        long menu_cap=ch->pcdata->cland[0]==menu_sphere
            || ch->pcdata->cland[1]==menu_sphere
            || ch->pcdata->cland[2]==menu_sphere ? 5 : 4;
        class_training_row(ch,sphere_name[menu_sphere],ch->spheres[menu_sphere],menu_cap,
            class_training_cost(ch->spheres[menu_sphere],menu_cap,10,FALSE),
            "primal",ch->primal,NULL);
    }
    class_training_row(ch,"Arete",ch->arete,10,
        class_training_cost(ch->arete,10,500000,FALSE),"XP",ch->exp,NULL);
    stc("View: sphere <name> | Train: sphere <name> learn | Arete: arete\n\r"
        "Use magepowers for rank-unlocked commands and targeting.\n\r",ch);
        return;
  }
  if ( arg2[0] == '\0' )
  {
    int i;
    for(i=0;i<9;++i)
        if (!str_prefix(arg1,sphere_name[i])) {
            if (ch->spheres[i]<=0)
                stc("You have no knowledge of that sphere.\n\r",ch);
            else do_magepowers(ch,"");
            return;
        }
    stc("Unknown sphere. Use sphere to list the nine tracks.\n\r",ch);
    return;
  }
  if ( !str_cmp( arg2, "learn" ) )
  {
         if ( !str_prefix( arg1, "correspondence" ) )
      disc = MCOR;
    else if ( !str_prefix( arg1, "life" ) )
      disc = MLIF;
    else if ( !str_prefix( arg1, "prime" ) )
      disc = MPRI;
    else if ( !str_prefix( arg1, "entropy" ) )
      disc = MENT;
    else if ( !str_prefix( arg1, "mind" ) )
      disc = MMIN;
    else if ( !str_prefix( arg1, "spirit" ) )
      disc = MSPI;
    else if ( !str_prefix( arg1, "forces" ) )
      disc = MFOR;
    else if ( !str_prefix( arg1, "matter" ) )
      disc = MMAT;
    else if ( !str_prefix( arg1, "time" ) )
      disc = MTIM;
    else
    {
  stc("You must select from the following spheres:\n\r Correspondence, Life, Prime, Entropy, Mind, Spirit, Forces, Matter, Time.\n\r",ch);
      return;
    }
  }
  else
  {
    do_spheres(ch,"");
    return;
  }
  if (ch->pcdata->cland[0] == disc || ch->pcdata->cland[1] == disc || ch->pcdata->cland[2] == disc) max = 5;
 
  cost = ( ( ch->spheres[disc] + 1 ) * 10 );
  if ( ch->spheres[disc] >= max )
  {
    sprintf(buf, "You have already learned all levels of the %s sphere.\n\r",sphere_name[disc] );
    stc(buf,ch);
    return;
  }
  if ( ch->primal < cost )
  {
    sprintf(buf, "You need %li primal to obtain a new level of understanding of the %s sphere.\n\r", cost, sphere_name[disc] );
    stc(buf,ch);
    return;
  }
  ch->primal -= cost;
  ch->spheres[disc]++;
  sprintf(buf, "Your knowledge of the %s sphere increases.\n\r",sphere_name[disc] );
  stc(buf,ch);
  return;
}

void do_clansphere(CHAR_DATA * ch, char * argument)
{

    if( !IS_MAGE(ch))
    {
	stc("Huh?\n\r", ch);
	return;
    }

    stc("Your clan spheres are:\n\r", ch);

    stc("Clan:     ", ch);
	if (ch->pcdata->cland[0] == SPHERE_CORRESPONDENCE) 	stc(" Correspondence.\n\r", ch);
	if (ch->pcdata->cland[0] == SPHERE_ENTROPY) 		stc(" Entropy.\n\r", ch);
	if (ch->pcdata->cland[0] == SPHERE_FORCES) 		stc(" Forces.\n\r", ch);
	if (ch->pcdata->cland[0] == SPHERE_LIFE) 		stc(" Life.\n\r", ch);
	if (ch->pcdata->cland[0] == SPHERE_MIND)		stc(" Mind.\n\r", ch);
	if (ch->pcdata->cland[0] == SPHERE_MATTER) 		stc(" Matter.\n\r", ch);
	if (ch->pcdata->cland[0] == SPHERE_SPIRIT)		stc(" Spirit.\n\r", ch);
	if (ch->pcdata->cland[0] == SPHERE_PRIME)		stc(" Prime.\n\r", ch);
	if (ch->pcdata->cland[0] == SPHERE_TIME) 		stc(" Time.\n\r", ch);

    stc("Avatar:   ", ch);
	if (ch->pcdata->cland[1] == SPHERE_CORRESPONDENCE)      stc(" Correspondence.\n\r", ch);
	if (ch->pcdata->cland[1] == SPHERE_ENTROPY)             stc(" Entropy.\n\r", ch);
	if (ch->pcdata->cland[1] == SPHERE_FORCES)              stc(" Forces.\n\r", ch);
	if (ch->pcdata->cland[1] == SPHERE_LIFE)                stc(" Life.\n\r", ch);
	if (ch->pcdata->cland[1] == SPHERE_MIND)                stc(" Mind.\n\r", ch);
	if (ch->pcdata->cland[1] == SPHERE_MATTER)              stc(" Matter.\n\r", ch);
	if (ch->pcdata->cland[1] == SPHERE_SPIRIT)              stc(" Spirit.\n\r", ch);
	if (ch->pcdata->cland[1] == SPHERE_PRIME)               stc(" Prime.\n\r", ch);
	if (ch->pcdata->cland[1] == SPHERE_TIME)                stc(" Time.\n\r", ch);

    stc("Personal: ", ch);
	if (ch->pcdata->cland[2] == SPHERE_CORRESPONDENCE)      stc(" Correspondence.\n\r", ch);
	if (ch->pcdata->cland[2] == SPHERE_ENTROPY)             stc(" Entropy.\n\r", ch);
	if (ch->pcdata->cland[2] == SPHERE_FORCES)              stc(" Forces.\n\r", ch);
	if (ch->pcdata->cland[2] == SPHERE_LIFE)                stc(" Life.\n\r", ch);
	if (ch->pcdata->cland[2] == SPHERE_MIND)                stc(" Mind.\n\r", ch);
	if (ch->pcdata->cland[2] == SPHERE_MATTER)              stc(" Matter.\n\r", ch);
	if (ch->pcdata->cland[2] == SPHERE_SPIRIT)              stc(" Spirit.\n\r", ch);
	if (ch->pcdata->cland[2] == SPHERE_PRIME)               stc(" Prime.\n\r", ch);
	if (ch->pcdata->cland[2] == SPHERE_TIME)                stc(" Time.\n\r", ch);


}

void do_magepowers( CHAR_DATA *ch, char * argument)
{
char buf[MSL];
if (!ch || IS_NPC(ch) || !ch->pcdata) return;
if (!IS_MAGE(ch)) { stc("Huh?\n\r",ch); return; }
class_menu_header(ch,"Mage Powers","Sphere","Known commands / targeting");

    if (ch->spheres[MCOR] > 0)
    {
strcpy(buf,"Correspondence: ");
if (ch->spheres[MCOR] >= 4) strcat(buf,"Rift, ");
if (ch->spheres[MCOR] == 1) strcat(buf,"Self Targeting.");
else if (ch->spheres[MCOR] == 2 || ch->spheres[MCOR] == 3 || ch->spheres[MCOR] == 4) strcat(buf,"Local Targeting.");
else if (ch->spheres[MCOR] == 5) strcat(buf,"Same Area Targeting.");
class_menu_commands(ch,buf);
    }

    if (ch->spheres[MENT] > 0)
    {
strcpy(buf,"Entropy: ");
if (ch->spheres[MENT] >= 2) strcat(buf,"beginnersluck");
if (ch->spheres[MENT] >= 3) strcat(buf,", brittlebones");
if (ch->spheres[MENT] >= 5) strcat(buf,", mutateephemera");
strcat(buf,"."); class_menu_commands(ch,buf);
    }

    if (ch->spheres[MFOR] > 0)
    {
        strcpy(buf,"Forces: ");
if (ch->spheres[MFOR] >= 1) strcat(buf,"nightsight");
        if (ch->spheres[MFOR] >= 2) strcat(buf,", electricalchaos");
        if (ch->spheres[MFOR] >= 3) strcat(buf,", friction");
        if (ch->spheres[MFOR] >= 5) strcat(buf,", inferno");
        strcat(buf,"."); class_menu_commands(ch,buf);
    }

    if (ch->spheres[MLIF] > 0)
    {
        strcpy(buf,"Life: ");
if (ch->spheres[MLIF] >= 1) strcat(buf,"readaura");
        if (ch->spheres[MLIF] >= 2) strcat(buf,", mageheal");
        if (ch->spheres[MLIF] >= 3) strcat(buf,", betterbody");
        if (ch->spheres[MLIF] >= 5) strcat(buf,", polymorphother");
        strcat(buf,"."); class_menu_commands(ch,buf);
    }

    if (ch->spheres[MMAT] > 0)
    {
        strcpy(buf,"Matter: ");
if (ch->spheres[MMAT] >= 1) strcat(buf,"analyze");
        if (ch->spheres[MMAT] >= 2) strcat(buf,", shatterwall");
        if (ch->spheres[MMAT] >= 3) strcat(buf,", sculpture");
        if (ch->spheres[MMAT] >= 5) strcat(buf,", alterweight");
        strcat(buf,"."); class_menu_commands(ch,buf);
    }

    if (ch->spheres[MMIN] > 0)
    {
        strcpy(buf,"Mind: ");
        if (ch->spheres[MMIN] >= 1) strcat(buf,"heighten");
        if (ch->spheres[MMIN] >= 2) strcat(buf,", scry");
        if (ch->spheres[MMIN] >= 3) strcat(buf,", shield");
        if (ch->spheres[MMIN] >= 5) strcat(buf,", mageinvis");
        strcat(buf,"."); class_menu_commands(ch,buf);
    }

    if (ch->spheres[MPRI] > 0)
    {
        strcpy(buf,"Prime: ");
if (ch->spheres[MPRI] >= 1) strcat(buf,"heartsblood");
        if (ch->spheres[MPRI] >= 2) strcat(buf,", holysmote");
        if (ch->spheres[MPRI] >= 3) strcat(buf,", mageenchant");
        if (ch->spheres[MPRI] >= 5) strcat(buf,", ward");
        strcat(buf,"."); class_menu_commands(ch,buf);
    }

    if (ch->spheres[MSPI] > 0)
    {
        strcpy(buf,"Spirit: ");
if (ch->spheres[MSPI] >= 1) strcat(buf,"spiritsense");
        if (ch->spheres[MSPI] >= 2) strcat(buf,", spiritcaress");
        if (ch->spheres[MSPI] >= 3) strcat(buf,", stepsideways");
        if (ch->spheres[MSPI] >= 5) strcat(buf,", breakdreamshell");
        strcat(buf,"."); class_menu_commands(ch,buf);
    }

    if (ch->spheres[MTIM] > 0)
    {
        strcpy(buf,"Time: ");
        if (ch->spheres[MTIM] >= 3) strcat(buf,"contingency");
        if (ch->spheres[MTIM] >= 5) strcat(buf,", doublecast");
        strcat(buf,"."); class_menu_commands(ch,buf);
    }

stc("Commands shown are rank-unlocked; other casting conditions still apply.\n\r",ch);

}


void do_select_spheres(CHAR_DATA* ch, char * argument)
{
    char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
    long clan, sphere;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);


    if(!IS_MAGE(ch))
    {
	stc("Huh?\n\r", ch);
	return;
    }

    if (str_cmp(arg1, "avatar") && str_cmp(arg1, "personal") && str_cmp(arg1, "clan"))
    {
	stc("You must select either your personal or avatar sphere.\n\r", ch);
	return;
    }

    if (!str_cmp(arg1, "clan")) clan = 0;	//Hollow Ones only
    else if (!str_cmp(arg1, "avatar")) clan = 1;
    else if (!str_cmp(arg1, "personal")) clan = 2;
    else
    {
	stc("Error: Invalid choice of clandisc.\n\r", ch);
	return;
    }

    if (ch->pcdata->cland[clan] != -1)
    {
format_buffer(buf, sizeof(buf), "You have already selected a %s clan sphere.\n\r", arg1);stc(buf, ch);
	return;
    }
 
    if (!str_cmp(arg2, "correspondence"))	 sphere = SPHERE_CORRESPONDENCE;
    else if (!str_cmp(arg2, "forces")) 		 sphere = SPHERE_FORCES;
    else if (!str_cmp(arg2, "entropy")) 	 sphere = SPHERE_ENTROPY;
    else if (!str_cmp(arg2, "matter")) 		 sphere = SPHERE_MATTER;
    else if (!str_cmp(arg2, "time")) 		 sphere = SPHERE_TIME;
    else if (!str_cmp(arg2, "life")) 		 sphere = SPHERE_LIFE;
    else if (!str_cmp(arg2, "mind")) 		 sphere = SPHERE_MIND;
    else if (!str_cmp(arg2, "spirit"))		 sphere = SPHERE_SPIRIT;
    else if (!str_cmp(arg2, "prime"))		 sphere = SPHERE_PRIME;
    else
    {
	stc("That is not a valid sphere.\n\r", ch);
	return;
    }

    if (ch->pcdata->cland[0] == sphere || ch->pcdata->cland[1] == sphere ||ch->pcdata->cland[2] == sphere)
    {
	stc("That is already one of your clan spheres.\n\r", ch);
	return;
    }

    ch->pcdata->cland[clan] = sphere;
    stc("You clan sphere has been set.\n\r", ch);

}

void do_magearmor( CHAR_DATA *ch, char *argument )
{
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;
    char arg[MAX_INPUT_LENGTH];
    long vnum = 0;

    argument = one_argument(argument, arg);

    if (!IS_MAGE(ch))
    {
        stc("Huh?\n\r", ch);
        return;
    }

    if (arg[0] == '\0')
    {
        stc("Please specify which piece of mage armor you wish to make:\n\r", ch);
        stc("  Robes, Hood, Talisman.\n\r", ch);
        return;
    }

    if (ch->primal < 100 || ch->pcdata->quest < 400)
    {
        stc("It costs 100 primal and 400 quest points to create mage armor.\n\r", ch);
        return;
    }

    if (!str_cmp(arg,"robes")) vnum = 29660;
    else if (!str_cmp(arg,"hood")) vnum = 29661;
    else if (!str_cmp(arg,"talisman")) vnum = 29662;
    else
    {
        do_magearmor(ch,"");
        return;
    }

    if ((pObjIndex = get_obj_index(vnum)) == NULL)
    {
        stc("Missing object, please inform Akuma.\n\r", ch);
        return;
    }

    obj = create_object(pObjIndex, 50);
    obj_to_char(obj, ch);
    ch->primal -= 100;
    ch->pcdata->quest -= 400;
    free_string(obj->questmaker);
    obj->questmaker = str_dup(ch->name);
    free_string(obj->questowner);
    obj->questowner = str_dup(ch->name);

    act("$p appears in your hands.",ch,obj,NULL,TO_CHAR);
    act("$p appears in $n's hands.",ch,obj,NULL,TO_ROOM);
    return;
}

