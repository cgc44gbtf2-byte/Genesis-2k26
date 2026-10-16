#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"
extern void init_mm(void);
extern char *target_name;
static void purge(CHAR_DATA *ch) { while(ch->affected) affect_remove(ch,ch->affected); }
static void mage_bad_numbers(CHAR_DATA *ch)
{
    typedef void (*MAGE_INPUT)(CHAR_DATA *,char *);
    static const MAGE_INPUT commands[]={do_brittle_bones,do_mutate_ephemera,
        do_alterweight,do_step_sideways,do_break_dreamshell,do_mage_heal,
        do_better_body,do_polymorph_other,do_electrical_chaos,do_inferno};
    static const char *formats[]={"Enemy %s","Enemy %s","Enemy %s","%s","%s",
        "Actor %s","Actor strength %s","Enemy %s","Enemy %s","%s"};
    static const char *bad[]={"-1","junk","1x","999999999999999999999999999"};
    char buf[MAX_INPUT_LENGTH];
    long mana=ch->mana,hp=ch->hit,quint=ch->quint[BODY],paradox=ch->paradox;
    long flags=ch->affected_by;
    ROOM_INDEX_DATA *room=ch->in_room;
    int i,j;
    for(i=0;i<10;++i) for(j=0;j<4;++j) {
        snprintf(buf,sizeof(buf),formats[i],bad[j]); commands[i](ch,buf);
        assert(ch->mana==mana && ch->hit==hp && ch->quint[BODY]==quint
            && ch->paradox==paradox && ch->affected_by==flags && ch->in_room==room);
    }
    puts("Ten legacy Mage casts: negative, malformed, trailing-junk and overflow inputs rejected before paradox, health, mana, quintessence or room changes");
}
static void drow_variants(CHAR_DATA *ch,CHAR_DATA *v,PC_DATA *vpc,
    ROOM_INDEX_DATA *room,ROOM_INDEX_DATA *remote)
{
    static long protections[]={ROOM_NO_RECALL,ROOM_NO_TELEPORT,ROOM_PRIVATE,ROOM_SOLITARY};
    EXIT_DATA north;
    OBJ_DATA crossbow;
    OBJ_INDEX_DATA crossbow_index;
    DESCRIPTOR_DATA vd;
    char voutput[4096];
    long hp,mana;
    int i,side;
    stop_fighting(ch,TRUE); stop_fighting(v,TRUE); purge(ch); purge(v);
    ch->class=CLASS_DROW; ch->position=v->position=POS_STANDING;
    ch->fight_timer=v->fight_timer=0; ch->affected_by=v->affected_by=0;
    ch->pcdata->drow[DROW_POWERS]=0; ch->pcdata->classpoints[CP_CURRENT]=5000;
    ch->pcdata->drow[DROW_SPEED]=2;
    ch->desc->outtop=0; ch->desc->outbuf[0]=0; do_drowupkeep(ch,"");
    assert(strstr(ch->desc->outbuf,"Speed") && strstr(ch->desc->outbuf,"2 / 4")
        && !strstr(ch->desc->outbuf,"no drow abilities"));
    ch->pcdata->drow[DROW_SPEED]=0;
    do_skipdisc(ch,""); assert(!IS_AFFECTED(ch,AFF_FLYING));
    SET_BIT(ch->pcdata->drow[DROW_POWERS],DROW_SKIP);
    SET_BIT(ch->pcdata->drow[DROW_POWERS],DROW_SPIDER);
    do_summonspider(ch,""); assert(ch->pcdata->followers==0);
    SET_BIT(ch->pcdata->drow[DROW_POWERS],DROW_FIRE|DROW_ACID);
    do_faeriefire(ch,"Actor"); assert(!IS_AFFECTED(ch,AFF_FAERIE_FIRE));
    v->leader=ch; hp=v->hit; do_acidbolt(ch,"Enemy"); do_faeriefire(ch,"Enemy");
    assert(v->hit==hp && !IS_AFFECTED(v,AFF_FAERIE_FIRE)); v->leader=NULL;
    v->act=0; v->pcdata=vpc;
    do_acidbolt(ch,"Enemy"); do_faeriefire(ch,"Enemy");
    assert(v->hit==hp && !IS_AFFECTED(v,AFF_FAERIE_FIRE));
    v->act=ACT_IS_NPC; v->pcdata=NULL;
    do_faeriefire(ch,"Enemy");
    assert(IS_AFFECTED(v,AFF_FAERIE_FIRE)); purge(v);
    SET_BIT(ch->itemaffect,AFF_FLYING);
    do_skipdisc(ch,""); assert(IS_AFFECTED(ch,AFF_FLYING));
    do_skipdisc(ch,""); assert(!IS_AFFECTED(ch,AFF_FLYING));
    REMOVE_BIT(ch->itemaffect,AFF_FLYING);
    room->description=remote->description=str_empty; room->light=remote->light=1;
    char_from_room(v); char_to_room(v,remote); mana=ch->mana;
    for(side=0;side<2;++side) for(i=0;i<4;++i)
    {
        ROOM_INDEX_DATA *blocked=side ? remote : room;
        blocked->room_flags=protections[i]; do_door(ch,"Enemy");
        assert(ch->in_room==room && ch->mana==mana);
        blocked->room_flags=0;
    }
    SET_BIT(ch->affected_by,AFF_WEBBED); do_door(ch,"Enemy");
    assert(ch->in_room==room); REMOVE_BIT(ch->affected_by,AFF_WEBBED);
    SET_BIT(ch->extra,TIED_UP); do_door(ch,"Enemy");
    assert(ch->in_room==room); REMOVE_BIT(ch->extra,TIED_UP);
    ch->fight_timer=1; do_door(ch,"Enemy"); assert(ch->in_room==room); ch->fight_timer=0;
    do_door(ch,"Enemy"); assert(ch->in_room==remote && ch->mana==mana);
    char_from_room(ch); char_to_room(ch,room);
    memset(&north,0,sizeof(north)); north.to_room=remote; room->exit[0]=&north;
    memset(&crossbow,0,sizeof(crossbow)); crossbow.name="crossbow";
    memset(&crossbow_index,0,sizeof(crossbow_index)); crossbow.pIndexData=&crossbow_index;
    crossbow.short_descr="a crossbow"; crossbow.item_type=ITEM_MISSILE;
    crossbow.wear_loc=WEAR_NONE;
    obj_to_char(&crossbow,ch); equip_char(ch,&crossbow,WEAR_HOLD);
    ch->damroll=100; hp=v->hit;
    SET_BIT(remote->room_flags,ROOM_SAFE); do_shoot(ch,"north Enemy");
    assert(ch->in_room==room && ch->pcdata->classpoints[CP_CURRENT]==5000 && v->hit==hp);
    remote->room_flags=0; SET_BIT(v->act,ACT_NOKILL); do_shoot(ch,"north Enemy");
    assert(ch->in_room==room && ch->pcdata->classpoints[CP_CURRENT]==5000 && v->hit==hp);
    REMOVE_BIT(v->act,ACT_NOKILL);
    v->leader=ch; do_shoot(ch,"north Enemy");
    assert(ch->in_room==room && ch->pcdata->classpoints[CP_CURRENT]==5000 && v->hit==hp);
    v->leader=NULL; do_shoot(ch,"north Enemy");
    assert(ch->in_room==room && room->people==ch && remote->people==v);
    assert(ch->pcdata->classpoints[CP_CURRENT]==4500 && v->hit<hp
        && v->position==POS_SLEEPING && IS_AFFECTED(v,AFF_SLEEP));
    purge(v); v->position=POS_STANDING; hp=v->hit; v->act=0; v->pcdata=vpc;
    do_shoot(ch,"north Enemy");
    assert(ch->in_room==room && v->hit==hp && ch->pcdata->classpoints[CP_CURRENT]==4500);
    memset(&vd,0,sizeof(vd)); vd.character=v; vd.outbuf=voutput;
    vd.outsize=sizeof(voutput); voutput[0]=0; v->desc=&vd;
    SET_BIT(ch->pcdata->flag,FLAG_PK); SET_BIT(vpc->flag,FLAG_PK);
    v->class=CLASS_DROW; SET_BIT(vpc->drow[DROW_POWERS],DROW_PIMM);
    do_shoot(ch,"north Enemy");
    assert(ch->in_room==room && v->hit<hp && !IS_AFFECTED(v,AFF_SLEEP)
        && ch->pcdata->classpoints[CP_CURRENT]==4000);
    v->desc=NULL; v->class=CLASS_NONE; v->act=ACT_IS_NPC; v->pcdata=NULL;
    unequip_char(ch,&crossbow); obj_from_char(&crossbow); room->exit[0]=NULL;
    char_from_room(v); char_to_room(v,room);
    puts("Drow: real Skip Disc on/off with unrelated item bits, protected/actual "
        "Dimensional Door, crossbow room integrity, exact costs, safe/NOKILL/group/"
        "non-PK rejection, actual NPC damage/sleep and PK-player poison immunity passed");
}

static void ninja_variants(CHAR_DATA *ch,CHAR_DATA *v,
    ROOM_INDEX_DATA *room,ROOM_INDEX_DATA *remote)
{
    static long protections[]={ROOM_NO_RECALL,ROOM_NO_TELEPORT,ROOM_PRIVATE,ROOM_SOLITARY};
    EXIT_DATA exits[6];
    AFFECT_DATA root;
    int i,side;
    stop_fighting(ch,TRUE); stop_fighting(v,TRUE);
    ch->class=CLASS_NINJA; ch->position=v->position=POS_STANDING;
    ch->pcdata->ninja[NINJA_SORA]=10; ch->pcdata->ninja[NINJA_CHIKYU]=7;
    ch->pcdata->ninja[NINJA_KI]=100;
    ch->desc->outtop=0; ch->desc->outbuf[0]=0; do_principles(ch,"chikyu");
    assert(!strstr(ch->desc->outbuf,"Gunaya"));
    ch->pcdata->ninja[NINJA_CHIKYU]=8;
    ch->desc->outtop=0; ch->desc->outbuf[0]=0; do_principles(ch,"chikyu");
    assert(strstr(ch->desc->outbuf,"8 : Gunaya"));
    do_kakusu(ch,""); assert(IS_AFFECTED(ch,AFF_KAKUSU)
        && ch->pcdata->ninja[NINJA_KI]==80);
    ch->pcdata->ninja[NINJA_KI]=100;
    char_from_room(v); char_to_room(v,remote);
    for(side=0;side<2;++side) for(i=0;i<4;++i)
    {
        ROOM_INDEX_DATA *blocked=side ? remote : room;
        blocked->room_flags=protections[i]; do_mizotamouyo(ch,"Enemy");
        assert(ch->in_room==room && ch->pcdata->ninja[NINJA_KI]==100
            && IS_AFFECTED(ch,AFF_KAKUSU));
        blocked->room_flags=0;
    }
    SET_BIT(ch->extra,TIED_UP); do_mizotamouyo(ch,"Enemy");
    assert(ch->in_room==room && ch->pcdata->ninja[NINJA_KI]==100);
    REMOVE_BIT(ch->extra,TIED_UP);
    ch->fight_timer=1; do_mizotamouyo(ch,"Enemy");
    assert(ch->in_room==room && ch->pcdata->ninja[NINJA_KI]==100); ch->fight_timer=0;
    do_mizotamouyo(ch,"Enemy");
    assert(ch->in_room==remote && ch->pcdata->ninja[NINJA_KI]==80
        && IS_AFFECTED(ch,AFF_KAKUSU));
    char_from_room(ch); char_to_room(ch,room);
    char_from_room(v); char_to_room(v,room);
    do_kakusu(ch,""); assert(!IS_AFFECTED(ch,AFF_KAKUSU)
        && ch->pcdata->ninja[NINJA_KI]==80);
    ch->pcdata->ninja[NINJA_KI]=100; do_michi(ch,"");
    assert(ch->pcdata->ninja[NINJA_KI]==0 && ch->pcdata->wolf==250);
    ch->pcdata->regenerate=0; ch->pcdata->ninja[NINJA_KI]=50; do_orimasu(ch,"");
    assert(ch->pcdata->regenerate==10 && ch->pcdata->ninja[NINJA_KI]==0);
    ch->pcdata->ninja[NINJA_KI]=50; do_orimasu(ch,"");
    assert(ch->pcdata->regenerate==10 && ch->pcdata->ninja[NINJA_KI]==50);
    for(i=0;i<6;++i) {
        memset(&exits[i],0,sizeof(exits[i])); exits[i].to_room=remote;
        room->exit[i]=&exits[i];
    }
    ch->pcdata->ninja[NINJA_KI]=100;
    set_fighting(ch,v); set_fighting(v,ch);
    memset(&root,0,sizeof(root)); root.type=gsn_lineage_root;
    root.duration=2; root.location=APPLY_NONE; affect_to_char(ch,&root);
    do_mienaku(ch,"");
    assert(ch->in_room==room && ch->pcdata->ninja[NINJA_KI]==100 && ch->fighting==v);
    affect_strip(ch,gsn_lineage_root); do_mienaku(ch,"");
    assert(ch->in_room==remote && !ch->fighting && !v->fighting
        && ch->pcdata->ninja[NINJA_KI]==80 && IS_EXTRA(v,BLINDFOLDED));
    char_from_room(ch); char_to_room(ch,room); REMOVE_BIT(v->extra,BLINDFOLDED);
    for(i=0;i<6;++i) room->exit[i]=NULL;
    puts("Ninja: real concealment/free dismissal, exact Michi/Orimasu costs, "
        "WEB menu rank, both-room shadow travel gates, real travel/upkeep and "
        "root-blocked/actual escape with exact20 Ki passed");
}

static void wolf_variants(CHAR_DATA *ch,CHAR_DATA *v,PC_DATA *vpc,
    ROOM_INDEX_DATA *room,ROOM_INDEX_DATA *remote)
{
    static long protections[]={ROOM_NO_RECALL,ROOM_NO_TELEPORT,ROOM_PRIVATE,ROOM_SOLITARY};
    OBJ_DATA head;
    OBJ_INDEX_DATA index;
    int i,side,landed=0,dissolved=0;
    ch->class=CLASS_WEREWOLF;
    ch->pcdata->auspice[AUSPICE_GALLIARD]=5;
    ch->pcdata->tgift[1]=AUSPICE_GALLIARD;
    ch->pcdata->auspice[AUSPICE_RAGABASH]=2;
    SET_BIT(ch->itemaffect,ITEMA_BLISSFUL);
    do_bridgewalk(ch,"Nobody");
    assert(ch->in_room==room && IS_ITEMAFF(ch,ITEMA_BLISSFUL));
    char_from_room(v); char_to_room(v,remote);
    for(side=0;side<2;++side) for(i=0;i<4;++i)
    {
        ROOM_INDEX_DATA *blocked=side ? remote : room;
        blocked->room_flags=protections[i]; do_bridgewalk(ch,"Enemy");
        assert(ch->in_room==room && IS_ITEMAFF(ch,ITEMA_BLISSFUL));
        blocked->room_flags=0;
    }
    SET_BIT(ch->extra,TIED_UP); do_bridgewalk(ch,"Enemy");
    assert(ch->in_room==room && IS_ITEMAFF(ch,ITEMA_BLISSFUL)); REMOVE_BIT(ch->extra,TIED_UP);
    /* No indexed rooms exist in this fixture: misfires must terminate rather
     * than spin forever, while the ordinary target-travel branch still works.
     */
    for(i=0;i<30 && (!landed || !dissolved);++i)
    {
        ch->desc->outtop=0; ch->desc->outbuf[0]=0;
        do_bridgewalk(ch,"Enemy");
        if(ch->in_room==remote) {
            assert(strstr(ch->desc->outbuf,"You create a moon bridge"));
            landed=1; char_from_room(ch); char_to_room(ch,room);
        } else {
            assert(ch->in_room==room && strstr(ch->desc->outbuf,"before finding an allowed landing"));
            dissolved=1;
        }
        assert(room->people==ch && remote->people==v);
    }
    assert(landed && dissolved && !IS_ITEMAFF(ch,ITEMA_BLISSFUL));
    char_from_room(v); char_to_room(v,room);
    ch->pcdata->tribes[TRIBE_UKTENA]=5; ch->pcdata->tgift[2]=TRIBE_UKTENA;
    v->act=0; v->pcdata=vpc;
    memset(&head,0,sizeof(head)); memset(&index,0,sizeof(index));
    head.pIndexData=&index; head.name="%n"; head.short_descr="a test head";
    head.item_type=ITEM_TRASH; head.value[2]=12; head.wear_loc=WEAR_NONE;
    obj_to_char(&head,ch); equip_char(ch,&head,WEAR_HOLD);
    ch->desc->outtop=0; ch->desc->outbuf[0]=0;
    do_fetishdoll(ch,"Enemy");
    assert(head.carried_by==ch && strstr(ch->desc->outbuf,"not Enemy"));
    head.name="head Enemy"; ch->desc->outtop=0; ch->desc->outbuf[0]=0;
    do_fetishdoll(ch,"Enemy");
    assert(head.carried_by==ch && strstr(ch->desc->outbuf,"template is unavailable"));
    unequip_char(ch,&head); obj_from_char(&head); v->act=ACT_IS_NPC; v->pcdata=NULL;
    puts("Werewolf: Bridge Walk exact track, protected/free rejection, actual travel "
        "and bounded empty-world misfire; Fetish Doll literal-format safety and "
        "component preservation without its template passed");
}

static void other_travel_variants(CHAR_DATA *ch,CHAR_DATA *v,
    ROOM_INDEX_DATA *room,ROOM_INDEX_DATA *remote)
{
    static DO_FUN *commands[]={do_celestial,do_watertravel,do_earthshift,do_shadowstep};
    static long classes[]={CLASS_MONK,CLASS_ELEM,CLASS_VAMPIRE,CLASS_VAMPIRE};
    static long protections[]={ROOM_NO_RECALL,ROOM_NO_TELEPORT,ROOM_PRIVATE,ROOM_SOLITARY};
    int mode,side,i;
    long level=ch->level;
    DESCRIPTOR_DATA occupied;
    char_from_room(v); char_to_room(v,remote); v->position=POS_STANDING;
    for(mode=0;mode<4;++mode)
    {
        ch->class=classes[mode]; ch->pcdata->monk[MONK_MANTRA]=7;
        ch->pcdata->powers[ELEM_WATER]=5; ch->pcdata->powers[DISC_PROTEAN]=6;
        ch->pcdata->powers[DISC_OBTENEBRATION]=5;
        if(mode==2) SET_BIT(ch->vampaff,VAM_EARTHMELDED);
        if(mode<2) {
            ch->mana=1499; commands[mode](ch,"Enemy");
            assert(ch->in_room==room && ch->mana==1499);
        }
        ch->mana=10000;
        for(side=0;side<2;++side) for(i=0;i<4;++i) {
            ROOM_INDEX_DATA *blocked=side ? remote : room;
            blocked->room_flags=protections[i]; commands[mode](ch,"Enemy");
            assert(ch->in_room==room && ch->mana==10000);
            blocked->room_flags=0;
        }
        SET_BIT(ch->extra,TIED_UP); commands[mode](ch,"Enemy");
        assert(ch->in_room==room && ch->mana==10000); REMOVE_BIT(ch->extra,TIED_UP);
        ch->fight_timer=3; commands[mode](ch,"Enemy");
        assert(ch->in_room==room && ch->mana==10000); ch->fight_timer=0;
        SET_BIT(v->affected_by,AFF_SHADOWPLANE); commands[mode](ch,"Enemy");
        assert(ch->in_room==room && ch->mana==10000); REMOVE_BIT(v->affected_by,AFF_SHADOWPLANE);
        commands[mode](ch,"Enemy");
        assert(ch->in_room==remote && ch->mana==(mode<2 ? 8500 : 10000));
        char_from_room(ch); char_to_room(ch,room);
        assert(room->people==ch && remote->people==v);
        REMOVE_BIT(ch->vampaff,VAM_EARTHMELDED);
    }
    char_from_room(v); char_to_room(v,room);
    assert(!ch->pcdata->familiar && !v->wizard);
    ch->level=LEV_IMP;
    do_familiar(ch,"Enemy"); assert(!ch->pcdata->familiar);
    v->master=ch; SET_BIT(v->affected_by,AFF_CHARM);
    ch->level=LEV_AWA; do_familiar(ch,"Enemy"); assert(!ch->pcdata->familiar);
    memset(&occupied,0,sizeof(occupied)); v->desc=&occupied;
    ch->level=LEV_IMP; do_familiar(ch,"Enemy"); assert(!ch->pcdata->familiar);
    v->desc=NULL; do_familiar(ch,"Enemy");
    assert(ch->pcdata->familiar==v && v->wizard==ch);
    v->desc=&occupied; ch->desc->outtop=0; ch->desc->outbuf[0]=0;
    do_fcommand(ch,"say must-not-run");
    assert(strstr(ch->desc->outbuf,"unoccupied NPC") && !IS_SET(v->act,ACT_COMMANDED));
    v->desc=NULL; do_fcommand(ch,"say familiar-link-check");
    assert(ch->pcdata->familiar==v && !IS_SET(v->act,ACT_COMMANDED)
        && strstr(ch->desc->outbuf,"familiar-link-check"));
    SET_BIT(v->act,ACT_COMMANDED); do_fcommand(ch,"say nested-link-check");
    assert(IS_SET(v->act,ACT_COMMANDED)); REMOVE_BIT(v->act,ACT_COMMANDED);
    do_familiar(ch,"clear");
    assert(!ch->pcdata->familiar && !v->wizard && v->master==ch);
    v->master=NULL; REMOVE_BIT(v->affected_by,AFF_CHARM);
    ch->level=level;
    puts("Monk/Elemental/Vampire Earth Shift/Shadow Step: exact affordability, both-room protection, "
        "restraint/timer/plane rejection, actual travel and costs passed; "
        "staff-only Familiar rejects unowned/low-trust binding and safely links/releases an owned NPC");
}

int main(void)
{
    CHAR_DATA ch,v;
    PC_DATA pc,vpc;
    ROOM_INDEX_DATA room,remote;
    AREA_DATA area;
    MOB_INDEX_DATA mob;
    DESCRIPTOR_DATA desc;
    OBJ_DATA voice;
    char output[32768],cmd[160];
    long mana,hp,quint,speed,paradox;
    int i;
    const char *bad[]={"-1","no","2junk","999999999999999999999999999999999999999",NULL};
    init_mm();
    for(i=0;i<MAX_SKILL && skill_table[i].name;++i)
        if(skill_table[i].pgsn) *skill_table[i].pgsn=i;
    memset(&ch,0,sizeof(ch)); memset(&v,0,sizeof(v));
    memset(&pc,0,sizeof(pc)); memset(&vpc,0,sizeof(vpc));
    memset(&room,0,sizeof(room)); memset(&remote,0,sizeof(remote));
    memset(&area,0,sizeof(area)); memset(&mob,0,sizeof(mob));
    memset(&desc,0,sizeof(desc)); memset(output,0,sizeof(output));
    ch.pcdata=&pc; ch.class=CLASS_MAGE; ch.name="Actor"; v.name="Enemy"; v.short_descr="an enemy";
    ch.short_descr="the actor"; ch.clan=ch.lord=v.clan=v.lord=str_empty;
    ch.contingency=v.contingency=str_empty;
    pc.love=pc.conding=vpc.love=vpc.conding=str_empty;
    ch.morph=ch.description=ch.long_descr=v.morph=v.description=v.long_descr=str_empty;
    ch.level=LEV_AWA; v.level=20; ch.position=v.position=POS_STANDING;
    ch.hit=ch.max_hit=v.hit=v.max_hit=1000000; ch.mana=ch.max_mana=1000000;
    ch.arete=10; ch.style[0]=v.style[0]=STYLE_UNARMED;
    for(i=0;i<5;++i) ch.stats[i]=v.stats[i]=5;
    for(i=0;i<9;++i) ch.spheres[i]=5;
    v.act=ACT_IS_NPC; v.pIndexData=&mob; mob.vnum=1234;
    room.area=remote.area=&area; room.name=remote.name="test room"; room.vnum=777; remote.vnum=778;
    char_to_room(&ch,&room); char_to_room(&v,&room);
    char_list=&ch; ch.next=&v;
    desc.outbuf=output; desc.outsize=sizeof(output); desc.character=&ch; ch.desc=&desc;
    ch.quint[BODY]=10; mana=ch.mana; hp=ch.hit; paradox=ch.paradox;
    mage_bad_numbers(&ch);
    ch.mana=2999; do_brittle_bones(&ch,"Enemy 2");
    assert(ch.mana==2999 && ch.quint[BODY]==10 && ch.paradox==paradox);
    ch.mana=mana;
    SET_BIT(room.room_flags,ROOM_SAFE); do_brittle_bones(&ch,"Enemy 2");
    assert(ch.mana==mana && ch.quint[BODY]==10 && ch.paradox==paradox);
    REMOVE_BIT(room.room_flags,ROOM_SAFE);
    char_from_room(&v); char_to_room(&v,&remote); v.act|=ACT_NOKILL;
    do_brittle_bones(&ch,"Enemy 2");
    assert(ch.in_room==&room && ch.mana==mana && ch.quint[BODY]==10 && ch.paradox==paradox);
    v.act&=~ACT_NOKILL; char_from_room(&v); char_to_room(&v,&room);
    for(i=0;i<20 && !is_affected(&v,gsn_brittle_bones);++i) {
        ch.quint[BODY]=10; ch.paradox=0; mana=ch.mana;
        do_brittle_bones(&ch,"Enemy 2");
        assert(ch.mana==mana-3000 && ch.quint[BODY]==8 && ch.in_room==&room);
    }
    assert(is_affected(&v,gsn_brittle_bones));
    mana=ch.mana; quint=ch.quint[BODY]; do_brittle_bones(&ch,"Enemy 2");
    assert(ch.mana==mana && ch.quint[BODY]==quint);
    assert(get_soak(&v)==125);
    for(i=0;i<100 && is_affected(&v,gsn_brittle_bones);++i) get_soak(&v);
    assert(i<100 && !is_affected(&v,gsn_brittle_bones) && get_soak(&v)==250);
    ch.quint[BODY]=10; ch.paradox=paradox; mana=ch.mana;
    puts("Brittle Bones: atomic costs, remote safety/restoration, duplicate protection, real soak reduction and safe charge exhaustion passed");
    for(i=0;bad[i];++i) {
        snprintf(cmd,sizeof(cmd),"Enemy %s",bad[i]); do_friction_curse(&ch,cmd);
        assert(ch.mana==mana && ch.quint[BODY]==10 && ch.hit==hp && ch.paradox==paradox);
    }
    ch.mana=4499; do_friction_curse(&ch,"Enemy 2");
    assert(ch.mana==4499 && ch.quint[BODY]==10 && ch.hit==hp);
    ch.mana=mana; room.room_flags|=ROOM_SAFE; do_friction_curse(&ch,"Enemy 2");
    assert(ch.mana==mana && ch.quint[BODY]==10 && ch.in_room==&room);
    room.room_flags&=~ROOM_SAFE;
    char_from_room(&v); char_to_room(&v,&remote); remote.room_flags|=ROOM_SAFE;
    do_friction_curse(&ch,"Enemy 2"); assert(ch.in_room==&room && ch.mana==mana && ch.quint[BODY]==10);
    remote.room_flags&=~ROOM_SAFE; char_from_room(&v); char_to_room(&v,&room);
    v.act=0; v.pcdata=&vpc; v.level=LEV_AWA;
    do_friction_curse(&ch,"Enemy 2");
    assert(ch.mana==mana && ch.quint[BODY]==10 && !is_affected(&v,gsn_friction_curse));
    v.act=ACT_IS_NPC|ACT_NOKILL; v.pcdata=NULL; v.level=20;
    char_from_room(&v); char_to_room(&v,&remote);
    do_friction_curse(&ch,"Enemy 2");
    assert(ch.in_room==&room && ch.mana==mana && ch.quint[BODY]==10);
    v.act=ACT_IS_NPC; char_from_room(&v); char_to_room(&v,&room);
    for(i=0;i<20 && !is_affected(&v,gsn_friction_curse);++i) {
        ch.quint[BODY]=10; ch.paradox=0; mana=ch.mana;
        do_friction_curse(&ch,"Enemy 2");
        assert(ch.mana==mana-4500 && ch.quint[BODY]==8 && ch.in_room==&room);
    }
    assert(is_affected(&v,gsn_friction_curse));
    mana=ch.mana; quint=ch.quint[BODY]; do_friction_curse(&ch,"Enemy 2");
    assert(ch.mana==mana && ch.quint[BODY]==quint);
    speed=get_speed(&v); affect_strip(&v,gsn_friction_curse); assert(get_speed(&v)==speed-2);
    ch.quint[BODY]=10; ch.paradox=0;
    for(i=0;i<20 && !is_affected(&v,gsn_friction_curse);++i) do_friction_curse(&ch,"Enemy 0");
    assert(is_affected(&v,gsn_friction_curse));
    for(i=0;i<30;++i) { hp=v.hit; one_hit(&v,&ch,TYPE_HIT,"test"); if(v.hit<hp) break; }
    assert(i<30 && v.hit==hp-100);
    stop_fighting(&ch,TRUE); purge(&ch); purge(&v);
    ch.class=CLASS_NINJA; pc.ninja[NINJA_SORA]=8; pc.ninja[NINJA_KI]=100;
    room.room_flags|=ROOM_SAFE; do_bomuzite(&ch,"Enemy");
    assert(v.position==POS_STANDING && pc.ninja[NINJA_KI]==100);
    room.room_flags&=~ROOM_SAFE;
    v.act=0; v.pcdata=&vpc; v.level=LEV_AWA;
    do_bomuzite(&ch,"Enemy"); assert(v.position==POS_STANDING && pc.ninja[NINJA_KI]==100);
    v.act=ACT_IS_NPC; v.pcdata=NULL; v.level=20;
    set_fighting(&ch,&v); set_fighting(&v,&ch);
    do_bomuzite(&ch,"Enemy"); assert(v.position==POS_SLEEPING && !v.fighting && !ch.fighting);
    assert(pc.ninja[NINJA_KI]==80);
    do_bomuzite(&ch,"Enemy"); assert(pc.ninja[NINJA_KI]==80);
    v.position=POS_STANDING; pc.ninja[NINJA_KI]=19; do_bomuzite(&ch,"Enemy");
    assert(pc.ninja[NINJA_KI]==19 && v.position==POS_STANDING);
    pc.ninja[NINJA_KI]=100;
    do_bomuzite(&ch,"Actor"); assert(ch.position==POS_STANDING && pc.ninja[NINJA_KI]==100);
    v.leader=&ch; do_bomuzite(&ch,"Enemy");
    assert(v.position==POS_STANDING && pc.ninja[NINJA_KI]==100); v.leader=NULL;
    pc.ninja[NINJA_SORA]=7; do_bomuzite(&ch,"Enemy");
    assert(pc.ninja[NINJA_KI]==100 && v.position==POS_STANDING);
    memset(&voice,0,sizeof(voice)); voice.name="stone"; voice.short_descr="a stone";
    obj_to_room(&voice,&room); desc.outtop=0; output[0]=0;
    target_name="stone hello $n";
    spell_ventriloquate(skill_lookup("ventriloquate"),20,&ch,NULL);
    assert(strstr(output,"illusory voice") && strstr(output,"hello $n"));
    desc.outtop=0; output[0]=0; target_name="stone bad\nline";
    spell_ventriloquate(skill_lookup("ventriloquate"),20,&ch,NULL);
    assert(!strstr(output,"says") && strstr(output,"control characters"));
    obj_from_room(&voice); target_name="";
    ch.class=CLASS_MAGE;
    for(i=0;i<9;++i) ch.spheres[i]=3;
    desc.outtop=0; output[0]=0; do_magepowers(&ch,"");
    assert(strstr(output,"brittlebones") && strstr(output,"friction")
        && strstr(output,"betterbody") && strstr(output,"sculpture")
        && strstr(output,"mageenchant") && strstr(output,"stepsideways")
        && strstr(output,"contingency") && !strstr(output,"inferno"));
    desc.outtop=0; output[0]=0; do_spheres(&ch,"Life");
    assert(strstr(output,"Mage Powers") && !strstr(output,"eventually"));
    ch.spheres[MLIF]=0; desc.outtop=0; output[0]=0; do_spheres(&ch,"Life");
    assert(strstr(output,"no knowledge") && !strstr(output,"Mage Powers"));
    drow_variants(&ch,&v,&vpc,&room,&remote);
    ninja_variants(&ch,&v,&room,&remote);
    wolf_variants(&ch,&v,&vpc,&room,&remote);
    other_travel_variants(&ch,&v,&room,&remote);
    puts("Mage Friction Curse: parsing, exact/atomic costs, invalid/safe/remote/duplicate cases, actual speed and recoil effects passed");
    puts("Ninja sleep bomb: actual NPC sleep/combat cleanup, tier/Ki gates, safe-room/non-PK/self/group protection and duplicate rejection passed");
    puts("Ventriloquate: actual attributed object speech, literal user formatting and control-character rejection passed");
    return 0;
}
