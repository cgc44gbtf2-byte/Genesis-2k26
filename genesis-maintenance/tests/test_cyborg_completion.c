/* Actual Cyborg effects and failure atomicity, complementary to component,
 * reference-equipment, state-codec and initial adapter suites. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"
#include "../src/parents.h"
extern void init_mm(void);
static void input(CHAR_DATA *ch,const char *s)
{ char b[MAX_INPUT_LENGTH]; assert(strlen(s)<sizeof(b)); strcpy(b,s); do_cyborg(ch,b); }
static void ready(CHAR_DATA *ch)
{
    if(ch->fighting) stop_fighting(ch,TRUE);
    ch->fight_timer=0; ch->position=POS_STANDING; ch->wait=0;
    affect_strip(ch,gsn_parent_recovery);
}
static void setup(CHAR_DATA *ch,PC_DATA *pc,char *name)
{
    int i;
    memset(ch,0,sizeof(*ch)); memset(pc,0,sizeof(*pc)); ch->pcdata=pc;
    ch->name=ch->short_descr=name;
    ch->morph=ch->description=ch->long_descr=str_empty;
    ch->level=LEV_AWA; ch->position=POS_STANDING;
    ch->hit=ch->max_hit=1000000; ch->mana=ch->max_mana=1000000;
    ch->primal=10000; ch->style[0]=STYLE_UNARMED;
    for(i=0;i<5;++i) ch->stats[i]=5;
}
int main(void)
{
    CHAR_DATA ch,v,ally;
    PC_DATA pc,vpc,apc;
    ROOM_INDEX_DATA room,remote;
    AREA_DATA area;
    MOB_INDEX_DATA mob;
    DESCRIPTOR_DATA desc;
    char out[65536],buf[120];
    CYBORG_SYSTEMS *s;
    long mana,nano,hp,primal,hr,dr,cost,base;
    int i,n,sn;
    const char *attacks[]={"taser","snipe","launch","detonate","stuntubes","web"};
    init_mm();
    for(i=0;i<MAX_SKILL && skill_table[i].name;++i)
        if(skill_table[i].pgsn) *skill_table[i].pgsn=i;
    setup(&ch,&pc,"Actor"); setup(&v,&vpc,"Enemy"); setup(&ally,&apc,"Ally");
    memset(&room,0,sizeof(room)); memset(&remote,0,sizeof(remote)); memset(&area,0,sizeof(area));
    room.area=remote.area=&area; area.nplayer=2; room.vnum=777; remote.vnum=778;
    room.name="home field"; remote.name="distant field";
    room.sector_type=remote.sector_type=SECT_FIELD; weather_info.sunlight=SUN_LIGHT;
    char_to_room(&ch,&room); char_to_room(&v,&room); char_to_room(&ally,&remote);
    ch.next=&v; v.next=&ally; char_list=&ch; ally.leader=&ch;
    memset(&mob,0,sizeof(mob)); mob.vnum=1234; mob.alignment=-500;
    v.act=ACT_IS_NPC; v.pcdata=NULL; v.level=20; v.pIndexData=&mob;
    memset(&desc,0,sizeof(desc)); memset(out,0,sizeof(out));
    desc.outbuf=out; desc.outsize=sizeof(out); ch.desc=&desc;
    ch.class=CLASS_CYBORG;
    assert(cyborg_core_init(&pc.cyborg,CY_COMPONENT_MASK,1000)==CY_OK);
    for(i=0;i<CY_COMPONENT_COUNT;++i) {
        snprintf(buf,sizeof(buf),"install %s",cyborg_component_at(i)->key);
        input(&ch,buf); assert(cyborg_has_component(&ch,i));
    }
    pc.cyber.rifle=12; pc.cyber.nanites=10000;
    s=&pc.cybernetics; s->face=5; s->limbs=5; s->body=6;
    newclasses_loaded(&ch); base=char_ac(&ch);
    assert(base>0 && cyborg_melee_damage(&ch,1000)==1950);
    assert(cyborg_absorb_damage(&ch,700)==400);
    ch.weapon[TYPE_SLASH]=200; improve_weapon(&ch,TYPE_SLASH);
    assert(ch.weapon[TYPE_SLASH]==200);
    s->limbs=0; s->body=0; s->face=0; /* remove authority without a class swap */
    assert(cyborg_melee_damage(&ch,1000)==1000 && cyborg_absorb_damage(&ch,700)==700);
    improve_weapon(&ch,TYPE_SLASH); assert(ch.weapon[TYPE_SLASH]==100);
    s->limbs=5; s->body=6; s->face=5;
    for(i=0;i<12;++i) {
        primal=ch.primal; cost=15*(i+1); input(&ch,"demolitions improve");
        assert(s->demolition==i+1 && ch.primal==primal-cost);
    }
    primal=ch.primal; input(&ch,"demolitions improve"); assert(ch.primal==primal);
    ready(&ch); mana=ch.mana; input(&ch,"scry Ally");
    assert(ch.in_room==&room && ch.mana==mana-500 && strstr(out,"distant field"));
    ready(&ch); mana=ch.mana; input(&ch,"read_aura Enemy");
    assert(ch.mana==mana-100 && strstr(out,"alignment is -500"));
    ready(&ch); mana=ch.mana; input(&ch,"read_aura Actor"); assert(ch.mana==mana);
    ready(&ch); mana=ch.mana; nano=pc.cyber.nanites;
    SET_BIT(remote.room_flags,ROOM_PRIVATE); input(&ch,"transmit Ally");
    assert(ch.in_room==&room && ch.mana==mana && pc.cyber.nanites==nano);
    REMOVE_BIT(remote.room_flags,ROOM_PRIVATE);
    SET_BIT(ch.affected_by,AFF_WEBBED); input(&ch,"transmit Ally");
    assert(ch.in_room==&room && ch.mana==mana && pc.cyber.nanites==nano);
    REMOVE_BIT(ch.affected_by,AFF_WEBBED); input(&ch,"transmit Ally");
    assert(ch.in_room==&remote && ch.mana==mana-1500 && pc.cyber.nanites==nano-500);
    char_from_room(&ch); char_to_room(&ch,&room);
    ready(&ch); hr=ch.hitroll; dr=ch.damroll;
    input(&ch,"terminator"); assert(ch.hitroll==hr+35 && ch.damroll==dr+35);
    affect_strip(&ch,skill_lookup("cyber avatar"));
    assert(ch.hitroll==hr && ch.damroll==dr);
    ready(&ch); input(&ch,"lloth"); assert(ch.hitroll==hr+15 && ch.damroll==dr+15);
    affect_strip(&ch,skill_lookup("cyber blessing")); assert(ch.hitroll==hr && ch.damroll==dr);
    ready(&ch); input(&ch,"upload"); assert(is_affected(&ch,gsn_parent_cloak));
    affect_strip(&ch,gsn_parent_cloak);
    ready(&ch); input(&ch,"mechsight");
    assert(newclasses_darkvision(&ch));
    affect_strip(&ch,skill_lookup("cyber sight")); assert(!newclasses_darkvision(&ch));
    ready(&ch); input(&ch,"darkness"); assert(newclasses_room_dark(&room));
    affect_strip(&ch,skill_lookup("cyber darkness")); assert(!newclasses_room_dark(&room));
    /* Saving throws may resist: verify an actual successful web/poison,
     * bounded retries, real costs on every attempt and named removal. */
    for(n=0;n<50 && !IS_AFFECTED(&v,AFF_WEBBED);++n) {
        ready(&ch); mana=ch.mana; input(&ch,"web Enemy"); assert(ch.mana==mana-500);
    }
    assert(IS_AFFECTED(&v,AFF_WEBBED));
    affect_strip(&v,skill_lookup("web")); assert(!IS_AFFECTED(&v,AFF_WEBBED));
    for(n=0;n<50 && !IS_AFFECTED(&v,AFF_POISON);++n) {
        ready(&ch); pc.cyber.nanites=10000; hp=v.hit;
        input(&ch,"oilspit Enemy"); assert(v.hit<hp && pc.cyber.nanites==9000);
    }
    assert(IS_AFFECTED(&v,AFF_POISON)); affect_strip(&v,gsn_poison);
    ready(&ch); pc.cyber.nanites=10000; s->bombs=20;
    input(&ch,"make_bomb"); assert(s->bombs==20 && pc.cyber.nanites==10000);
    s->bombs=1; input(&ch,"arm"); assert(s->armed);
    input(&ch,"arm off"); assert(!s->armed && s->bombs==1);
    input(&ch,"mine"); assert(s->bombs==1 && !is_affected(&ch,skill_lookup("cyber mine")));
    input(&ch,"arm"); input(&ch,"mine"); assert(!s->bombs);
    assert(is_affected(&ch,skill_lookup("cyber mine")));
    SET_BIT(remote.room_flags,ROOM_SAFE);
    char_from_room(&ch); char_to_room(&ch,&remote); cyborg_update(&ch);
    assert(!is_affected(&ch,skill_lookup("cyber mine")));
    REMOVE_BIT(remote.room_flags,ROOM_SAFE);
    char_from_room(&ch); char_to_room(&ch,&room);
    /* Each weapon must reject a grouped NPC, even though NPC combat is legal. */
    v.leader=&ch;
    for(i=0;i<6;++i) {
        ready(&ch); s->bombs=1; s->armed=1; nano=pc.cyber.nanites;
        mana=ch.mana; hp=v.hit; snprintf(buf,sizeof(buf),"%s Enemy",attacks[i]);
        input(&ch,buf);
        assert(v.hit==hp && ch.mana==mana && pc.cyber.nanites==nano && s->bombs==1 && s->armed);
    }
    v.leader=NULL; ready(&ch); s->armed=0;
    /* Radio must own its whole message, not reference_command's three tokens. */
    desc.outtop=0; out[0]=0; input(&ch,"radio one two three four five %s $n");
    assert(strstr(out,"one two three four five %s $n"));
    mana=ch.mana; nano=pc.cyber.nanites;
    SET_BIT(ch.more,MORE_HEAD); input(&ch,"convert 1"); input(&ch,"radio blocked");
    assert(ch.mana==mana && pc.cyber.nanites==nano);
    REMOVE_BIT(ch.more,MORE_HEAD);
    s->bombs=1; s->armed=1;
    ready(&ch); hp=v.hit; nano=pc.cyber.nanites;
    /* Missing required legs cannot consume launch currency or hurt a target. */
    assert(cyborg_core_uninstall(&pc.cyborg,CY_LEFT_LEG)==CY_OK);
    input(&ch,"launch Enemy"); assert(v.hit==hp && pc.cyber.nanites==nano);
    /* Every inventory row is now backed by tested Genesis code, not a
     * declaration that arbitrary upstream behaviour was copied verbatim. */
    assert(genesis_parent_power_count(GEN_PARENT_CYBORG)==60);
    for(i=0;i<60;++i) assert(genesis_parent_power_at(GEN_PARENT_CYBORG,i)->implemented);
    assert(!genesis_parent_ready(GEN_PARENT_CYBORG));
    sn=skill_lookup("cyber mine"); assert(sn>=0);
    puts("Cyborg completion: training/caps, real sensing/transport, aliases, web/poison, mine locality, explosive bounds, grouped-target safety, radio, incapacitation and all 60 catalogue rows passed");
    return 0;
}
