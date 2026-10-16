#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"
extern void init_mm(void);

static void funds(CHAR_DATA *ch)
{
    ch->hit=ch->max_hit=1000000;
    ch->mana=ch->max_mana=1000000;
    ch->quint[BODY]=10; ch->paradox=0; ch->wait=0;
}

static void help_checks(CHAR_DATA *ch, DESCRIPTOR_DATA *d)
{
    HELP_DATA prefix, exact, hidden;
    HELP_DATA *saved=help_first;
    char oversized[MAX_INPUT_LENGTH+2];
    memset(&prefix,0,sizeof(prefix)); memset(&exact,0,sizeof(exact));
    memset(&hidden,0,sizeof(hidden));
    prefix.keyword="ARMORCLASS"; prefix.text="PREFIX-REFERENCE";
    hidden.keyword="ARMOR SECRET"; hidden.text="HIDDEN-REFERENCE";
    hidden.level=MAX_LEVEL+1;
    exact.keyword="ARMOR"; exact.text="EXACT-REFERENCE";
    prefix.next=&hidden; hidden.next=&exact; help_first=&prefix;
    d->outtop=0; d->outbuf[0]=0;
    do_help(ch,"armor");
    assert(strstr(d->outbuf,"EXACT-REFERENCE") && !strstr(d->outbuf,"PREFIX-REFERENCE")
        && !strstr(d->outbuf,"HIDDEN-REFERENCE"));
    d->outtop=0; d->outbuf[0]=0; do_help(ch,"armorc");
    assert(strstr(d->outbuf,"PREFIX-REFERENCE"));
    d->outtop=0; d->outbuf[0]=0; do_help(ch,"secret");
    assert(strstr(d->outbuf,"No help") && !strstr(d->outbuf,"HIDDEN-REFERENCE"));
    memset(oversized,'x',sizeof(oversized)-1); oversized[sizeof(oversized)-1]=0;
    d->outtop=0; d->outbuf[0]=0; do_help(ch,oversized);
    assert(strstr(d->outbuf,"too long"));
    help_first=saved; d->outtop=0; d->outbuf[0]=0;
}

static void rift_checks(CHAR_DATA *ch,CHAR_DATA *v,ROOM_INDEX_DATA *room,
    ROOM_INDEX_DATA *remote)
{
    static long protections[]={ROOM_SAFE,ROOM_PRIVATE,ROOM_SOLITARY,
        ROOM_NO_RECALL,ROOM_NO_TELEPORT};
    OBJ_INDEX_DATA portal,*saved;
    long hash=OBJ_VNUM_PORTAL%MAX_KEY_HASH;
    int i,side;
    funds(ch); char_from_room(v); char_to_room(v,remote);
    for(side=0;side<2;++side) for(i=0;i<5;++i)
    {
        ROOM_INDEX_DATA *blocked=side ? remote : room;
        blocked->room_flags=protections[i]; do_rift(ch,"Enemy 0");
        assert(ch->in_room==room && ch->mana==1000000 && ch->quint[BODY]==10);
        blocked->room_flags=0;
    }
    ch->fighting=v; do_rift(ch,"Enemy 0"); ch->fighting=NULL;
    SET_BIT(ch->affected_by,AFF_WEBBED); do_rift(ch,"Enemy 0");
    REMOVE_BIT(ch->affected_by,AFF_WEBBED);
    assert(ch->mana==1000000 && ch->quint[BODY]==10 && ch->in_room==room);
    saved=obj_index_hash[hash]; assert(get_obj_index(OBJ_VNUM_PORTAL)==NULL);
    do_rift(ch,"Enemy 2");
    assert(ch->mana==1000000 && ch->quint[BODY]==10 && !room->contents && !remote->contents);
    ch->mana=499; do_rift(ch,"Enemy 0");
    assert(ch->mana==499 && ch->quint[BODY]==10 && ch->in_room==room);
    funds(ch); do_rift(ch,"Enemy");
    assert(ch->mana==999500 && ch->quint[BODY]==10 && ch->in_room==remote);
    char_from_room(ch); char_to_room(ch,room); funds(ch);
    memset(&portal,0,sizeof(portal)); portal.vnum=OBJ_VNUM_PORTAL;
    /* Production prototype strings live in the shared area-string pool. */
    portal.name=portal.short_descr=portal.description=str_empty;
    portal.item_type=ITEM_PORTAL;
    portal.next=saved; obj_index_hash[hash]=&portal;
    do_rift(ch,"Enemy 2");
    assert(ch->in_room==room && ch->mana==999500 && ch->quint[BODY]==8);
    assert(room->contents && remote->contents);
    assert(room->contents->item_type==ITEM_WGATE && room->contents->value[0]==remote->vnum
        && room->contents->value[1]==1 && room->contents->timer>0);
    assert(remote->contents->value[0]==room->vnum && remote->contents->value[1]==2
        && remote->contents->timer==room->contents->timer);
    extract_obj(room->contents); extract_obj(remote->contents);
    assert(!room->contents && !remote->contents && portal.count==0);
    obj_index_hash[hash]=saved; char_from_room(v); char_to_room(v,room);
    puts("Rift: both-room protection gates, restraint/combat, missing portal template, "
        "exact costs, zero-quintessence travel and real linked portals passed");
}

int main(void)
{
    typedef void (*CAST)(CHAR_DATA *,char *);
    static CAST attacks[]={do_mutate_ephemera,do_alterweight,do_electrical_chaos};
    static long bases[]={8000,6000,3500};
    static char *attributes[]={"str","dex","con","int","wis"};
    static long bits[]={XTRA_BETTER_STR,XTRA_BETTER_DEX,XTRA_BETTER_CON,
        XTRA_BETTER_INT,XTRA_BETTER_WIS};
    static char *prepared[]={"brittlebones","mutateephemera","electricalchaos",
        "frictioncurse","inferno","polymorphother","alterweight"};
    static long prepared_cost[]={2000,8000,3500,3500,5000,13000,6000};
    static char *double_commands[]={"inferno 2","mutateephemera Enemy 2",
        "electricalchaos Enemy 2","mageheal Actor 2","alterweight Enemy 2"};
    static long double_cost[]={14200,12000,12000,9400,17000};
    CHAR_DATA ch,v;
    PC_DATA pc,vpc;
    ROOM_INDEX_DATA room,remote;
    AREA_DATA area;
    MOB_INDEX_DATA mob;
    DESCRIPTOR_DATA d,vd;
    OBJ_DATA weapon;
    char output[131072],voutput[32768],command[128];
    long mana,hp,q,paradox,before,speed;
    int i,n;
    init_mm();
    for(i=0;i<MAX_SKILL && skill_table[i].name;++i)
        if(skill_table[i].pgsn) *skill_table[i].pgsn=i;
    memset(&ch,0,sizeof(ch)); memset(&v,0,sizeof(v));
    memset(&pc,0,sizeof(pc)); memset(&vpc,0,sizeof(vpc));
    memset(&room,0,sizeof(room)); memset(&remote,0,sizeof(remote));
    memset(&area,0,sizeof(area)); memset(&mob,0,sizeof(mob));
    memset(&d,0,sizeof(d)); memset(&vd,0,sizeof(vd));
    ch.pcdata=&pc; v.pcdata=&vpc; ch.class=CLASS_MAGE;
    ch.name="Actor"; v.name="Enemy"; ch.short_descr="the actor"; v.short_descr="the enemy";
    ch.clan=ch.lord=v.clan=v.lord=pc.love=pc.conding=vpc.love=vpc.conding=str_empty;
    ch.morph=v.morph=ch.description=v.description=ch.long_descr=v.long_descr=str_empty;
    ch.contingency=v.contingency=str_empty;
    ch.level=LEV_AWA; v.level=20; ch.position=v.position=POS_STANDING;
    ch.style[0]=v.style[0]=STYLE_UNARMED;
    ch.arete=10; v.act=ACT_IS_NPC; v.pIndexData=&mob;
    mob.vnum=1234; v.hit=v.max_hit=1000000;
    for(i=0;i<9;++i) ch.spheres[i]=5;
    for(i=0;i<5;++i) ch.stats[i]=v.stats[i]=5;
    room.name=remote.name="test room"; room.description=remote.description=str_empty;
    room.light=remote.light=1; room.area=remote.area=&area;
    area.name="test area"; room.vnum=777; remote.vnum=778;
    char_to_room(&ch,&room); char_to_room(&v,&room);
    char_list=&ch; ch.next=&v;
    d.outbuf=output; d.outsize=sizeof(output); d.character=&ch; ch.desc=&d;
    vd.outbuf=voutput; vd.outsize=sizeof(voutput); vd.character=&v; v.desc=&vd;
    output[0]=voutput[0]=0;
    funds(&ch); help_checks(&ch,&d); rift_checks(&ch,&v,&room,&remote);
    for(i=0;i<3;++i)
    {
        funds(&ch); mana=ch.mana; hp=ch.hit; paradox=ch.paradox;
        attacks[i](&ch,"Missing 2");
        assert(ch.mana==mana && ch.hit==hp && ch.quint[BODY]==10 && ch.paradox==paradox);
        SET_BIT(room.room_flags,ROOM_SAFE); attacks[i](&ch,"Enemy 2");
        assert(ch.mana==mana && ch.quint[BODY]==10 && ch.paradox==paradox);
        REMOVE_BIT(room.room_flags,ROOM_SAFE);
        char_from_room(&v); char_to_room(&v,&remote);
        SET_BIT(v.act,ACT_NOKILL); attacks[i](&ch,"Enemy 2");
        assert(ch.in_room==&room && ch.mana==mana && ch.quint[BODY]==10
            && ch.paradox==paradox);
        REMOVE_BIT(v.act,ACT_NOKILL);
        SET_BIT(remote.room_flags,ROOM_PRIVATE); attacks[i](&ch,"Enemy 2");
        assert(ch.in_room==&room && ch.mana==mana && ch.quint[BODY]==10);
        REMOVE_BIT(remote.room_flags,ROOM_PRIVATE);
        char_from_room(&v); char_to_room(&v,&room);
        ch.mana=0; SET_BIT(ch.xtra,XTRA_HEARTS_BLOOD); ch.quint[BODY]=0;
        attacks[i](&ch,"Enemy 2");
        assert(ch.hit==hp && ch.mana==0 && ch.quint[BODY]==0 && ch.paradox==paradox);
        REMOVE_BIT(ch.xtra,XTRA_HEARTS_BLOOD); funds(&ch); before=v.hit;
        speed=get_speed(&v);
        attacks[i](&ch,"Enemy 2");
        assert(ch.mana==1000000-bases[i]-400-600 && ch.quint[BODY]==8 && ch.in_room==&room);
        if(i==0)
        {
            assert(IS_XTRA(&v,XTRA_MUTATE_EPHEMERA) && v.ephemera_timer>0);
            mana=ch.mana; attacks[i](&ch,"Enemy 2");
            assert(ch.mana==mana && ch.quint[BODY]==8);
            REMOVE_BIT(v.xtra,XTRA_MUTATE_EPHEMERA); v.ephemera_timer=0;
        }
        else
        {
            assert(v.hit<before);
            if(i==1) assert(is_affected(&v,gsn_alter_weight) && get_speed(&v)==speed+3);
        }
        stop_fighting(&ch,TRUE);
        while(v.affected) affect_remove(&v,v.affected);
        if(i==1) assert(get_speed(&v)==speed);
    }
    /* Every attribute changes derived stats, never permanent base stats. */
    v.leader=&ch;
    for(i=0;i<5;++i)
    {
        funds(&ch); before=char_stat(&v,i);
        snprintf(command,sizeof(command),"Enemy %s 2",attributes[i]);
        do_better_body(&ch,command);
        assert(IS_XTRA(&v,bits[i]) && v.better_body_timer>0
            && char_stat(&v,i)==before+1 && v.stats[i]==5);
        assert(ch.mana==994100 && ch.quint[BODY]==8);
        mana=ch.mana; do_better_body(&ch,command);
        assert(ch.mana==mana && ch.quint[BODY]==8);
        REMOVE_BIT(v.xtra,bits[i]); v.better_body_timer=0;
        assert(char_stat(&v,i)==before);
    }
    funds(&ch); do_better_body(&ch,"Enemy invalid 2");
    assert(ch.mana==1000000 && ch.quint[BODY]==10 && !ch.paradox);
    v.hit=500000; funds(&ch); do_mage_heal(&ch,"Enemy 2");
    assert(v.hit>500000 && v.hit<=v.max_hit && ch.mana==996500 && ch.quint[BODY]==8);
    v.leader=NULL; funds(&ch); hp=v.hit; do_mage_heal(&ch,"Enemy 2");
    assert(v.hit==hp && ch.mana==1000000 && ch.quint[BODY]==10 && !ch.paradox);
    ch.hit=500000; do_mage_heal(&ch,"Actor 2");
    assert(ch.hit>500000 && ch.mana==996800 && ch.quint[BODY]==8);
    funds(&ch); do_mage_heal(&ch,"Actor 2");
    assert(ch.mana==1000000 && ch.quint[BODY]==10 && !ch.paradox);
    /* Planar toggles debit only real quintessence and preserve the room. */
    funds(&ch); SET_BIT(room.room_flags,ROOM_NO_TELEPORT);
    do_step_sideways(&ch,"2");
    assert(ch.quint[BODY]==10 && !ch.paradox && !IS_AFFECTED(&ch,AFF_SHADOWPLANE));
    REMOVE_BIT(room.room_flags,ROOM_NO_TELEPORT);
    for(n=0;n<20 && !IS_AFFECTED(&ch,AFF_SHADOWPLANE);++n)
    {
        funds(&ch); do_step_sideways(&ch,"2");
        assert(ch.quint[BODY]==8 && ch.mana==1000000 && ch.in_room==&room);
    }
    assert(IS_AFFECTED(&ch,AFF_SHADOWPLANE));
    for(n=0;n<20 && !IS_AFFECTED(&ch,AFF_DEEPUMBRA);++n)
    {
        funds(&ch); do_break_dreamshell(&ch,"2"); assert(ch.quint[BODY]==8);
    }
    assert(IS_AFFECTED(&ch,AFF_DEEPUMBRA)); funds(&ch); do_step_sideways(&ch,"2");
    assert(ch.quint[BODY]==10);
    REMOVE_BIT(ch.affected_by,AFF_SHADOWPLANE|AFF_DEEPUMBRA);
    funds(&ch); do_break_dreamshell(&ch,"2"); assert(ch.quint[BODY]==10 && !ch.paradox);
    /* Polymorph respects PC/PK eligibility and a duplicate is free. */
    funds(&ch); do_polymorph_other(&ch,"Enemy 2");
    assert(ch.mana==1000000 && ch.quint[BODY]==10);
    v.act=0; v.level=LEV_AWA; v.class=CLASS_NONE;
    do_polymorph_other(&ch,"Enemy 2"); assert(ch.mana==1000000 && !ch.paradox);
    SET_BIT(pc.flag,FLAG_PK); SET_BIT(vpc.flag,FLAG_PK);
    for(n=0;n<20 && !IS_POLYAFF(&v,POLY_PIG);++n)
    {
        funds(&ch); do_polymorph_other(&ch,"Enemy 2");
        assert(ch.mana==986000 && ch.quint[BODY]==8 && ch.in_room==&room);
    }
    assert(IS_POLYAFF(&v,POLY_PIG) && v.polymorph_timer>0);
    mana=ch.mana; q=ch.quint[BODY]; do_polymorph_other(&ch,"Enemy 2");
    assert(ch.mana==mana && ch.quint[BODY]==q);
    REMOVE_BIT(v.polyaff,POLY_PIG); REMOVE_BIT(v.affected_by,AFF_POLYMORPH);
    free_string(v.morph); v.morph=str_empty; v.act=ACT_IS_NPC;
    funds(&ch); SET_BIT(room.room_flags,ROOM_SAFE); do_inferno(&ch,"2");
    assert(ch.mana==1000000 && ch.quint[BODY]==10 && !ch.paradox);
    REMOVE_BIT(room.room_flags,ROOM_SAFE); hp=v.hit; do_inferno(&ch,"2");
    assert(v.hit<hp && IS_AFFECTED(&v,AFF_FLAMING) && ch.mana==994400
        && ch.quint[BODY]==8 && ch.in_room==&room);
    stop_fighting(&ch,TRUE);
    /* Fixed-cost Prime spells must actually debit quintessence, exactly once. */
    REMOVE_BIT(v.affected_by,AFF_FLAMING);
    memset(&weapon,0,sizeof(weapon));
    weapon.name="testweapon"; weapon.short_descr="a test weapon";
    weapon.item_type=ITEM_WEAPON; weapon.wear_loc=WEAR_NONE;
    weapon.carried_by=&ch; ch.carrying=&weapon;
    funds(&ch); ch.mana=0; ch.quint[BODY]=0;
    SET_BIT(ch.xtra,XTRA_HEARTS_BLOOD); hp=ch.hit;
    do_holy_smote(&ch,""); do_mageenchant(&ch,"testweapon");
    assert(ch.hit==hp && ch.mana==0 && ch.quint[BODY]==0);
    REMOVE_BIT(ch.xtra,XTRA_HEARTS_BLOOD);
    for(n=0;n<20 && !IS_XTRA(&ch,XTRA_HOLY_SMOTE);++n)
    {
        funds(&ch); do_holy_smote(&ch,"");
        assert(ch.mana==996000 && ch.quint[BODY]==8);
    }
    assert(IS_XTRA(&ch,XTRA_HOLY_SMOTE));
    mana=ch.mana; do_holy_smote(&ch,"");
    assert(!IS_XTRA(&ch,XTRA_HOLY_SMOTE) && ch.mana==mana && ch.quint[BODY]==8);
    for(n=0;n<20 && !IS_SET(weapon.quest,QUEST_AGGRAVATED);++n)
    {
        funds(&ch); do_mageenchant(&ch,"testweapon");
        assert(ch.mana==995000 && ch.quint[BODY]==5);
    }
    assert(IS_SET(weapon.quest,QUEST_AGGRAVATED));
    mana=ch.mana; do_mageenchant(&ch,"testweapon");
    assert(ch.mana==mana && ch.quint[BODY]==5); ch.carrying=NULL;
    /* Exercise the real combat-start hook and nested set_fighting callback.
     * A staged spell is disarmed before dispatch and cannot fire recursively.
     */
    for(i=0;i<7;++i)
    {
        funds(&ch); v.hit=v.max_hit=1000000;
        v.act=(i==5) ? 0 : ACT_IS_NPC; v.class=CLASS_NONE;
        do_contingency(&ch,prepared[i]);
        assert(!str_cmp(ch.contingency,prepared[i]) && ch.mana==998000);
        do_contingency(&ch,prepared[i]); assert(ch.mana==998000);
        set_fighting(&ch,&v);
        assert(ch.contingency[0]==0 && ch.quint[BODY]==10
            && ch.mana==1000000-4000-prepared_cost[i]-(i==4 ? 0 : 400));
        stop_fighting(&ch,TRUE);
        while(v.affected) affect_remove(&v,v.affected);
        REMOVE_BIT(v.xtra,XTRA_MUTATE_EPHEMERA); v.ephemera_timer=0;
        REMOVE_BIT(v.polyaff,POLY_PIG); REMOVE_BIT(v.affected_by,AFF_POLYMORPH|AFF_FLAMING);
        free_string(v.morph); v.morph=str_empty;
    }
    v.act=ACT_IS_NPC;
    funds(&ch); do_contingency(&ch,"inferno");
    mana=ch.mana; do_contingency(&ch,""); assert(ch.mana==mana);
    do_contingency(&ch,"clear"); assert(ch.contingency[0]==0 && ch.mana==mana);
    funds(&ch); do_contingency(&ch,"unknown");
    assert(ch.mana==1000000 && ch.quint[BODY]==10 && !ch.paradox);
    for(i=0;i<5;++i)
    {
        funds(&ch); v.hit=1000000; ch.doublecast_timer=0;
        if(i==3) ch.hit=500000;
        do_double_cast(&ch,double_commands[i]);
        assert(ch.doublecast_timer>=100 && ch.doublecast_timer<=600
            && ch.mana==1000000-double_cost[i] && ch.quint[BODY]==(i==1 ? 8 : 6));
        mana=ch.mana; q=ch.quint[BODY]; paradox=ch.paradox;
        do_double_cast(&ch,double_commands[i]);
        assert(ch.mana==mana && ch.quint[BODY]==q && ch.paradox==paradox);
        stop_fighting(&ch,TRUE);
        while(v.affected) affect_remove(&v,v.affected);
        REMOVE_BIT(v.xtra,XTRA_MUTATE_EPHEMERA); v.ephemera_timer=0;
        REMOVE_BIT(v.affected_by,AFF_FLAMING);
    }
    funds(&ch); ch.doublecast_timer=0;
    do_double_cast(&ch,"unknown Enemy 2");
    do_double_cast(&ch,"inferno -1");
    do_double_cast(&ch,"electricalchaos Missing 2");
    assert(ch.mana==1000000 && ch.quint[BODY]==10 && !ch.paradox && !ch.doublecast_timer);
    puts("Nine Mage variants: exact/atomic costs, protected remote-room restoration, real damage/healing, all five derived-stat buffs, consent, planar travel, polymorph and Inferno passed");
    puts("Help lookup: exact-first, abbreviation fallback, inaccessible-topic protection and oversized input passed");
    puts("Prime: Holy Smote and Mage Enchant exact/atomic costs, real activation and free dismissal/duplicate checks passed");
    puts("Time: all seven Contingency combat-start selections, recursive-call prevention, managed strings, free status/clear, all five Doublecast selections and cooldown/invalid-input gates passed");
    return 0;
}
