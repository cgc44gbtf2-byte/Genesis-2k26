/* Reproducible, engine-linked combat experiment, NOT a balance certification.
 * Run with: CRYPT_LIBRARY=... sh tests/test-combat-system.sh duel
 * No live server, player file or production world is used.
 */
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/merc.h"
#include "../src/parents.h"
extern void init_mm(void);
extern void char_update(void);
static CHAR_DATA fighter[2];
static PC_DATA player[2];
static DESCRIPTOR_DATA descriptor[2];
static char output[2][262144];
static OBJ_DATA gear[2][MAX_WEAR];
static OBJ_INDEX_DATA prototype;
static ROOM_INDEX_DATA arena;
static AREA_DATA area;
static void discard(int id) { descriptor[id].outtop=0; output[id][0]='\0'; }
static void view(int id)
{
    size_t i,n=(size_t)descriptor[id].outtop;
    assert(n<sizeof(output[id]));
    for(i=0;i<n;++i) {
        if(output[id][i]==27 && i+1<n && output[id][i+1]=='[') {
            i+=2; while(i<n && !(output[id][i]>='@' && output[id][i]<='~')) ++i;
        } else if(output[id][i]!='\r') putchar(output[id][i]);
    }
    discard(id);
}
static void command(int id,const char *text)
{
    char input[MAX_INPUT_LENGTH];
    assert(strlen(text)<sizeof(input)); strcpy(input,text);
    printf("\n[%s command] %s\n",fighter[id].name,text);
    interpret(&fighter[id],input);
    view(id);
}
static void setup(int id,long kind,char *name)
{
    CHAR_DATA *ch=&fighter[id];
    PC_DATA *pc=&player[id];
    int i,slot;
    clear_char(ch); memset(pc,0,sizeof(*pc)); ch->pcdata=pc;
    ch->name=ch->short_descr=name; ch->class=kind; ch->level=LEV_AWA;
    pc->pwd=pc->bamfin=pc->bamfout=pc->cdest=pc->title=pc->decap_char=str_empty;
    pc->marriage=pc->email=pc->conding=pc->love=pc->covenant=pc->cov_time=str_empty;
    pc->leader=pc->vleader=str_empty;
    SET_BIT(pc->flag,FLAG_PK);
    ch->contingency=str_empty;
    ch->hit=ch->max_hit=ch->mana=ch->max_mana=100000;
    ch->primal=10000; ch->exp=1000000000;
    ch->save_time=LONG_MAX; ch->armor=0;
    for(i=0;i<5;++i) ch->stats[i]=stat_max(ch,i);
    for(i=0;i<(int)(sizeof(ch->weapon)/sizeof(ch->weapon[0]));++i) ch->weapon[i]=200;
    for(i=0;i<(int)(sizeof(ch->stance)/sizeof(ch->stance[0]));++i) ch->stance[i]=200;
    for(i=0;i<(int)(sizeof(ch->style)/sizeof(ch->style[0]));++i) ch->style[i]=200;
    ch->stance[0]=STANCE_TIGER; ch->style[0]=STYLE_TWOWEAPON;
    descriptor[id].outbuf=output[id]; descriptor[id].outsize=sizeof(output[id]);
    descriptor[id].character=ch; descriptor[id].connected=CON_PLAYING; ch->desc=&descriptor[id];
    char_to_room(ch,&arena);
    for(slot=WEAR_FINGER_L;slot<MAX_WEAR;++slot) {
        OBJ_DATA *obj=&gear[id][slot];
        memset(obj,0,sizeof(*obj)); obj->pIndexData=&prototype; obj->name="benchmark";
        obj->short_descr="a matched benchmark armor piece";
        obj->description=str_empty; obj->wear_loc=-1;
        obj->item_type=ITEM_ARMOR; obj->value[0]=15; obj->weight=1;
        if(slot==WEAR_WEAPON || slot==WEAR_HOLD) {
            obj->name="benchmark blade"; obj->short_descr="a matched benchmark blade";
            obj->item_type=ITEM_WEAPON; obj->value[1]=180; obj->value[2]=220;
            obj->value[3]=TYPE_SLASH;
        }
        obj_to_char(obj,ch); equip_char(ch,obj,slot);
    }
}
static void tick(void)
{
    /* Real engine affect expiry, regeneration and Cyborg factory updates. */
    current_time+=60; char_update();
    fighter[0].wait=fighter[1].wait=0;
}
static void stats(int id)
{
    CHAR_DATA *ch=&fighter[id];
    int i;
    printf("%s: HP %ld/%ld; mana %ld/%ld; base stats",ch->name,ch->hit,ch->max_hit,ch->mana,ch->max_mana);
    for(i=0;i<5;++i) printf(" %ld",ch->stats[i]);
    printf("; derived HR %ld DR %ld AC %ld cap %ld soak %ld speed %ld\n",
        char_hitroll(ch),char_damroll(ch),char_ac(ch),get_damcap(ch),get_soak(ch),get_speed(ch));
}
int main(int argc,char **argv)
{
    int i,pulse,opener=0,order,elapsed;
    long seed=1792094400;
    char *end;
    long before[2],spent[2]={0,0};
    if(argc>3) { fprintf(stderr,"Usage: duel [positive-seed] [cyborg|wraith]\n"); return 2; }
    if(argc>=2) {
        errno=0; seed=strtol(argv[1],&end,10);
        if(errno || end==argv[1] || *end || seed<=0 || seed>INT_MAX) {
            fprintf(stderr,"Invalid positive seed.\n"); return 2;
        }
    }
    if(argc==3) {
        if(!strcmp(argv[2],"wraith")) opener=1;
        else if(strcmp(argv[2],"cyborg")) { fprintf(stderr,"Invalid opener.\n"); return 2; }
    }
    current_time=seed; init_mm();
    for(i=0;i<MAX_SKILL && skill_table[i].name;++i)
        if(skill_table[i].pgsn) *skill_table[i].pgsn=i;
    memset(&arena,0,sizeof(arena)); memset(&area,0,sizeof(area));
    arena.area=&area; arena.name="isolated benchmark arena"; arena.sector_type=SECT_FIELD;
    area.name=area.filename="benchmark"; arena.vnum=777; area.nplayer=2;
    weather_info.sunlight=SUN_LIGHT; prototype.vnum=12345;
    setup(0,CLASS_CYBORG,"Cyborg"); setup(1,CLASS_WRAITH,"Wraith");
    fighter[opener].next=&fighter[1-opener]; char_list=&fighter[opener];
    assert(cyborg_core_init(&player[0].cyborg,CY_COMPONENT_MASK,100)==CY_OK);
    for(i=0;i<CY_COMPONENT_COUNT;++i) {
        if(i==CY_LEFT_LEG || i==CY_RIGHT_LEG || i==CY_EARS || i==CY_BOOSTMASTER
            || i==CY_WOLVERS || i==CY_CHAMELEON || i==CY_RADIOSPLICE) continue;
        assert(cyborg_core_install(&player[0].cyborg,i,&fighter[0].primal)==CY_OK);
    }
    assert(player[0].cyborg.humanity==0 && cyborg_core_valid(&player[0].cyborg));
    player[0].cyber.rifle=12; player[0].cyber.nanites=10000; player[0].cyber.factory=1;
    player[0].cybernetics.face=player[0].cybernetics.limbs=5;
    player[0].cybernetics.body=6; player[0].cybernetics.demolition=12;
    player[0].cybernetics.bombs=20; player[0].cybernetics.armed=1;
    player[1].lineage.parent=GEN_PARENT_WRAITH; player[1].lineage.energy=10000;
    for(i=0;i<lineage_track_count(GEN_PARENT_WRAITH);++i) player[1].lineage.tracks[i]=5;
    assert(lineage_state_valid(&player[1].lineage)); newclasses_restore_passives(&fighter[0]);
    lineage_restore(&fighter[1]);
    puts("GENESIS: 2k26 | CYBORG vs WRAITH | ENGINE-LINKED FIGHT SNAPSHOT");
    printf("Seed/current_time: %ld. Isolated in-memory PCs; no live-host changes.\n",seed);
    puts("Both: PK enabled, no staff flags; base HP/mana caps 100000; class-specific stat caps; all weapon/stance/style mastery 200.");
    puts("Active configuration: Tiger stance, dual blades (180..220), matched armor (15 per eligible slot).");
    puts("Benchmark gear is controlled/equal, NOT a claim of the strongest obtainable world equipment.");
    puts("Cyborg: rifle12 face5 limbs5 body6 demolition12, full starting nanites/bombs; 17 implants at 100/100 humanity spent.");
    puts("Implants omitted by the legal budget: both legs, ears, BoostMaster, wolvers, chameleon, radiosplice.");
    puts("Wraith: all 11 Arcanoi rank5, full starting pathos. Utility/travel/minion powers are not forced into a duel.");
    puts("Preparation: Terminator and Bodyshape, then two real minute ticks to clear their recovery; no free resource refill.");
    puts("Combat rotation: Cyborg snipe or repair below 75% HP; Wraith requiem or charitable below 75% HP.");
    printf("Commands only attempted when wait/recovery allow them. Real violence/character updates; opener/command priority: %s.\n",
        fighter[opener].name);
    command(0,"cyborg terminator"); command(1,"wraith bodyshape"); tick(); tick();
    discard(0); discard(1); stats(0); stats(1);
    assert(!is_safe(&fighter[0],&fighter[1]));
    command(opener,opener ? "kill Cyborg" : "kill Wraith");
    assert(fighter[0].fighting==&fighter[1]);
    for(pulse=1;pulse<=2400 && fighter[0].hit>0 && fighter[1].hit>0;++pulse) {
        if(pulse%PULSE_TICK==0) tick();
        for(order=0;order<2;++order) {
            CHAR_DATA *ch;
            i=(opener+order)%2; ch=&fighter[i];
            if(ch->wait>0) --ch->wait;
            if(ch->hit<=0 || fighter[1-i].hit<=0 || ch->wait>0
                || is_affected(ch,gsn_parent_recovery)
                || (i==1 && is_affected(ch,skill_lookup("lineage recovery")))) continue;
            before[i]=ch->mana+(i ? player[i].lineage.energy : player[i].cyber.nanites);
            command(i,i ? (ch->hit<ch->max_hit*3/4 ? "wraith charitable" : "wraith requiem Cyborg")
                : (ch->hit<ch->max_hit*3/4 ? "cyborg repair" : "cyborg snipe Wraith"));
            if(ch->mana+(i ? player[i].lineage.energy : player[i].cyber.nanites)<before[i]) ++spent[i];
        }
        if(fighter[0].hit<=0 || fighter[1].hit<=0) break;
        violence_update();
        if(descriptor[0].outtop) { printf("\n[PULSE %d | %.2f seconds]\n",pulse,(double)pulse/PULSE_PER_SECOND); view(0); }
        discard(1);
        if(pulse%20==0 || fighter[0].hit<=0 || fighter[1].hit<=0)
            printf("[STATUS] Cyborg HP %ld | Wraith HP %ld\n",fighter[0].hit,fighter[1].hit);
        if(!fighter[0].fighting && !fighter[1].fighting) break;
    }
    puts("\n--- RESULT ---"); stats(0); stats(1);
    elapsed=UMIN(pulse,2400);
    printf("Simulated time %.2f seconds; successful resource-spending combat actions: Cyborg %ld, Wraith %ld.\n",
        (double)elapsed/PULSE_PER_SECOND,spent[0],spent[1]);
    printf("Outcome: %s\n",fighter[0].hit<=0 ? "Cyborg incapacitated; Wraith remains standing."
        : fighter[1].hit<=0 ? "Wraith incapacitated; Cyborg remains standing."
        : "No incapacitation within this bounded run (or native combat stopped).");
    puts("One controlled rotation/seed is not class balance proof; no ability was invented for this transcript.");
    assert(fighter[0].hit<100000 && fighter[1].hit<100000);
    assert(spent[0]>0 && spent[1]>0);
    printf("DUEL_RESULT seed=%ld opener=%s cyborg_hp=%ld wraith_hp=%ld pulses=%d cyborg_actions=%ld wraith_actions=%ld\n",
        seed,opener ? "wraith" : "cyborg",fighter[0].hit,fighter[1].hit,elapsed,spent[0],spent[1]);
    stop_fighting(&fighter[0],TRUE); return 0;
}
