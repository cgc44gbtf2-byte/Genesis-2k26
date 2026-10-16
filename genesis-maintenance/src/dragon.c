#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"

void do_dragonform( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char c1[MAX_STRING_LENGTH];
    char c2[MAX_STRING_LENGTH];
    char age[MAX_STRING_LENGTH];

    argument = one_argument( argument, arg );

    if (IS_NPC(ch)) return;
    if (!IS_DRAGON(ch))
    {
        send_to_char("Huh?\n\r",ch);
        return;
    }
    if (IS_AFFECTED(ch,AFF_POLYMORPH) && !IS_POLYAFF(ch,POLY_DRAGONFORM))
    {
	send_to_char( "You cannot change from this form.\n\r", ch );
	return;
    }
    if (IS_AFFECTED(ch,AFF_POLYMORPH) && !IS_VAMPAFF(ch,VAM_DISGUISED))
    {
        send_to_char( "Not while polymorphed.\n\r", ch );
        return;
    }
    if (IS_POLYAFF(ch,POLY_DRAGONFORM))
    {
	if (IS_DRAGAFF(ch, DRAG_HORNSOUT)) do_horns(ch,"");
	if (IS_DRAGAFF(ch, DRAG_WINGSOUT)) do_wings(ch,"");
	if (IS_VAMPAFF(ch, VAM_CLAWS)) do_claws(ch,"");
	if (IS_VAMPAFF(ch, VAM_NIGHTSIGHT)) do_nightsight(ch,"");
	if (IS_VAMPAFF(ch, VAM_FANGS)) do_fangs(ch,"");
	free_string(ch->morph);
	ch->morph = str_dup("");
	REMOVE_BIT(ch->affected_by, AFF_POLYMORPH);
	REMOVE_BIT(ch->vampaff, VAM_DISGUISED);
	REMOVE_BIT(ch->polyaff, POLY_DRAGONFORM);
	ch->pcdata->atm = 0;
	send_to_char("Your body shrinks back to its human form.\n\r",ch);
	act("A large dragon's body shrinks into the form of $n.",ch,NULL,NULL,TO_ROOM);
	update_style(ch);
	return;
    }
    switch (ch->pcdata->dragcol)
    {
	case RED_MAGIC:		sprintf(c1, "#R"); sprintf(c2, "Red");		break;
	case PURPLE_MAGIC:	sprintf(c1, "#o"); sprintf(c2, "Bronze");	break;
	case BLUE_MAGIC:	sprintf(c1, "#L"); sprintf(c2, "Blue");		break;
	case GREEN_MAGIC:	sprintf(c1, "#7"); sprintf(c2, "Silver");	break;
	case YELLOW_MAGIC:	sprintf(c1, "#y"); sprintf(c2, "Gold");		break;
	case BLACK_MAGIC:	sprintf(c1, "#0"); sprintf(c2, "Black");	break;
	default:		stc("Bug, report to Akuma(color).\n\r", ch); return;
    }
    switch (ch->vampgen)
    {
	case AGE_HATCHLING:	format_buffer(age, sizeof(age), "%sA Hatchling %s Dragon#n[#0%s#n]", c1, c2, ch->name);	break;
	case AGE_VYOUNG:	format_buffer(age, sizeof(age), "%sA Very Young %s Dragon#n[#0%s#n]", c1, c2, ch->name);	break;
	case AGE_YOUNG:		format_buffer(age, sizeof(age), "%sA Young %s Dragon#n[#0%s#n]", c1, c2, ch->name);	break;
	case AGE_JUVENILE:	format_buffer(age, sizeof(age), "%sA Juvenile %s Dragon#n[#0%s#n]", c1, c2, ch->name);	break;
	case AGE_YADULT:	format_buffer(age, sizeof(age), "%sA Young Adult %s Dragon#n[#0%s#n]", c1, c2, ch->name);	break;
	case AGE_ADULT:		format_buffer(age, sizeof(age), "%sAn Adult %s Dragon#n[#0%s#n]", c1, c2, ch->name);	break;
	case AGE_MATURE:	format_buffer(age, sizeof(age), "%sA Mature Adult %s Dragon#n[#0%s#n]", c1, c2, ch->name);	break;
	case AGE_OLD:		format_buffer(age, sizeof(age), "%sAn Old %s Dragon#n[#0%s#n]", c1, c2, ch->name);		break;
	case AGE_VOLD:		format_buffer(age, sizeof(age), "%sA Very Old %s Dragon#n[#0%s#n]", c1, c2, ch->name);	break;
	case AGE_ANCIENT:	format_buffer(age, sizeof(age), "%sAn Ancient %s Dragon#n[#0%s#n]", c1, c2, ch->name);	break;
	case AGE_WYRM:		format_buffer(age, sizeof(age), "%sA %s Wyrm#n[#0%s#n]", c1, c2, ch->name);		break;
	case AGE_AWYRM:		format_buffer(age, sizeof(age), "%sAn Ancient %s Wyrm#n[#0%s#n]", c1, c2, ch->name);	break;
	default:		stc("Bug, report to Akuma(age).\n\r", ch); return;
    }

    stc("Your body grows into a large dragon.\n\r", ch);
    act("$n's body grows and distorts into a large dragon.", ch, NULL, NULL, TO_ROOM);

    clear_stats(ch);
    SET_BIT(ch->affected_by, AFF_POLYMORPH);
    SET_BIT(ch->vampaff, VAM_DISGUISED);
    SET_BIT(ch->polyaff, POLY_DRAGONFORM);
    free_string(ch->morph);
    ch->morph = str_dup(age);

    if (ch->vampgen <= 1) ch->pcdata->atm = 50;
    else if (ch->vampgen < 7) ch->pcdata->atm = (7 - ch->vampgen) * 10;

    if (IS_DRAGAFF(ch, DRAG_CLAWS)) do_claws(ch,"");
    if (IS_DRAGAFF(ch, DRAG_HORNS)) do_horns(ch,"");
    if (IS_DRAGAFF(ch, DRAG_WINGS)) do_wings(ch,"");
    if (IS_DRAGAFF(ch, DRAG_FANGS)) do_fangs(ch,"");
    if (IS_DRAGAFF(ch, DRAG_NIGHT)) do_nightsight(ch,"");
    update_style(ch);
    return;
}

void do_dbreath( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (IS_NPC(ch)) return;
    if (!IS_DRAGON(ch))
    {
	stc("Huh?\n\r",ch);
        return;
    }
    if (!IS_DRAGAFF(ch,DRAG_BREATH))
    {
	stc("You do not have the breath ability yet.\n\r",ch);
	return;
    }
    if (!IS_POLYAFF(ch,POLY_DRAGONFORM))
    {
        stc("You must be in dragon form to use your breath weapon.\n\r",ch);
        return;
    }
    if (arg[0] == '\0' && ch->fighting == NULL)
    {
        send_to_char("Who do you wish to use breath on?\n\r",ch);
        return;
    }
    if ((victim = ch->fighting) == NULL && (victim = get_char_room(ch,arg)) == NULL)
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }
    if (is_safe(ch,victim)) return;

    one_hit(ch, victim, TYPE_HIT, "breath");
    WAIT_STATE(ch, 8);
    return;
}

void do_dust(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_STRING_LENGTH];
    long sn;
    AFFECT_DATA af;
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if(IS_NPC(ch)) return;
    if(!IS_DRAGON(ch))
    {
	stc("Huh?\n\r",ch);
	return;
    }
    if (!IS_DRAGAFF(ch,DRAG_DUST))
    {
	stc("You do not have the dust ability yet.\n\r",ch);
	return;
    }
    if (!IS_POLYAFF(ch,POLY_DRAGONFORM))
    {
        stc("You must be in dragon form to use dust.\n\r",ch);
        return;
    }
    if (!IS_DRAGAFF(ch, DRAG_WINGSOUT))
    {
	stc("You must have your wings to use dust.\n\r",ch);
	return;
    }
    if((victim = ch->fighting) == NULL)
    {
	if(arg[0] == '\0')
	{
	    stc("Dust whom?\r\n",ch);
	    return;
	}
	if((victim = get_char_room(ch,arg)) == NULL)
	{
	    stc("How can you dust someone who is not here?\r\n",ch);
	    return;
	}
    }
    if (is_safe(ch,victim)) return;

    if (!IS_NPC(victim))
    {
	if (IS_SET(victim->act, PLR_HOLYLIGHT)) REMOVE_BIT(victim->act, PLR_HOLYLIGHT);
	else
	{
/*	    stc("They are unaffected by your dust",ch);
	    WAIT_STATE(ch,24);
	    return; */
	}
    }

    if (!IS_AFFECTED(victim, AFF_BLIND))
    {
	if ((sn = skill_lookup("blind")) < 0) return;
	af.type      = sn;
	af.location  = APPLY_HITROLL;
	af.modifier  = -50;
	af.duration  = 20;
	af.bitvector = AFF_BLIND;
	affect_to_char(victim, &af);
    }
    act("You use your wings to blow dust into $N's eyes!",ch,NULL,victim,TO_CHAR);
    act("$n uses $s wings to blow dust into $N's eyes!",ch,NULL,victim,TO_NOTVICT);
    act("$n uses $s wings to blow dust into your eyes!",ch,NULL,victim,TO_VICT);
    WAIT_STATE(ch,24);
    return;
}

void do_stail( CHAR_DATA *ch, char *argument )
{
    char arg [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if (IS_NPC(ch)) return;
    if (!IS_DRAGON(ch))
    {
	stc("Huh?\n\r",ch);
	return;
    }
    if (!IS_DRAGAFF(ch,DRAG_TAIL))
    {
	stc("You do not have the spiked tail ability yet.\n\r",ch);
	return;
    }
    if (!IS_POLYAFF(ch,POLY_DRAGONFORM))
    {
	stc("You must be in dragon form to use your tail.\n\r",ch);
	return;
    }
    if (arg[0] == '\0' && ch->fighting == NULL)
    {
	stc("Who do you wish to use your spiked tail on?\n\r",ch);
	return;
    }
    if ((victim = ch->fighting) == NULL && (victim = get_char_room(ch,arg)) == NULL)
    {
	stc("They aren't here.\n\r",ch);
	return;
    }
    if (is_safe(ch,victim)) return;

    one_hit( ch, victim, TYPE_HIT, "spiked tail" );
    WAIT_STATE(ch,8);
    return;
}


void do_horns( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];

    one_argument( argument, arg );

    if (IS_NPC(ch)) return;

    if (!IS_DRAGON(ch))
    {
        stc("Huh?\n\r",ch);
        return;
    }

    if (IS_DRAGAFF(ch, DRAG_HORNSOUT) )
    {
        send_to_char("Your horns slide back into your skull.\n\r",ch);
        act("$n's horns slide back into $s skull.",ch,NULL,NULL,TO_ROOM);
        REMOVE_BIT(ch->pcdata->dragskills, DRAG_HORNSOUT);
        return;
    }

    if (!IS_DRAGAFF(ch,DRAG_HORNS))
    {
        stc("You do not have the horn ability yet.\n\r",ch);
        return;
    }

    if (!IS_POLYAFF(ch,POLY_DRAGONFORM))
    {
        stc("You must be in dragon form to use horns.\n\r",ch);
        return;
    }

    send_to_char("You grow horns from your skull.\n\r",ch);
    act("$n's horns sprout from $s skull.",ch,NULL,NULL,TO_ROOM);
    SET_BIT(ch->pcdata->dragskills, DRAG_HORNSOUT);
    return;
}

static int dragon_age_price(long age, long *dp, long *qp)
{
    static const long points[]={2500,5000,7500,10000,15000,25000,35000,50000,70000,100000};
    static const long quests[]={0,0,0,0,500,1000,1500,2000,2500,3000};
    long index;
    *dp=*qp=0;
    if(age>AGE_HATCHLING || age<AGE_ANCIENT) return 0;
    index=AGE_HATCHLING-age;
    *dp=points[index]; *qp=quests[index]; return 1;
}

void do_dragonskill( CHAR_DATA *ch, char *argument )
{
    char arg[MIL];
    char buf[MSL];
    long age, skill, cost, qpcost = 0;
    size_t menu_skill;
    static const struct {
        const char *key, *age_name;
        long age, bit;
    } skills[]={
        {"Fangs","Very young",AGE_VYOUNG,DRAG_FANGS},
        {"Nightsight","Very young",AGE_VYOUNG,DRAG_NIGHT},
        {"Claws","Young",AGE_YOUNG,DRAG_CLAWS},
        {"Horns","Young",AGE_YOUNG,DRAG_HORNS},
        {"Wings","Juvenile",AGE_JUVENILE,DRAG_WINGS},
        {"Breath","Juvenile",AGE_JUVENILE,DRAG_BREATH},
        {"Tail","Young adult",AGE_YADULT,DRAG_TAIL},
        {"Truesight","Adult",AGE_ADULT,DRAG_TRUESIGHT},
        {"Dust","Mature",AGE_MATURE,DRAG_DUST},
        {"Magic","Old",AGE_OLD,DRAG_MAGIC},
        {"Scry","Old",AGE_OLD,DRAG_SCRY},
        {"Readaura","Very old",AGE_VOLD,DRAG_READAURA},
        {"Shield","Ancient",AGE_ANCIENT,DRAG_SHIELD}
    };

    one_argument(argument, arg);

    if (!IS_DRAGON(ch))
    {
	stc("Huh?\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
    {
	class_menu_header(ch,"DRAGON TRAINING","Skill","Next cost / status");
	if(dragon_age_price(ch->vampgen,&cost,&qpcost)) {
	    format_buffer(buf,sizeof(buf),"%li DP + %li QP | %s",cost,qpcost,
	        ch->pcdata->classpoints[CP_CURRENT]<cost || ch->pcdata->quest<qpcost
	            ? "Needs DP/QP" : "Affordable");
	    class_menu_row(ch,"Age",buf);
	} else class_menu_row(ch,"Age","-- | MAXED");
	for(menu_skill=0;menu_skill<sizeof(skills)/sizeof(skills[0]);++menu_skill) {
	    if(IS_DRAGAFF(ch,skills[menu_skill].bit)) {
	        class_menu_row(ch,skills[menu_skill].key,"-- | Learned");
	        continue;
	    }
	    cost=1000*(12-skills[menu_skill].age);
	    format_buffer(buf,sizeof(buf),"%li DP | %s%s",cost,
	        ch->vampgen>skills[menu_skill].age ? "Need age: " : "",
	        ch->vampgen>skills[menu_skill].age ? skills[menu_skill].age_name
	            : ch->pcdata->classpoints[CP_CURRENT]<cost ? "Needs DP" : "Affordable");
	    class_menu_row(ch,skills[menu_skill].key,buf);
	}
	stc("Train: dragonskill <skill>. DP = class points; QP = quest points.\n\r",ch);
	return;
    }

    if (!str_cmp(arg, "fangs"))			{age = AGE_VYOUNG; skill = DRAG_FANGS;}
    else if (!str_cmp(arg, "nightsight"))	{age = AGE_VYOUNG; skill = DRAG_NIGHT;}
    else if (!str_cmp(arg, "claws"))		{age = AGE_YOUNG; skill = DRAG_CLAWS;}
    else if (!str_cmp(arg, "horns"))		{age = AGE_YOUNG; skill = DRAG_HORNS;}
    else if (!str_cmp(arg, "wings"))		{age = AGE_JUVENILE; skill = DRAG_WINGS;}
    else if (!str_cmp(arg, "breath"))		{age = AGE_JUVENILE; skill = DRAG_BREATH;}
    else if (!str_cmp(arg, "tail"))		{age = AGE_YADULT; skill = DRAG_TAIL;}
    else if (!str_cmp(arg, "truesight"))	{age = AGE_ADULT; skill = DRAG_TRUESIGHT;}
    else if (!str_cmp(arg, "dust"))		{age = AGE_MATURE; skill = DRAG_DUST;}
    else if (!str_cmp(arg, "magic"))		{age = AGE_OLD; skill = DRAG_MAGIC;}
    else if (!str_cmp(arg, "scry"))		{age = AGE_OLD; skill = DRAG_SCRY;}
    else if (!str_cmp(arg, "readaura"))		{age = AGE_VOLD; skill = DRAG_READAURA;}
    else if (!str_cmp(arg, "shield"))		{age = AGE_ANCIENT; skill = DRAG_SHIELD;}
    else if (!str_cmp(arg, "age"))
    {
	if(!dragon_age_price(ch->vampgen,&cost,&qpcost)) {
	    stc("You have attained the highest possible age.\n\r",ch); return;
	}

	if (ch->pcdata->classpoints[CP_CURRENT] < cost || ch->pcdata->quest < qpcost)
	{
	    sprintf(buf, "You require %li DP and %li QP to advance to the next age.\n\r", cost, qpcost);
	    stc(buf, ch);
	    return;
	}
	ch->vampgen--;
	ch->pcdata->quest -= qpcost;
	ch->pcdata->classpoints[CP_CURRENT] -= cost;
        stc("You have now gone up in age.\n\r", ch);
	return;
    }
    else
    {
        stc("That is not a dragon skill.\n\r", ch);
        return;
    }
    if (ch->vampgen > age)
    {
	do_dragonskill(ch, "");
	return;
    }

    if (IS_DRAGAFF(ch, skill))
    {
	stc("You already have that skill.\n\r", ch);
	return;
    }

    cost = 1000 * (12 - age);
    if (ch->pcdata->classpoints[CP_CURRENT] < cost)
    {
	sprintf(buf, "You need %li dp to learn that skill.\n\r", cost);
	stc(buf, ch);
	return;
    }
    SET_BIT(ch->pcdata->dragskills, skill);
    ch->pcdata->classpoints[CP_CURRENT] -= cost;
    stc("You learn a Dragon skill.\n\r", ch);
    return;
}

void do_wings( CHAR_DATA *ch, char *argument )
{
    char arg [MAX_INPUT_LENGTH];

    one_argument( argument, arg );

    if (IS_NPC(ch)) return;
    if (!IS_DRAGON(ch))
    {
        stc("Huh?\n\r",ch);
        return;
    }
    if (!IS_DRAGAFF(ch,DRAG_WINGS))
    {
        stc("You do not have the wings ability yet.\n\r",ch);
        return;
    }
    if (!IS_POLYAFF(ch,POLY_DRAGONFORM))
    {
        stc("You must be in dragon form to use wings.\n\r",ch);
        return;
    }

    if (IS_DRAGAFF(ch, DRAG_WINGSOUT))
    {
        send_to_char("Your wings slide back into your back.\n\r",ch);
        act("$n's wings slide back into $s back.",ch,NULL,NULL,TO_ROOM);
        REMOVE_BIT(ch->pcdata->dragskills, DRAG_WINGSOUT);
        return;
    }

    send_to_char("You grow wings from your back.\n\r",ch);
    act("$n's wings protrude from $s back.",ch,NULL,NULL,TO_ROOM);
    SET_BIT(ch->pcdata->dragskills, DRAG_WINGSOUT);
    return;
}
