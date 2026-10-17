#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"
extern void init_mm(void);
extern void char_update(void);
extern void fread_char(CHAR_DATA *,FILE *);
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
static CHAR_DATA a,b,loaded;
static PC_DATA ap,bp,lp;
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static AFFECT_DATA *recovery(CHAR_DATA *ch,long sn)
{
    AFFECT_DATA *af;
    for(af=ch->affected;af;af=af->next) if(af->type==sn) return af;
    return NULL;
}
static void setup(CHAR_DATA *ch,PC_DATA *pc,char *name,long kind)
{
    int i;
    clear_char(ch); memset(pc,0,sizeof(*pc)); ch->pcdata=pc;
    ch->name=ch->short_descr=name; ch->class=kind; ch->level=LEV_AWA;
    ch->hit=ch->max_hit=ch->mana=ch->max_mana=1000000;
    ch->style[0]=STYLE_UNARMED; ch->save_time=LONG_MAX;
    pc->love=str_empty; SET_BIT(pc->flag,FLAG_PK);
    for(i=0;i<5;++i) ch->stats[i]=5;
    ch->in_room=&room;
    if(kind==CLASS_MOOGLE) { pc->crafted.parent=GEN_PARENT_MOOGLE; pc->crafted.rank=8; }
    if(kind==CLASS_WRAITH || kind==CLASS_GARGOYLE) {
        pc->lineage.parent=kind==CLASS_WRAITH ? GEN_PARENT_WRAITH : GEN_PARENT_GARGOYLE;
        pc->lineage.tracks[0]=5;
    }
}
static void purge(CHAR_DATA *ch)
{
    while(ch->affected) affect_remove(ch,ch->affected);
    if(ch->fighting) stop_fighting(ch,TRUE);
    ch->fight_timer=0; ch->position=POS_STANDING;
}
static void mitigation(void)
{
    long hp;
    setup(&a,&ap,"Actor",CLASS_NONE); setup(&b,&bp,"Target",CLASS_CYBORG);
    bp.cybernetics.body=bp.cybernetics.limbs=1;
    b.position=POS_FIGHTING; /* A connected-fight fixture, not a linkdead PC. */
    assert(cyborg_systems_valid(&bp.cybernetics));
    assert(combat_damage_amount(&a,&b,10000,TYPE_SPELL)==5714);
    hp=b.hit; legacy_damage(&a,&b,10000,TYPE_SPELL); assert(hp-b.hit==5714);
    hp=b.hit; damage(&a,&b,10000,TYPE_SPELL,"test"); assert(hp-b.hit==5714);
    assert(combat_damage_amount(&b,&b,10000,TYPE_SPELL)==10000);
    hp=b.hit; hurt_person(&b,&b,10000); assert(hp-b.hit==10000);
    purge(&a); purge(&b);
    b.class=CLASS_GARGOYLE; memset(&bp.cybernetics,0,sizeof(bp.cybernetics));
    b.position=POS_FIGHTING;
    bp.lineage.parent=GEN_PARENT_GARGOYLE; bp.lineage.tracks[0]=5;
    assert(combat_damage_amount(&a,&b,10000,TYPE_BLUNT)==7500);
    hp=b.hit; legacy_damage(&a,&b,10000,TYPE_BLUNT); assert(hp-b.hit==7500);
    hp=b.hit; damage(&a,&b,10000,TYPE_BLUNT,"test"); assert(hp-b.hit==7500);
    hp=b.hit; legacy_damage(&a,&b,10000,TYPE_SPELL); assert(hp-b.hit==10000);
    assert(combat_damage_amount(&a,&b,0,TYPE_SPELL)==0);
    purge(&a); purge(&b);
    puts("Shared mitigation: legacy/new attacks agree; no double absorption; physical/spell and raw self-harm preserved");
}
static void cooldowns(void)
{
    const char *names[]={"parent recovery","phoenix recovery","crafted recovery","lineage recovery","gift recovery"};
    const long classes[]={CLASS_ANGEL,CLASS_PHOENIX,CLASS_MOOGLE,CLASS_WRAITH,CLASS_WEREWOLF};
    AFFECT_DATA af,*saved;
    long sn,duration;
    FILE *fp;
    int i,j;
    for(i=0;i<5;++i) {
        setup(&a,&ap,"Actor",classes[i]); char_list=&a; room.people=&a;
        sn=skill_lookup(names[i]); combat_recovery_start(&a,sn,i+1);
        assert(a.affected && combat_recovery_affect(a.affected));
        duration=(i+1)*3*PULSE_PER_SECOND; assert(a.affected->duration==duration);
        current_time+=60; char_update(); assert(a.affected->duration==duration);
        combat_recovery_tick(&a); assert(a.affected->duration==duration-1);
        saved=a.affected;
        skill_table[skill_lookup("dispel magic")].spell_fun(
            skill_lookup("dispel magic"),1000,&a,&a);
        assert(is_affected(&a,sn) && saved->duration==duration-1);
        setup(&loaded,&lp,"Loaded",classes[i]);
        fp=tmpfile(); assert(fp);
        fprintf(fp,"AffectData '%s' %ld -1 0 0\nEnd\n",names[i],saved->duration);
        rewind(fp); fread_char(&loaded,fp); fclose(fp);
        assert(is_affected(&loaded,sn) && recovery(&loaded,sn)->duration==duration-1);
        assert(combat_recovery_affect(recovery(&loaded,sn)));
        combat_recovery_loaded(&loaded); assert(recovery(&loaded,sn)->duration==duration-1);
        for(j=1;j<duration-1;++j) combat_recovery_tick(&loaded);
        assert(is_affected(&loaded,sn) && recovery(&loaded,sn)->duration==1);
        combat_recovery_tick(&loaded); assert(!is_affected(&loaded,sn));
        purge(&loaded); purge(&a);
        memset(&af,0,sizeof(af)); af.type=sn; af.duration=1;
        affect_to_char(&a,&af); combat_recovery_loaded(&a);
        assert(a.affected->duration==12 && combat_recovery_affect(a.affected));
        combat_recovery_loaded(&a); assert(a.affected->duration==12);
        purge(&a);
    }
    setup(&a,&ap,"Actor",CLASS_NONE); char_list=&a;
    memset(&af,0,sizeof(af)); af.type=skill_lookup("armor");
    af.location=APPLY_AC; af.modifier=20; af.duration=2;
    affect_to_char(&a,&af);
    for(i=0;i<100;++i) combat_recovery_tick(&a);
    assert(a.affected->duration==2);
    current_time+=60; char_update(); assert(a.affected->duration==1);
    purge(&a);
    puts("Cooldowns: five families, exact 3s units, no minute double-tick, save/reload, migration, nondispellable recovery; ordinary buffs retain tick clock");
}
static void book_damage(void)
{
    OBJ_INDEX_DATA proto;
    OBJ_DATA *book;
    long raw,hp;
    int chaos;
    setup(&a,&ap,"Caster",CLASS_SORCERER); a.level=LEV_SOR;
    setup(&b,&bp,"Target",CLASS_NONE);
    char_list=&a; a.next=&b; room.people=&a; a.next_in_room=&b;
    memset(&proto,0,sizeof(proto)); proto.vnum=OBJ_VNUM_PROTOPLASM;
    proto.name=proto.short_descr=proto.description=str_empty;
    proto.item_type=ITEM_TRASH; proto.wear_flags=ITEM_TAKE;
    obj_index_hash[proto.vnum%MAX_KEY_HASH]=&proto;
    assert(sorcerer_starter_kit(&a)); book=get_obj_carry(&a,"primer"); assert(book);
    equip_char(&a,book,WEAR_HOLD); do_turn(&a,"primer 2");
    for(chaos=0;chaos<2;++chaos) {
        if(chaos) SET_BIT(book->spectype,ADV_CHAOS_BOOK);
        b.position=POS_FIGHTING;
        current_time=1792094400; init_mm();
        hp=b.hit; do_chant(&a,"Target"); raw=hp-b.hit; assert(raw>0);
        purge(&a); purge(&b); b.class=CLASS_CYBORG;
        bp.cybernetics.body=bp.cybernetics.limbs=1;
        b.position=POS_FIGHTING;
        assert(cyborg_systems_valid(&bp.cybernetics));
        current_time=1792094400; init_mm();
        hp=b.hit; do_chant(&a,"Target");
        assert(hp-b.hit==combat_damage_amount(&a,&b,raw,TYPE_SPELL));
        purge(&a); purge(&b); b.class=CLASS_NONE;
        memset(&bp.cybernetics,0,sizeof(bp.cybernetics));
    }
    obj_index_hash[proto.vnum%MAX_KEY_HASH]=NULL;
    puts("Actual normal/Chaos Book Mage chants: shared Cyborg defense applied with deterministic paired rolls");
}
int main(void)
{
    int i;
    current_time=1792094400; init_mm();
    for(i=0;i<MAX_SKILL && skill_table[i].name;++i)
        if(skill_table[i].pgsn) *skill_table[i].pgsn=i;
    memset(&room,0,sizeof(room)); memset(&area,0,sizeof(area));
    room.area=&area; room.name="balance regression"; room.light=1;
    room.vnum=99901; area.nplayer=2; weather_info.sunlight=SUN_LIGHT;
    mitigation(); cooldowns(); book_damage();
    char_list=NULL; first_fight=last_fight=NULL;
    puts("Balance foundation regression passed");
    return 0;
}
