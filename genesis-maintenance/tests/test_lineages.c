#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"
extern void init_mm(void);
extern void fread_char(CHAR_DATA *,FILE *);
extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
typedef struct test_power {
    int parent; const char *key,*kind; int track,level; long mana,energy,amount;
} TEST_POWER;
static const TEST_POWER powers[]={
#define L(parent,key,title,track,level,mana,energy,kind,amount,source) \
 {GEN_PARENT_##parent,#key,#kind,track,level,mana,energy,amount},
#include "../src/lineage_powers.def"
#undef L
};
static int is(const TEST_POWER *p,const char *kind) { return !strcmp(p->kind,kind); }
static void run(CHAR_DATA *ch,const char *text)
{
    char input[MAX_INPUT_LENGTH];
    assert(strlen(text)<sizeof(input)); strcpy(input,text);
    if(ch->class==CLASS_GARGOYLE) do_gargoyle(ch,input);
    else if(ch->class==CLASS_KUEIJIN) do_kueijin(ch,input);
    else do_wraith(ch,input);
}
static void purge(CHAR_DATA *ch)
{
    while(ch->affected) affect_remove(ch,ch->affected);
}
static void recover(CHAR_DATA *ch)
{
    affect_strip(ch,skill_lookup("lineage recovery")); affect_strip(ch,gsn_parent_recovery); ch->wait=0;
}
static void setup(CHAR_DATA *ch,PC_DATA *pc,char *name)
{
    int i;
    memset(ch,0,sizeof(*ch)); memset(pc,0,sizeof(*pc)); ch->pcdata=pc;
    ch->name=ch->short_descr=name; ch->morph=ch->long_descr=ch->description=str_empty;
    ch->hit=ch->max_hit=1000000; ch->mana=ch->max_mana=1000000;
    ch->position=POS_STANDING; ch->level=LEV_AWA; ch->style[0]=STYLE_UNARMED;
    for(i=0;i<5;++i) ch->stats[i]=5;
}
int main(void)
{
    CHAR_DATA ch,v,ally,loaded;
    PC_DATA pc,vpc,apc,lpc;
    ROOM_INDEX_DATA room,remote;
    AREA_DATA area;
    EXIT_DATA north,south;
    MOB_INDEX_DATA mobs[2],target;
    OBJ_INDEX_DATA proto;
    OBJ_DATA corpse,weapon,*obj;
    LINEAGE_STATE master;
    AFFECT_DATA af;
    FILE *fp;
    const TEST_POWER *p;
    const char *classes[]={"gargoyle","kuei-jin","wraith"};
    const char *tracks[3][18]={{"visceratika"},{"yin","yang","hun","po","blood","bone","jade","flesh",
        "ghost","equilibrium","tapestry","yangprana","yinprana","blackwind","chimuh","internalize","dragonscales","demonart"},
        {"argos","castigate","embody","inhabit","keening","moliate","outrage","pandemonium","phantasm","puppetry","usury"}};
    long mana,energy,hp,xp,base,raw[5];
    int i,j,k,n,tries,done,counts[3]={0,0,0},ids[]={MOB_VNUM_EYE,MOB_VNUM_GUARDIAN};
    char input[MAX_INPUT_LENGTH];
    init_mm();
    for(i=0;i<MAX_SKILL && skill_table[i].name;++i)
        if(skill_table[i].pgsn) *skill_table[i].pgsn=i;
    memset(&room,0,sizeof(room)); memset(&remote,0,sizeof(remote)); memset(&area,0,sizeof(area));
    memset(&north,0,sizeof(north)); memset(&south,0,sizeof(south)); memset(&target,0,sizeof(target));
    memset(&corpse,0,sizeof(corpse)); memset(&weapon,0,sizeof(weapon)); memset(&proto,0,sizeof(proto));
    memset(mobs,0,sizeof(mobs));
    room.area=remote.area=&area; room.name=remote.name="lineage test"; room.description=remote.description=str_empty;
    room.light=remote.light=1; room.vnum=990071; remote.vnum=990072; area.nplayer=3;
    north.to_room=&remote; south.to_room=&room;
    room.exit[DIR_NORTH]=&north; remote.exit[DIR_SOUTH]=&south;
    setup(&ch,&pc,"Actor"); setup(&v,&vpc,"Enemy"); setup(&ally,&apc,"Ally");
    ch.in_room=v.in_room=ally.in_room=&room; room.people=&ch; ch.next_in_room=&v; v.next_in_room=&ally;
    ch.next=&v; v.next=&ally; char_list=&ch; ally.leader=&ch;
    v.act=ACT_IS_NPC; v.pIndexData=&target; v.level=20;
    for(i=0;i<2;++i) {
        mobs[i].vnum=ids[i]; mobs[i].level=1; mobs[i].act=ACT_IS_NPC;
        mobs[i].player_name=mobs[i].short_descr=mobs[i].long_descr=mobs[i].description=str_empty;
        assert(!mob_index_hash[ids[i]%MAX_KEY_HASH]); mob_index_hash[ids[i]%MAX_KEY_HASH]=&mobs[i];
    }
    proto.vnum=OBJ_VNUM_PROTOPLASM; proto.name=proto.short_descr=proto.description=str_empty;
    proto.item_type=ITEM_TRASH; proto.wear_flags=ITEM_TAKE;
    assert(!obj_index_hash[proto.vnum%MAX_KEY_HASH]); obj_index_hash[proto.vnum%MAX_KEY_HASH]=&proto;
    corpse.name="remains"; corpse.item_type=ITEM_CORPSE_NPC; corpse.in_room=&room; corpse.level=20;
    weapon.name="testblade"; weapon.item_type=ITEM_WEAPON; weapon.carried_by=&ch;
    weapon.wear_loc=WEAR_WEAPON; weapon.value[1]=10; weapon.value[2]=20; weapon.value[3]=TYPE_SLASH;
    for(j=0;j<3;++j) {
        ch.class=CLASS_NONE; do_autoclass(&ch,(char *)classes[j]);
        assert(lineage_valid(&ch) && pc.lineage.energy==1000);
        assert(pc.powers[0]==0);
        for(k=0;k<5;++k) raw[k]=ch.stats[k];
        ch.exp=1000000000;
        if(j==1) for(k=0;k<5;++k) {
            xp=ch.exp; run(&ch,"dharma train"); assert(pc.lineage.dharma==k+1);
            assert(ch.exp==xp-100000*(k+1)*(k+1));
        }
        n=lineage_track_count(pc.lineage.parent);
        for(k=0;k<n;++k) {
            for(i=1;i<=5;++i) {
                snprintf(input,sizeof(input),"train %s",tracks[j][k]); xp=ch.exp;
                run(&ch,input); assert(pc.lineage.tracks[k]==i && ch.exp==xp-25000*i*i);
            }
            xp=ch.exp; run(&ch,input); assert(ch.exp==xp && pc.lineage.tracks[k]==5);
        }
        master=pc.lineage;
        for(k=0;k<30;++k) lineage_restore(&ch);
        for(k=0;k<5;++k) assert(ch.stats[k]==raw[k] && char_stat(&ch,k)<=raw[k]+1);
        for(i=0;i<(int)(sizeof(powers)/sizeof(powers[0]));++i) {
            p=&powers[i]; if(p->parent!=master.parent) continue;
            stop_fighting(&ch,TRUE); stop_fighting(&v,TRUE);
            ch.fight_timer=v.fight_timer=0; ch.position=v.position=ally.position=POS_STANDING;
            newclasses_clear(&ch); purge(&v); purge(&ally);
            pc.lineage=master; pc.lineage.energy=10000; lineage_restore(&ch);
            ch.hit=ch.max_hit=1000000; ch.mana=ch.max_mana=1000000; ch.primal=1000;
            v.hit=v.max_hit=1000000; v.mana=v.max_mana=1000000; ally.hit=ally.max_hit=1000000;
            room.room_flags=remote.room_flags=0; north.exit_info=0; ch.carrying=NULL;
            strcpy(input,p->key);
            if(is(p,"HEAL") || is(p,"CALM") || is(p,"CURE") || is(p,"MENTOR")
                || is(p,"WAKE") || is(p,"BLESSSIGHT") || is(p,"ITEMJUMP")) strcat(input," Ally");
            else if(is(p,"TRAVEL")) strcat(input," Ally");
            else if(is(p,"READ") || is(p,"MELEE") || is(p,"STRIKE") || is(p,"DRAIN")
                || is(p,"POISON") || is(p,"BLIND") || is(p,"BURST") || is(p,"ROOT")
                || is(p,"FEAR") || is(p,"WEAKEN") || is(p,"CRIPPLE") || is(p,"DREAM")) strcat(input," Enemy");
            else if(is(p,"HARVEST")) { strcat(input," remains"); corpse.quest=0; room.contents=&corpse; pc.lineage.energy=5000; }
            else if(is(p,"ARMOR")) strcat(input," robe");
            else if(is(p,"WEATHER")) strcat(input," rain");
            if(is(p,"SELFHEAL") || is(p,"REST")) ch.hit-=5000;
            if(is(p,"HEAL")) ally.hit-=5000;
            if(is(p,"WAKE")) ally.position=POS_SLEEPING;
            if(is(p,"CALM") || is(p,"CURE")) {
                memset(&af,0,sizeof(af)); af.type=skill_lookup("lineage fear"); af.duration=2;
                af.location=APPLY_HITROLL; af.modifier=-10; affect_to_char(&ally,&af);
                if(is(p,"CURE")) {
                    af.type=gsn_poison; af.location=APPLY_STR; af.modifier=-1; af.bitvector=AFF_POISON;
                    affect_to_char(&ally,&af);
                }
            }
            if(is(p,"TRAVEL")) { char_from_room(&ally); char_to_room(&ally,&remote); }
            if(is(p,"FIREEDGE")) ch.carrying=&weapon;
            if(is(p,"ITEMJUMP") || is(p,"REFORM")) {
                assert(newclasses_lineage_reference(&ch,3,"armor",0));
                recover(&ch);
            }
            if(is(p,"AKUMA")) {
                pc.lineage.dharma=4; mana=ch.mana; run(&ch,input); assert(ch.mana==mana);
                pc.lineage.dharma=5;
            }
            mana=ch.mana; energy=pc.lineage.energy; hp=v.hit;
            fprintf(stderr,"lineage ability: %s %s\n",classes[j],input);
            run(&ch,input);
            if(is(p,"REFORM")) assert(!pc.reference_form && !pc.chobj);
            else {
                if(is(p,"DRAIN")) assert(ch.mana==mana-p->mana+150);
                else assert(ch.mana<=mana-p->mana);
                if(is(p,"HARVEST")) assert(pc.lineage.energy==energy+300 && (corpse.quest&REF_QUEST_SOUL_CLAIMED));
                else if(is(p,"BURST")) assert(pc.lineage.energy==0);
                else assert(pc.lineage.energy==energy-p->energy);
            }
            if(is(p,"MELEE") || is(p,"STRIKE") || is(p,"DRAIN") || is(p,"POISON")
                || is(p,"BLIND") || is(p,"BURST") || is(p,"CRIPPLE")) assert(v.hit<hp);
            if(is(p,"SELFHEAL") || is(p,"REST")) assert(ch.hit==1000000-5000+p->amount);
            if(is(p,"HEAL")) assert(ally.hit==1000000-5000+p->amount);
            if(is(p,"WAKE")) assert(ally.position==POS_STANDING);
            if(is(p,"MENTOR")) assert(is_affected(&ally,skill_lookup("lineage mentor")));
            if(is(p,"BLESSSIGHT")) assert(is_affected(&ally,gsn_detect_hidden));
            if(is(p,"CALM") || is(p,"CURE")) assert(!is_affected(&ally,skill_lookup("lineage fear")));
            if(is(p,"CURE")) assert(!is_affected(&ally,gsn_poison) && char_stat(&ally,STAT_STR)==5);
            if(is(p,"WINGS")) assert(IS_AFFECTED(&ch,AFF_FLYING));
            if(is(p,"HIDE")) assert(lineage_concealed(&ch));
            if(is(p,"FORM") || is(p,"MASK") || is(p,"AGILITY") || is(p,"RAGE") || is(p,"AKUMA"))
                assert(lineage_aspect(&ch)!=NULL);
            if(is(p,"EDGE") || is(p,"FIREEDGE")) assert(lineage_weapon_bonus(&ch,&v,is(p,"FIREEDGE")?&weapon:NULL)==p->amount);
            if(is(p,"SENSE")) assert(lineage_nightvision(&ch) && lineage_perception(&ch));
            if(is(p,"PHASE")) assert(lineage_absorb(&ch,1000,TYPE_SPELL)==1000-10*p->amount);
            if(is(p,"PASS")) assert(is_affected(&ch,gsn_lineage_pass));
            if(is(p,"WEATHER")) assert(weather_info.sky==SKY_RAINING);
            if(is(p,"OBJECT")) assert(pc.chobj && pc.reference_form && pc.chobj->chobj==&ch);
            if(is(p,"ITEMJUMP")) assert(pc.chobj && pc.chobj->carried_by==&ally);
            if(is(p,"SCOUT") || is(p,"GUARD")) {
                assert(pc.familiar && pc.familiar->wizard==&ch && pc.reference_familiar);
                lineage_restore(&ch); newclasses_restore_passives(&ch); assert(pc.familiar);
            }
            if(is(p,"CRAFT") || is(p,"ARMOR")) {
                assert(ch.carrying && ch.carrying->timer==5 && (ch.carrying->quest&QUEST_VANISH));
                if(is(p,"ARMOR")) assert(ch.primal==930 && ch.carrying->item_type==ITEM_ARMOR);
            }
            if(is(p,"TRAVEL")) { assert(ch.in_room==&remote); char_from_room(&ch); char_to_room(&ch,&room); char_from_room(&ally); char_to_room(&ally,&room); }
            if(is(p,"ROOT") || is(p,"FEAR") || is(p,"WEAKEN") || is(p,"DREAM")) {
                done=0;
                for(tries=0;tries<40;++tries) {
                    done=is_affected(&v,is(p,"ROOT")?gsn_lineage_root:skill_lookup(is(p,"WEAKEN")?"lineage frailty":"lineage fear"));
                    if(done) break;
                    recover(&ch); pc.lineage.energy=10000; run(&ch,input);
                }
                assert(done);
                if(is(p,"ROOT")) { move_char(&v,DIR_NORTH); assert(v.in_room==&room); }
                if(is(p,"DREAM")) assert(v.position==POS_SLEEPING);
            }
            if(is(p,"HIDE") || is(p,"FORM") || is(p,"SHELL") || is(p,"SHIELD") || is(p,"WINGS") || is(p,"SENSE") || is(p,"EDGE")) {
                recover(&ch); mana=ch.mana; energy=pc.lineage.energy; run(&ch,input);
                assert(ch.mana==mana && pc.lineage.energy==energy);
            }
            if(is(p,"HARVEST")) {
                recover(&ch); mana=ch.mana; run(&ch,input); assert(ch.mana==mana); room.contents=NULL;
            }
            if(is(p,"FIREEDGE")) ch.carrying=NULL;
            if(is(p,"CRAFT") || is(p,"ARMOR")) { obj=ch.carrying; extract_obj(obj); }
            ++counts[j];
        }
        stop_fighting(&ch,TRUE); stop_fighting(&v,TRUE); ch.fight_timer=v.fight_timer=0;
        ch.position=v.position=POS_STANDING;
        newclasses_clear(&ch); purge(&v); pc.lineage=master; pc.lineage.energy=10000; lineage_restore(&ch);
        room.room_flags=ROOM_SAFE; mana=ch.mana; hp=v.hit;
        run(&ch,j==0?"stonefist Enemy":j==1?"chirift Enemy":"requiem Enemy");
        assert(ch.mana==mana && v.hit==hp); room.room_flags=0;
        if(j==0) {
            assert(lineage_absorb(&ch,1000,TYPE_HIT)==750 && lineage_absorb(&ch,1000,TYPE_SPELL)==1000);
            assert(lineage_ground_awareness(&ch,&v));
            SET_BIT(v.affected_by,AFF_FLYING); assert(!lineage_ground_awareness(&ch,&v)); REMOVE_BIT(v.affected_by,AFF_FLYING);
        }
        fp=tmpfile(); assert(fp); assert(lineage_state_write(&pc.lineage,fp)); fputs("Exp 987654\nEnd\n",fp); rewind(fp);
        setup(&loaded,&lpc,"Loaded"); loaded.class=ch.class; loaded.in_room=&room;
        fread_char(&loaded,fp); fclose(fp);
        assert(lpc.lineage.parent==master.parent && loaded.exp==987654);
        for(k=0;k<LINEAGE_TRACKS;++k) assert(lpc.lineage.tracks[k]==master.tracks[k]);
        base=char_stat(&loaded,STAT_CON); lineage_loaded(&loaded); assert(char_stat(&loaded,STAT_CON)==base);
        loaded.class=CLASS_MOOGLE; lineage_loaded(&loaded); assert(!lpc.lineage.parent);
        newclasses_clear(&ch);
        for(k=0;k<5;++k) assert(ch.stats[k]==raw[k] && char_stat(&ch,k)==raw[k]);
        printf("%s: %d converted/original abilities invoked; all tracks trained; costs, effects, safety, persistence and cleanup passed\n",classes[j],counts[j]);
    }
    for(i=0;i<2;++i) mob_index_hash[ids[i]%MAX_KEY_HASH]=NULL;
    obj_index_hash[proto.vnum%MAX_KEY_HASH]=NULL;
    do_gargoyle(NULL,""); do_kueijin(NULL,""); do_wraith(NULL,"");
    do_wraith(&v,"train argos"); assert(!vpc.lineage.parent);
    return 0;
}
