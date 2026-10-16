#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"
extern void init_mm(void);
extern void fread_char(CHAR_DATA *ch, FILE *fp);

static void run(CHAR_DATA *ch,const char *text)
{
    char input[MAX_INPUT_LENGTH];
    assert(strlen(text)<sizeof(input)); strcpy(input,text);
    if (ch->class==CLASS_MOOGLE) do_moogle(ch,input); else do_deathknight(ch,input);
}
static void recover(CHAR_DATA *ch)
{
    affect_strip(ch,skill_lookup("crafted recovery"));
    ch->wait=0;
}
static void peace(CHAR_DATA *ch,CHAR_DATA *v)
{
    stop_fighting(ch,TRUE); stop_fighting(v,TRUE);
    ch->fight_timer=v->fight_timer=0;
    ch->position=v->position=POS_STANDING;
    recover(ch);
}
static void actor(CHAR_DATA *ch,PC_DATA *pc,char *name)
{
    int i;
    memset(ch,0,sizeof(*ch)); memset(pc,0,sizeof(*pc)); ch->pcdata=pc;
    ch->name=name; ch->short_descr=name;
    ch->morph=ch->long_descr=ch->description=str_empty;
    ch->level=LEV_AWA; ch->position=POS_STANDING;
    ch->hit=ch->max_hit=100000; ch->mana=ch->max_mana=100000;
    ch->style[0]=STYLE_UNARMED;
    for(i=0;i<5;++i) ch->stats[i]=5;
}
int main(void)
{
    CHAR_DATA ch,enemy,ally,loaded;
    PC_DATA pc,epc,apc,lpc;
    ROOM_INDEX_DATA room,home;
    AREA_DATA area;
    MOB_INDEX_DATA mob;
    OBJ_DATA weapon,corpse,gear;
    AFFECT_DATA af;
    long i,j,mana,energy,xp,hp,raw[5],hit,dam,ac;
    FILE *fp;
    char oversized[MAX_INPUT_LENGTH+5];
    const char *names[]={"moogle","deathknight"};
    const char *mattacks[]={"pom_bolt Enemy","pomflare Enemy","pomnova Enemy",NULL};
    const char *dattacks[]={"deathstrike Enemy","soulrend Enemy","doomblade Enemy","deathcoil Enemy",NULL};
    const char *buffs[]={"pom_ward","lucky_star","mogdance","graveward","unholy_might"};
    const char *effects[]={"crafted ward","crafted fortune","crafted dance","crafted ward","crafted might"};
    init_mm();
    for(i=0;i<MAX_SKILL && skill_table[i].name;++i)
        if(skill_table[i].pgsn) *skill_table[i].pgsn=i;
    memset(&room,0,sizeof(room)); memset(&home,0,sizeof(home));
    memset(&area,0,sizeof(area)); memset(&mob,0,sizeof(mob));
    memset(&weapon,0,sizeof(weapon)); memset(&corpse,0,sizeof(corpse)); memset(&gear,0,sizeof(gear));
    room.area=home.area=&area; room.vnum=990051; home.vnum=990052;
    room.name=home.name="a test room"; room.description=home.description=str_empty;
    room.light=home.light=1; area.nplayer=3;
    actor(&ch,&pc,"Actor"); actor(&enemy,&epc,"Enemy"); actor(&ally,&apc,"Ally");
    ch.in_room=enemy.in_room=ally.in_room=&room;
    ch.next_in_room=&enemy; enemy.next_in_room=&ally; room.people=&ch;
    ch.next=&enemy; enemy.next=&ally; char_list=&ch;
    ally.leader=&ch;
    enemy.act=ACT_IS_NPC; enemy.pIndexData=&mob; enemy.level=20;
    home.next=room_index_hash[home.vnum%MAX_KEY_HASH];
    room_index_hash[home.vnum%MAX_KEY_HASH]=&home;
    weapon.name="testblade"; weapon.item_type=ITEM_WEAPON;
    weapon.wear_loc=WEAR_WEAPON; weapon.carried_by=&ch; weapon.value[1]=10; weapon.value[2]=20;
    weapon.value[3]=TYPE_SLASH;
    for(j=0;j<2;++j) {
        ch.class=CLASS_NONE; ch.stats[STAT_STR]=1;
        do_autoclass(&ch,(char *)names[j]); assert(ch.class==CLASS_NONE);
        ch.stats[STAT_STR]=5;
        do_autoclass(&ch,"moogle extra"); assert(ch.class==CLASS_NONE);
        do_autoclass(&ch,(char *)names[j]);
        assert(ch.class==(j?CLASS_DEATHKNIGHT:CLASS_MOOGLE));
        assert(pc.crafted.parent==(j?GEN_PARENT_DEATHKNIGHT:GEN_PARENT_MOOGLE));
        assert(pc.crafted.rank==0 && pc.crafted.energy==1000 && pc.powers[0]==0);
        for(i=0;i<5;++i) raw[i]=ch.stats[i];
        ch.exp=100000000;
        for(i=1;i<=8;++i) {
            xp=ch.exp; run(&ch,"train");
            assert(pc.crafted.rank==i && ch.exp==xp-50000*i*i);
        }
        xp=ch.exp; run(&ch,"train"); assert(pc.crafted.rank==8 && ch.exp==xp);
        assert(char_stat(&ch,j?STAT_STR:STAT_INT)==6);
        assert(char_stat(&ch,j?STAT_CON:STAT_WIS)==6);
        assert(char_stat(&ch,j?STAT_WIS:STAT_DEX)==6);
        assert(stat_max(&ch,j?STAT_STR:STAT_INT)==10);
        for(i=0;i<5;++i) assert(ch.stats[i]==raw[i]);
        for(i=0;i<20;++i) crafted_restore(&ch);
        assert(char_stat(&ch,j?STAT_STR:STAT_INT)==6);
        pc.crafted.energy=9000; mana=ch.mana;
        run(&ch,"focus 99999999999999999999"); run(&ch,"focus -1"); run(&ch,"focus 1.5");
        assert(pc.crafted.energy==9000 && ch.mana==mana);
        run(&ch,"focus 1000");
        assert(pc.crafted.energy==10000 && ch.mana==mana-1000);
        recover(&ch); mana=ch.mana; run(&ch,"focus 1");
        assert(ch.mana==mana && pc.crafted.energy==10000);
        for(i=j?3:0;i<(j?5:3);++i) {
            recover(&ch); mana=ch.mana; energy=pc.crafted.energy;
            hit=ch.hitroll; dam=ch.damroll; ac=ch.armor;
            run(&ch,buffs[i]); assert(ch.mana<mana && pc.crafted.energy<energy);
            assert(is_affected(&ch,skill_lookup(effects[i])));
            recover(&ch); mana=ch.mana; energy=pc.crafted.energy; run(&ch,buffs[i]);
            assert(ch.mana==mana && pc.crafted.energy==energy);
            affect_strip(&ch,skill_lookup(effects[i]));
            assert(ch.hitroll==hit && ch.damroll==dam && ch.armor==ac);
        }
        peace(&ch,&enemy);
        for(i=0;(j?dattacks[i]:mattacks[i]);++i) {
            hp=enemy.hit=100000; mana=ch.mana; energy=pc.crafted.energy;
            run(&ch,j?dattacks[i]:mattacks[i]);
            assert(enemy.hit<hp && ch.mana<mana && pc.crafted.energy<energy);
            mana=ch.mana; hp=enemy.hit; run(&ch,j?dattacks[i]:mattacks[i]);
            assert(ch.mana==mana && enemy.hit==hp);
            peace(&ch,&enemy);
            pc.crafted.energy=10000;
        }
        if(!j) {
            ally.hit=95000; mana=ch.mana;
            run(&ch,"kupo_heal Ally");
            assert(ally.hit==98000 && ch.mana==mana-700);
            recover(&ch); ally.leader=NULL; mana=ch.mana;
            run(&ch,"kupo_heal Ally"); assert(ch.mana==mana); ally.leader=&ch;
            recover(&ch); hit=ally.hitroll; dam=ally.damroll;
            run(&ch,"cheer Ally"); assert(ally.hitroll==hit+15 && ally.damroll==dam+15);
            recover(&ch); mana=ch.mana; run(&ch,"cheer Ally"); assert(ch.mana==mana);
            affect_strip(&ally,skill_lookup("crafted morale"));
            assert(ally.hitroll==hit && ally.damroll==dam);
            recover(&ch); memset(&af,0,sizeof(af)); af.type=gsn_poison; af.duration=3;
            af.location=APPLY_STR; af.modifier=-1; af.bitvector=AFF_POISON; affect_to_char(&ally,&af);
            ally.hit=ally.max_hit; run(&ch,"serenade Ally");
            assert(!is_affected(&ally,gsn_poison) && ally.hit==ally.max_hit);
            recover(&ch); run(&ch,"flutter"); assert(IS_AFFECTED(&ch,AFF_FLYING));
            gear.item_type=ITEM_ARMOR; gear.value[3]=39; gear.wear_loc=WEAR_BODY; ch.carrying=&gear;
            affect_strip(&ch,skill_lookup("crafted wings")); assert(IS_AFFECTED(&ch,AFF_FLYING));
            ch.carrying=NULL; REMOVE_BIT(ch.affected_by,AFF_FLYING);
            recover(&ch); run(&ch,"flutter"); assert(IS_AFFECTED(&ch,AFF_FLYING));
            affect_strip(&ch,skill_lookup("crafted wings")); assert(!IS_AFFECTED(&ch,AFF_FLYING));
            recover(&ch); ch.home=home.vnum; home.room_flags=ROOM_PRIVATE; mana=ch.mana;
            run(&ch,"kupostep"); assert(ch.in_room==&room && ch.mana==mana);
            home.room_flags=0; run(&ch,"kupostep"); assert(ch.in_room==&home && ch.mana==mana-1200);
            char_from_room(&ch); char_to_room(&ch,&room); recover(&ch);
            puts("Moogle: all eleven abilities, costs, ally consent, cure, nonstacking and protected/actual home travel passed");
        } else {
            ch.carrying=&weapon; recover(&ch); mana=ch.mana;
            run(&ch,"frostedge"); assert(ch.mana==mana-600);
            assert(crafted_weapon_bonus(&ch,&enemy,&weapon)==150);
            room.room_flags=ROOM_SAFE; assert(crafted_weapon_bonus(&ch,&enemy,&weapon)==0); room.room_flags=0;
            affect_strip(&ch,skill_lookup("crafted weapon")); ch.carrying=NULL;
            recover(&ch); mana=ch.mana; run(&ch,"frostedge"); assert(ch.mana==mana);
            for(i=0;i<20 && !is_affected(&enemy,skill_lookup("crafted fear"));++i) {
                peace(&ch,&enemy); pc.crafted.energy=10000; run(&ch,"dreadgaze Enemy");
            }
            assert(is_affected(&enemy,skill_lookup("crafted fear")));
            affect_strip(&enemy,skill_lookup("crafted fear")); peace(&ch,&enemy);
            ch.hit=20000; run(&ch,"laststand"); assert(ch.hit==25000);
            recover(&ch); ch.hit=ch.max_hit; mana=ch.mana; run(&ch,"laststand"); assert(ch.mana==mana);
            corpse.name="remains"; corpse.item_type=ITEM_CORPSE_NPC; corpse.level=20;
            corpse.in_room=&room; room.contents=&corpse; pc.crafted.energy=5000;
            recover(&ch); run(&ch,"soulharvest remains"); assert(pc.crafted.energy==5300);
            assert(corpse.quest & REF_QUEST_SOUL_CLAIMED);
            recover(&ch); mana=ch.mana; run(&ch,"soulharvest remains");
            assert(ch.mana==mana && pc.crafted.energy==5300); room.contents=NULL;
            puts("Deathknight: all ten abilities, real frost weapon hook, soul claim once, fear and bounded Last Stand passed");
        }
        peace(&ch,&enemy); pc.crafted.energy=10000;
        if(j) {
            affect_strip(&enemy,skill_lookup("crafted frailty"));
            affect_strip(&enemy,skill_lookup("crafted doom"));
            for(i=0;i<30 && !is_affected(&enemy,skill_lookup("crafted frailty"));++i) {
                run(&ch,"soulrend Enemy"); peace(&ch,&enemy); pc.crafted.energy=10000;
            }
            assert(is_affected(&enemy,skill_lookup("crafted frailty")));
            assert(enemy.saving_throw==10);
            affect_strip(&enemy,skill_lookup("crafted frailty")); assert(enemy.saving_throw==0);
            dam=enemy.damroll;
            for(i=0;i<30 && !is_affected(&enemy,skill_lookup("crafted doom"));++i) {
                run(&ch,"doomblade Enemy"); peace(&ch,&enemy); pc.crafted.energy=10000;
            }
            assert(is_affected(&enemy,skill_lookup("crafted doom")) && enemy.damroll==dam-15);
            affect_strip(&enemy,skill_lookup("crafted doom")); assert(enemy.damroll==dam);
        }
        room.room_flags=ROOM_SAFE; mana=ch.mana; hp=enemy.hit;
        run(&ch,j?"deathstrike Enemy":"pom_bolt Enemy");
        assert(ch.mana==mana && enemy.hit==hp); room.room_flags=0;
        mana=ch.mana; run(&ch,j?"deathstrike Ally":"pom_bolt Ally");
        assert(ch.mana==mana);
        pc.crafted.energy=0; run(&ch,j?"deathstrike Enemy":"pom_bolt Enemy"); assert(ch.mana==mana);
        pc.crafted.energy=1000;
        memset(oversized,'x',sizeof(oversized)-1); oversized[sizeof(oversized)-1]=0;
        run(&ch,"not_a_power"); do_moogle(&ch,oversized); do_deathknight(&ch,oversized); assert(ch.mana==mana);
        actor(&loaded,&lpc,"Loaded"); loaded.class=ch.class; loaded.in_room=&room;
        fp=tmpfile(); assert(fp); assert(crafted_state_write(&pc.crafted,fp));
        fputs("Exp 123456\nEnd\n",fp); rewind(fp); fread_char(&loaded,fp); fclose(fp);
        assert(lpc.crafted.parent==pc.crafted.parent && lpc.crafted.rank==8
            && lpc.crafted.energy==1000 && loaded.exp==123456);
        assert(char_stat(&loaded,j?STAT_STR:STAT_INT)==6);
        crafted_loaded(&loaded); assert(char_stat(&loaded,j?STAT_STR:STAT_INT)==6);
        loaded.class=j?CLASS_MOOGLE:CLASS_DEATHKNIGHT; crafted_loaded(&loaded);
        assert(lpc.crafted.parent==crafted_class_parent(&loaded) && !lpc.crafted.rank && !lpc.crafted.energy);
        crafted_clear(&loaded);
        newclasses_clear(&ch);
        for(i=0;i<5;++i) assert(ch.stats[i]==raw[i] && char_stat(&ch,i)==raw[i]);
        assert(!pc.crafted.parent && !pc.crafted.rank && !pc.crafted.energy);
    }
    room_index_hash[home.vnum%MAX_KEY_HASH]=home.next;
    do_moogle(NULL,""); do_deathknight(NULL,"");
    do_moogle(&enemy,"train"); assert(enemy.class==CLASS_NONE && !epc.crafted.parent);
    puts("Crafted classes: independent progression, raw-stat preservation, caps, full player-parser round trip, class mismatch and cleanup passed");
    return 0;
}
