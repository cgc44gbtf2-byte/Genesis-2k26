#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"
extern void init_mm(void);
static CHAR_DATA staff,target;
static PC_DATA sp,tp;
static ROOM_INDEX_DATA room;
static DESCRIPTOR_DATA sd;
static char output[65536];

static void reset(void)
{
    int i;
    while(target.affected) affect_remove(&target,target.affected);
    while(staff.affected) affect_remove(&staff,staff.affected);
    memset(&staff,0,sizeof(staff)); memset(&target,0,sizeof(target));
    memset(&sp,0,sizeof(sp)); memset(&tp,0,sizeof(tp));
    memset(&room,0,sizeof(room)); memset(&sd,0,sizeof(sd));
    staff.pcdata=&sp; target.pcdata=&tp;
    staff.name=staff.short_descr="Operator"; target.name=target.short_descr="Mortal";
    staff.clan=target.clan=staff.morph=target.morph=str_empty;
    staff.level=LEV_ST; target.level=LEV_AWA;
    staff.position=target.position=POS_STANDING;
    staff.in_room=target.in_room=&room; room.people=&staff; room.name="test";
    staff.next=staff.next_in_room=&target; char_list=&staff;
    staff.hit=staff.max_hit=target.hit=target.max_hit=100000;
    staff.mana=staff.max_mana=target.mana=target.max_mana=100000;
    target.primal=10000; target.exp=100000000;
    target.blood[BLOOD_POOL]=100; tp.willpower[1]=50;
    for(i=0;i<5;++i) target.stats[i]=5;
    sd.outbuf=output; sd.outsize=sizeof(output); staff.desc=&sd;
}
static void run(const char *text)
{
    char input[2048];
    strcpy(input,text); sd.outtop=0; output[0]=0;
    do_pset(&staff,input);
}
static void denied(const char *text)
{
    CHAR_DATA saved=target;
    PC_DATA pc=tp;
    run(text);
    if(memcmp(&saved,&target,sizeof(target)) || memcmp(&pc,&tp,sizeof(tp)))
        fprintf(stderr,"Unexpected edit: %s\n",text);
    assert(!memcmp(&saved,&target,sizeof(target)) && !memcmp(&pc,&tp,sizeof(tp)));
}
static void menu_width(void)
{
    const char *p=output;
    size_t column=0;
    while(*p) {
        if(*p=='\033' && p[1]=='[') {
            while(*p && *p!='m') ++p;
            if(*p) ++p;
        } else if(*p=='\n' || *p=='\r') {
            assert(column<=79); column=0; ++p;
        } else { ++column; ++p; }
    }
    assert(column<=79);
}
static void set(const char *name,long value)
{
    char command[128];
    snprintf(command,sizeof(command),"Mortal %s %li",name,value); run(command);
    if(!strstr(output,"Normal save/autosave")) fprintf(stderr,"%s: %s\n",command,output);
    assert(strstr(output,"Normal save/autosave"));
}
static void classed(const char *name)
{
    char input[32];
    reset(); strcpy(input,name); do_autoclass(&target,input);
    assert(target.class!=CLASS_NONE);
}
int main(void)
{
    static const char *bad[]={"-1","+2","1.5","1e3","abc","999999999999999999999999999",
        "12 4","12 4 7","1x",""};
    static const char *names[]={"moogle","deathknight","gargoyle","kueijin","wraith"};
    static const char *energy_alias[]={"pomenergy","soulenergy","stoneenergy","chi","pathos"};
    size_t i;
    char text[2048];
    long parsed=0;
    CYBORG_CORE core;
    init_mm();
    for(i=0;i<MAX_SKILL && skill_table[i].name;++i)
        if(skill_table[i].pgsn) *skill_table[i].pgsn=i;
    assert(pset_parse_amount("1000000000",&parsed) && parsed==1000000000);
    assert(!pset_parse_amount(NULL,&parsed));
    reset();
    denied("Mortal currencies"); assert(strstr(output,"PSET CURRENCIES")); menu_width();
    denied("Mortal currencies 5");
    set("xp",900000000); assert(target.exp==900000000);
    set("experience",100); assert(target.exp==100);
    set("primal",100000); assert(target.primal==100000);
    set("gold",1000000000); assert(target.gold==1000000000);
    set("classpoints",10000000); assert(tp.classpoints[CP_CURRENT]==10000000);
    assert(!tp.classpoints[CP_TOTAL]);
    set("cps",12); set("dp",13); set("favor",14);
    assert(tp.classpoints[CP_CURRENT]==14);
    set("cptotal",123); assert(tp.classpoints[CP_TOTAL]==123 && tp.classpoints[CP_CURRENT]==14);
    set("quest",1000000); assert(tp.quest==1000000);
    set("qp",25); assert(tp.quest==25);
    set("mana",100000); assert(target.mana==100000);
    set("hp",1); assert(target.hit==1 && target.position==POS_STANDING);
    set("blood",100); assert(target.blood[BLOOD_CURRENT]==100);
    set("willpower",50); assert(tp.willpower[0]==50);
    denied("Mortal wallet"); menu_width();
    for(i=0;i<sizeof(bad)/sizeof(bad[0]);++i) {
        snprintf(text,sizeof(text),"Mortal exp %s",bad[i]); denied(text);
    }
    denied("Mortal exp 1000000001"); denied("Mortal cps 10000001");
    denied("Mortal quest 1000001"); denied("Mortal hp 0");
    denied("Mortal hp 100001"); denied("Mortal mana 100001");
    denied("Mortal blood 101"); denied("Mortal willpower 51");
    denied("Mortal bionics 100"); denied("Mortal ki 20");
    memset(text,'9',sizeof(text)-1); text[sizeof(text)-1]=0; denied(text);
    staff.level=LEV_AWA; denied("Mortal exp 1000");
    staff.level=LEV_IMM; target.level=LEV_IMM; denied("Mortal exp 1000");
    staff.level=LEV_ST; target.level=LEV_ST; denied("Mortal exp 1000");
    target.level=LEV_AWA; sd.original=&target; denied("Mortal exp 1000"); sd.original=NULL;
    SET_BIT(staff.act,ACT_IS_NPC); denied("Mortal exp 1000"); REMOVE_BIT(staff.act,ACT_IS_NPC);
    SET_BIT(target.act,ACT_IS_NPC); denied("Mortal exp 1000"); REMOVE_BIT(target.act,ACT_IS_NPC);
    run("Operator xp 25"); assert(staff.exp==25);
    puts("Pset legacy balances, aliases, bounds, strict parsing, queries, role/NPC/switched protections passed");
    classed("ninja"); set("ki",300); assert(tp.ninja[NINJA_KI]==300); denied("Mortal ki 301");
    classed("mage hermes"); set("quint",10000); assert(target.quint[BODY]==10000);
    denied("Mortal quint 10001");
    classed("demon"); set("soulpower",REF_SOUL_CAP); assert(tp.legacy.souls==REF_SOUL_CAP);
    denied("Mortal souls 1000001");
    for(i=0;i<sizeof(names)/sizeof(names[0]);++i) {
        classed(names[i]); set("energy",10000); set(energy_alias[i],321);
        assert((i<2 ? tp.crafted.energy : tp.lineage.energy)==321);
        denied("Mortal energy 10001");
        if(i!=3) denied("Mortal chi 50");
        denied("Mortal currencies"); menu_width();
    }
    classed("cyborg");
    denied("Mortal nanites 1234"); set("nanites",0);
    do_cyborg(&target,"install battery"); assert(cyborg_has_component(&target,CY_BATTERY));
    set("nanites",10000); set("bionic",10000000); set("bombs",20);
    assert(tp.cyber.nanites==10000 && tp.cybernetics.bionic==10000000 && tp.cybernetics.bombs==20);
    denied("Mortal nanites 10001"); denied("Mortal bionics 10000001"); denied("Mortal bombs 21");
    tp.cybernetics.armed=1; denied("Mortal bombs 0");
    tp.cybernetics.armed=0; set("bombs",0);
    core=tp.cyborg;
    set("humanity",150); assert(tp.cyborg.humanity==150 && cyborg_core_valid(&tp.cyborg));
    assert(tp.cyborg.installed==core.installed && tp.cyborg.permitted==core.permitted);
    assert(tp.cyborg.humanity_limit==150+core.humanity_limit-core.humanity);
    set("humanitycap",200); assert(tp.cyborg.humanity_limit==200 && cyborg_core_valid(&tp.cyborg));
    denied("Mortal humanitycap 0"); denied("Mortal humanitycap 1001");
    denied("Mortal humanity 1000");
    set("humanity",0); assert(tp.cyborg.humanity==0 && cyborg_core_valid(&tp.cyborg));
    assert(tp.cyborg.installed==core.installed);
    tp.cyber.nanites=10001; denied("Mortal nanites 100");
    denied("Mortal currencies"); menu_width();
    puts("Pset class resources, native caps, class gates, armed-bomb protection and implant-budget invariants passed");
    reset(); return 0;
}
