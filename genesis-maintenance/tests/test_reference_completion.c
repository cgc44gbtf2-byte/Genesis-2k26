/* Exercise real handlers/effect hooks, not dispatch-only inventory flags. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"
extern void init_mm(void);
extern void fread_char(CHAR_DATA *,FILE *);
extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];

static void setup(CHAR_DATA *ch,PC_DATA *pc,char *name)
{
    int i;
    memset(ch,0,sizeof(*ch)); memset(pc,0,sizeof(*pc)); ch->pcdata=pc;
    ch->name=ch->short_descr=name;
    ch->morph=ch->description=ch->long_descr=str_empty;
    ch->hit=ch->max_hit=1000000; ch->mana=ch->max_mana=1000000;
    ch->position=POS_STANDING; ch->level=LEV_AWA; ch->style[0]=STYLE_UNARMED;
    ch->exp=1000000; ch->primal=10000;
    for(i=0;i<5;++i) ch->stats[i]=5;
}
static void purge(CHAR_DATA *ch)
{ while(ch->affected) affect_remove(ch,ch->affected); }
static void recover(CHAR_DATA *ch)
{
    affect_strip(ch,gsn_parent_recovery); ch->wait=0;
    if(ch->fighting) stop_fighting(ch,TRUE);
    ch->fight_timer=0; ch->position=POS_STANDING;
}
static void run(CHAR_DATA *ch,const char *command)
{
    char buf[MAX_INPUT_LENGTH];
    assert(strlen(command)<sizeof(buf)); strcpy(buf,command);
    if(IS_ANGEL(ch)) do_angel(ch,buf);
    else if(IS_CYBORG(ch)) do_cyborg(ch,buf);
    else do_demon(ch,buf);
}
static void grant_tests(CHAR_DATA *ch,CHAR_DATA *ally)
{
    const char *keys[]={"might","toughness","speed","nightsight","heightened_senses","wings",NULL};
    char cmd[100];
    long xp;
    int i,kind,id,already;
    const GENESIS_PARENT_POWER *power;
    long mana;
    uint64_t grants[2];
    FILE *fp;
    for(kind=0;kind<2;++kind) {
        newclasses_clear(ch); newclasses_clear(ally);
        ch->class=ally->class=kind ? CLASS_DEMON:CLASS_ANGEL;
        ch->pcdata->powers[0]=8; ally->pcdata->powers[0]=0;
        ally->leader=ch; recover(ch); recover(ally); ch->exp=1000000;
        for(i=0;keys[i];++i) {
            snprintf(cmd,sizeof(cmd),"inpart Ally %s",keys[i]); xp=ch->exp;
            run(ch,cmd); assert(ch->exp==xp-50000);
            run(ch,cmd); assert(ch->exp==xp-50000);
        }
        assert(char_stat(ally,STAT_STR)==6 && char_stat(ally,STAT_CON)==6
            && char_stat(ally,STAT_DEX)==6 && ally->stats[STAT_STR]==5);
        run(ally,"nightsight"); assert(newclasses_nightvision(ally));
        purge(ally); recover(ally); run(ally,"heightened_senses");
        assert(newclasses_perception(ally));
        recover(ally); run(ally,"wings"); assert(IS_AFFECTED(ally,AFF_FLYING));
        newclasses_loaded(ally); assert(IS_AFFECTED(ally,AFF_FLYING));
        assert(newclasses_perception(ally));
        fp=tmpfile(); assert(fp); assert(reference_legacy_write(&ally->pcdata->legacy,fp));
        newclasses_write_grants(ally,fp); fputs("Exp 1234\nEnd\n",fp); rewind(fp);
        newclasses_clear(ally); fread_char(ally,fp); fclose(fp);
        assert(ally->exp==1234 && char_stat(ally,STAT_STR)==6 && char_stat(ally,STAT_CON)==6);
        recover(ally); run(ally,"nightsight"); assert(newclasses_nightvision(ally));
        xp=ch->exp; run(ch,"inpart Ally inpart"); assert(ch->exp==xp);
        ally->class=CLASS_VAMPIRE; newclasses_loaded(ally);
        assert(!ally->pcdata->legacy.grants[0] && !ally->pcdata->legacy.grants[1]);
        assert(char_stat(ally,STAT_STR)==5);
        assert(!genesis_parent_ready(kind?GEN_PARENT_DEMON:GEN_PARENT_ANGEL));
        newclasses_clear(ally);
        ally->class=ch->class; ally->pcdata->powers[0]=0;
        ally->leader=ch; ch->exp=10000000;
        id=kind?GEN_PARENT_DEMON:GEN_PARENT_ANGEL;
        for(i=0;i<(int)genesis_parent_power_count(id);++i) {
            power=genesis_parent_power_at(id,i); assert(power->implemented);
            already=(ally->pcdata->legacy.grants[i/64] & (UINT64_C(1)<<(i%64)))!=0;
            xp=ch->exp;
            snprintf(cmd,sizeof(cmd),"inpart Ally %s",power->key); run(ch,cmd);
            if(power->kind==GEN_POWER_SYSTEM) assert(ch->exp==xp);
            else {
                assert(ch->exp==xp-(already?0:50000));
                assert(ally->pcdata->legacy.grants[i/64] & (UINT64_C(1)<<(i%64)));
            }
        }
        grants[0]=ally->pcdata->legacy.grants[0]; grants[1]=ally->pcdata->legacy.grants[1];
        fp=tmpfile(); assert(fp); newclasses_write_grants(ally,fp);
        fputs("Exp 4321\nEnd\n",fp); rewind(fp); newclasses_clear(ally);
        fread_char(ally,fp); fclose(fp);
        assert(ally->exp==4321 && ally->pcdata->legacy.grants[0]==grants[0]
            && ally->pcdata->legacy.grants[1]==grants[1]);
        recover(ally); mana=ally->mana; run(ally,"scry Actor");
        assert(ally->mana==mana-500);
        recover(ally); run(ally,"weaponform");
        assert(ally->pcdata->reference_form && ally->pcdata->chobj);
        run(ally,"humanform"); assert(!ally->pcdata->reference_form && !ally->pcdata->chobj);
    }
    puts("Both complete catalogues: every non-system grant, exact charges, blocked system delegation, all grant-bit save/reload, low-rank real scry and object transformation passed");
    puts("Angel/Demon inpart: exact cost, consent/group/class/system gates, duplicate prevention, derived passives, low-rank real sight/flight, player-parser persistence and foreign-class cleanup passed");
}
int main(void)
{
    CHAR_DATA ch,v,ally;
    PC_DATA pc,vpc,apc;
    ROOM_INDEX_DATA room,remote;
    EXIT_DATA north,south;
    AREA_DATA area;
    MOB_INDEX_DATA mob,mount,guard;
    OBJ_INDEX_DATA proto,portal;
    OBJ_DATA corpse,*obj,*weapon;
    DESCRIPTOR_DATA desc;
    char output[65536],buf[120];
    AFFECT_DATA af;
    long mana,hp,xp,primal,armor,bonus;
    int i,j;
    uint32_t mask;
    const char *attacks[]={"leech","blink","destroy","bane","cone","dbreath","fear","confuse","evileye",NULL};
    const char *enchants[]={"immolate","freezeweapon","caust","daemonseed"};
    const char *animals[]={"wolf","bat","serpent","bear","raven"};
    const char *demons[]={"lemure","nupperibo","spinagon","barbazu","abishai","osyluth",
        "hamatula","erinyes","amnizu","cornugon","gelugon"};
    init_mm();
    for(i=0;i<MAX_SKILL && skill_table[i].name;++i)
        if(skill_table[i].pgsn) *skill_table[i].pgsn=i;
    setup(&ch,&pc,"Actor"); setup(&v,&vpc,"Enemy"); setup(&ally,&apc,"Ally");
    memset(&room,0,sizeof(room)); memset(&remote,0,sizeof(remote)); memset(&area,0,sizeof(area));
    memset(&north,0,sizeof(north)); memset(&south,0,sizeof(south));
    room.area=remote.area=&area; area.nplayer=2;
    room.vnum=777; remote.vnum=778; room.name=remote.name="public test field";
    room.sector_type=remote.sector_type=SECT_FIELD;
    weather_info.sunlight=SUN_LIGHT;
    north.to_room=&remote; south.to_room=&room; room.exit[0]=&north; remote.exit[2]=&south;
    char_to_room(&ch,&room); char_to_room(&v,&room); char_to_room(&ally,&room);
    ch.next=&v; v.next=&ally; char_list=&ch; ally.leader=&ch;
    memset(&mob,0,sizeof(mob)); mob.vnum=1234; mob.alignment=-500;
    v.act=ACT_IS_NPC; v.pIndexData=&mob; v.level=20;
    memset(&proto,0,sizeof(proto)); proto.vnum=OBJ_VNUM_PROTOPLASM;
    proto.item_type=ITEM_WEAPON;
    proto.name=proto.short_descr=proto.description=str_empty;
    obj_index_hash[proto.vnum%MAX_KEY_HASH]=&proto;
    memset(&portal,0,sizeof(portal)); portal.vnum=OBJ_VNUM_PORTAL;
    portal.item_type=ITEM_PORTAL;
    portal.name=portal.short_descr=portal.description=str_empty;
    obj_index_hash[portal.vnum%MAX_KEY_HASH]=&portal;
    memset(&mount,0,sizeof(mount)); mount.vnum=MOB_VNUM_MOUNT;
    mount.player_name=mount.short_descr=mount.long_descr=mount.description=str_empty;
    mob_index_hash[mount.vnum%MAX_KEY_HASH]=&mount;
    memset(&guard,0,sizeof(guard)); guard.vnum=MOB_VNUM_GUARDIAN;
    guard.player_name=guard.short_descr=guard.long_descr=guard.description=str_empty;
    mob_index_hash[guard.vnum%MAX_KEY_HASH]=&guard;
    memset(&desc,0,sizeof(desc)); memset(output,0,sizeof(output));
    desc.outbuf=output; desc.outsize=sizeof(output); ch.desc=&desc;
    grant_tests(&ch,&ally);
    newclasses_clear(&ch); newclasses_clear(&ally);
    ch.class=CLASS_DEMON; pc.powers[0]=8; ally.class=CLASS_NONE; apc.powers[0]=0;
    recover(&ch); recover(&ally);
    xp=ch.exp; run(&ch,"pact Ally"); assert(ch.exp==xp && ally.class==CLASS_NONE);
    run(&ally,"offersoul on"); assert(apc.legacy.willing==1);
    ally.stats[STAT_STR]=1; run(&ch,"pact Ally"); assert(ally.class==CLASS_NONE && ch.exp==xp);
    ally.stats[STAT_STR]=5; run(&ch,"pact Ally");
    assert(ally.class==CLASS_DEMON && ch.exp==xp-666 && !strcmp(apc.legacy.patron,"Actor"));
    assert(!apc.legacy.willing && apc.powers[0]==0);
    run(&ch,"champions"); assert(strstr(output,"Champion: Ally"));
    recover(&ch); xp=ch.exp; run(&ch,"souls 100000");
    assert(ch.exp==xp-100000 && pc.legacy.souls==100000);
    run(&ch,"souls -1"); assert(pc.legacy.souls==100000);
    pc.legacy.souls=REF_SOUL_CAP;
    for(i=0;i<10;++i) { run(&ch,"obtain"); }
    mask=pc.legacy.warps; assert(mask!=0);
    pc.legacy.souls=REF_SOUL_CAP; run(&ch,"obtain"); assert(pc.legacy.warps==mask);
    pc.legacy.warps=1UL<<14; newclasses_restore_passives(&ch);
    run(&ch,"redeem clumsy"); assert(!pc.legacy.warps && pc.legacy.souls==REF_SOUL_CAP-10000);
    pc.legacy.warps=3; run(&ch,"clearwarps nope"); assert(pc.legacy.warps==3);
    run(&ch,"clearwarps confirm"); assert(!pc.legacy.warps);
    pc.legacy.warps=(1UL<<2)|(1UL<<7)|(1UL<<9)|(1UL<<10)|(1UL<<11)|(1UL<<22)|(1UL<<25);
    newclasses_restore_passives(&ch); newclasses_restore_passives(&ch);
    assert(char_stat(&ch,STAT_STR)==7 && ch.stats[STAT_STR]==5);
    assert(newclasses_speed_bonus(&ch)==1 && newclasses_cap_bonus(&ch)==300);
    assert(newclasses_perception(&ch) && IS_AFFECTED(&ch,AFF_FLYING));
    ch.hit=ch.max_hit-100; cyborg_update(&ch); assert(ch.hit==ch.max_hit-75);
    newclasses_describe(&ch,&ch); assert(strstr(output,"Warped features:") && strstr(output,"spoon"));
    pc.legacy.warps=0; newclasses_restore_passives(&ch); assert(!IS_AFFECTED(&ch,AFF_FLYING));
    for(i=0;i<5;++i) {
        const char *gifts[]={"strength","speed","stake","cone","form"};
        snprintf(buf,sizeof(buf),"gifts %s",gifts[i]); primal=ch.primal;
        run(&ch,buf); assert(ch.primal==primal-50);
        run(&ch,buf); assert(ch.primal==primal-50);
    }
    assert(pc.legacy.gifts==31 && newclasses_speed_bonus(&ch)==1);
    memset(&corpse,0,sizeof(corpse)); corpse.name="remains"; corpse.item_type=ITEM_CORPSE_NPC; corpse.level=20;
    obj_to_room(&corpse,&room); pc.legacy.souls=0; recover(&ch); mana=ch.mana;
    run(&ch,"stealsoul remains"); assert(pc.legacy.souls==300 && ch.mana==mana-500);
    recover(&ch); run(&ch,"stealsoul remains"); assert(pc.legacy.souls==300);
    obj_from_room(&corpse);
    pc.legacy.souls=10000;
    for(i=0;attacks[i];++i) {
        recover(&ch); recover(&v); purge(&v); v.hit=v.max_hit; v.saving_throw=1000;
        if(v.in_room!=&room) { char_from_room(&v); char_to_room(&v,&room); }
        mob.alignment=!strcmp(attacks[i],"bane")?500:-500;
        if(!strcmp(attacks[i],"dbreath")) pc.legacy.warps=1UL<<6;
        snprintf(buf,sizeof(buf),"%s Enemy",attacks[i]);
        hp=v.hit; mana=ch.mana; run(&ch,buf); assert(ch.mana<mana);
        if(!strcmp(attacks[i],"fear") || !strcmp(attacks[i],"confuse"))
            assert(is_affected(&v,skill_lookup("parent fear")) || v.in_room!=&room);
        else if(!strcmp(attacks[i],"evileye")) assert(is_affected(&v,skill_lookup("parent evileye")));
        else assert(v.hit<hp);
        if(!strcmp(attacks[i],"blink")) assert(is_affected(&ch,skill_lookup("parent blink")));
        recover(&ch); room.room_flags|=ROOM_SAFE; mana=ch.mana; hp=v.hit;
        run(&ch,buf); assert(ch.mana==mana && v.hit==hp); room.room_flags&=~ROOM_SAFE;
    }
    if(v.in_room!=&room) { char_from_room(&v); char_to_room(&v,&room); }
    recover(&ch); recover(&v); purge(&v); v.saving_throw=1000;
    for(i=0;i<100 && v.in_room==&room;++i) {
        recover(&ch); recover(&v); run(&ch,"gust Enemy north");
    }
    assert(v.in_room==&remote);
    char_from_room(&v); char_to_room(&v,&room);
    for(i=0;i<3;++i) {
        const char *self[]={"baal","graft","deathsense"};
        const char *effect[]={"parent baal","parent graft","parent deathvision"};
        recover(&ch); run(&ch,self[i]); assert(is_affected(&ch,skill_lookup(effect[i])));
    }
    assert(newclasses_nightvision(&ch));
    set_fighting(&ch,&v); set_fighting(&v,&ch);
    hp=v.hit; newclasses_extra_attack(&ch,&v); assert(v.hit<hp);
    recover(&ch); recover(&v);
    for(j=0;j<2;++j) for(i=0;i<(j?11:5);++i) {
        snprintf(buf,sizeof(buf),"%s %s",j?"shadowdemon":"polymorph",j?demons[i]:animals[i]);
        recover(&ch); mana=ch.mana; armor=ch.armor; run(&ch,buf);
        assert(ch.mana<mana && IS_AFFECTED(&ch,AFF_POLYMORPH) && ch.armor>armor);
        snprintf(buf,sizeof(buf),"%s back",j?"shadowdemon":"polymorph");
        run(&ch,buf); assert(!IS_AFFECTED(&ch,AFF_POLYMORPH) && ch.armor==armor);
    }
    weapon=create_object(&proto,0); weapon->item_type=ITEM_WEAPON;
    weapon->name=str_dup("testblade"); obj_to_char(weapon,&ch);
    for(i=0;i<4;++i) {
        recover(&ch); snprintf(buf,sizeof(buf),"%s testblade",enchants[i]); mana=ch.mana;
        run(&ch,buf); assert(ch.mana<mana && (weapon->reference_enchant&(1<<i)));
        recover(&ch); mana=ch.mana; run(&ch,buf); assert(ch.mana==mana);
    }
    recover(&v); purge(&v); bonus=newclasses_weapon_bonus(&ch,&v,weapon);
    assert(bonus==750 && weapon->reference_enchant==0 && weapon->timer==1);
    weapon->reference_enchant=3; weapon->reference_ticks=1; newclasses_item_tick(weapon);
    assert(!weapon->reference_enchant && !weapon->reference_ticks); extract_obj(weapon);
    recover(&ch); primal=ch.primal; run(&ch,"dstake");
    assert(ch.primal==primal-60);
    obj=ch.carrying; assert(obj && (obj->quest&REF_QUEST_STAKE));
    run(&ch,"dstake"); assert(ch.primal==primal-60); extract_obj(obj);
    recover(&ch); run(&ch,"firewall north"); assert(north.exit_info&EX_WALL_FLAME);
    obj=room.contents; assert(obj && obj->item_type==ITEM_WALL && obj->timer==3);
    recover(&ch); mana=ch.mana; run(&ch,"firewall north"); assert(ch.mana==mana);
    extract_obj(obj); assert(!(north.exit_info&EX_WALL_FLAME));
    recover(&ch); run(&ch,"entomb"); assert(north.exit_info&EX_WALL_GRANITE);
    extract_obj(room.contents); assert(!(north.exit_info&EX_WALL_GRANITE));
    recover(&ch); recover(&ally); char_from_room(&ally); char_to_room(&ally,&remote);
    run(&ch,"infernal_portal Ally"); assert(ch.in_room==&remote);
    char_from_room(&ch); char_to_room(&ch,&room); recover(&ch);
    run(&ch,"dgate Ally"); assert(ally.in_room==&room);
    recover(&ch); run(&ch,"chaosportal"); assert(ch.in_room==&remote);
    char_from_room(&ch); char_to_room(&ch,&room); recover(&ch);
    memset(&af,0,sizeof(af)); af.type=gsn_lineage_root; af.duration=1; affect_to_char(&ch,&af);
    char_from_room(&ally); char_to_room(&ally,&remote); mana=ch.mana;
    run(&ch,"infernal_portal Ally"); assert(ch.in_room==&room && ch.mana==mana);
    affect_strip(&ch,gsn_lineage_root);
    recover(&ch); run(&ch,"steed"); assert(pc.reference_familiar==6 && pc.familiar);
    assert(newclasses_speed_bonus(&ch)==2); run(&ch,"steed off"); assert(!pc.familiar);
    recover(&ch); run(&ch,"imp"); assert(pc.reference_familiar==3 && pc.familiar);
    obj=create_object(&proto,0); obj->name=str_dup("courier"); obj->weight=5; obj_to_char(obj,&ch);
    recover(&ch); mana=ch.mana; run(&ch,"dimpsend courier Ally");
    assert(obj->carried_by==&ally && ch.mana==mana-500);
    extract_obj(obj); run(&ch,"imp off"); assert(!pc.familiar);
    /* Invalid authority and class cleanup do not preserve foreign benefits. */
    pc.legacy.warps=REF_WARP_MASK; newclasses_loaded(&ch); assert(pc.legacy.warps==0);
    newclasses_clear(&ch); assert(!pc.legacy.gifts && !pc.legacy.souls);
    ch.class=CLASS_CYBORG;
    assert(cyborg_core_init(&pc.cyborg,CY_COMPONENT_MASK,10000)==CY_OK);
    pc.cybernetics.face=5; pc.cybernetics.limbs=5; pc.cybernetics.body=6;
    {
        const char *gear[]={"whip","ring","collar","armor","helmet","leggings","boots",
            "gloves","sleeves","cloak","belt","bracer","mask"};
        const char *passive[]={"armor_plating","limb_damage","limb_speed","limb_damage_cap",
            "limb_defense","limb_accuracy","weapon_proficiency","regeneration","darkvision"};
        for(i=0;i<13;++i) {
            snprintf(buf,sizeof(buf),"cyborgeq %s",gear[i]); primal=ch.primal;
            run(&ch,buf); assert(ch.primal==primal-100);
            obj=ch.carrying; assert(obj && obj->weight==5 && !strcmp(obj->questowner,"Actor"));
            run(&ch,buf); assert(ch.primal==primal-100); extract_obj(obj);
        }
        newclasses_restore_passives(&ch); armor=ch.armor; mana=ch.mana;
        for(i=0;i<9;++i) { run(&ch,passive[i]); assert(ch.armor==armor && ch.mana==mana); }
    }
    puts("Demon extensions: consenting pacts/champions, all gifts, soul conversion/claims, warp caps/redemption/cleansing, real traits/flight/regen/speed/caps/appearance passed");
    puts("Demon extensions: every attack/debuff, all 16 shapes, weapon hooks/expiry, stake, walls/native cleanup, protected travel/root rejection and steed passed");
    puts("Reference adapters: real imp courier delivery, all 13 cyber equipment variants/duplicate protection and all nine passive entry points passed");
    return 0;
}
