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
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
extern OBJ_DATA *get_page(OBJ_DATA *,long);
extern long chainspell;
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
static void class_armor(void)
{
    static const char *keys[]={"ring","collar","armor","helmet","leggings",
        "boots","gloves","sleeves","cloak","belt","bracer","mask"};
    static const int slots[]={WEAR_FINGER_L,WEAR_NECK_1,WEAR_BODY,WEAR_HEAD,
        WEAR_LEGS,WEAR_FEET,WEAR_HANDS,WEAR_ARMS,WEAR_ABOUT,WEAR_WAIST,
        WEAR_WRIST_L,WEAR_FACE};
    CHAR_DATA *ch=&fighter[0];
    OBJ_DATA *old,*created;
    char input[100];
    int i;
    long primal;
    /* Build a pre-owned loadout before the timed buff-preparation phase.
     * Fabrication uses the real handler/template/cost, not edited item stats.
     * Duplicate restrictions leave the second ring/neck/wrist as benchmarks.
     */
    for(i=0;i<12;++i) {
        primal=ch->primal;
        snprintf(input,sizeof(input),"cyborg cyborgeq %s",keys[i]);
        command(0,input); created=ch->carrying;
        assert(ch->primal==primal-100 && created->item_type==ITEM_ARMOR);
        assert(created->value[0]==20 && !strcmp(created->questowner,ch->name));
        old=get_eq_char(ch,slots[i]); assert(old);
        unequip_char(ch,old); obj_from_char(old); equip_char(ch,created,slots[i]);
        assert(get_eq_char(ch,slots[i])==created);
        /* Fixture construction is outside combat; no recovery is bypassed
         * after timed preparation begins. */
        ch->wait=0;
    }
}
static void book_loadout(void)
{
    CHAR_DATA *ch=&fighter[1];
    OBJ_DATA *old,*book,*pen,*page;
    char input[200];
    int p;
    /* Build and bind actual scripts with the public crafting commands.
     * This is pre-battle fixture construction, not a combat cooldown reset. */
    ch->level=LEV_SOR; ch->style[0]=STYLE_ONEHAND;
    for(p=0;p<6;++p) player[1].colors[p]=5;
    command(1,"sorccreate book"); book=get_obj_carry(ch,"book"); assert(book);
    command(1,"sorccreate pen"); pen=get_obj_carry(ch,"pen"); assert(pen);
    old=get_eq_char(ch,WEAR_HOLD); assert(old);
    unequip_char(ch,old); obj_from_char(old); equip_char(ch,pen,WEAR_HOLD);
    for(p=1;p<=21;++p) {
        command(1,"sorccreate page"); page=get_obj_carry(ch,"page"); assert(page);
        snprintf(input,sizeof(input),"write page title %s %d",p==1 ? "War Ward" : p<=11 ? "Red Assault" : "Blue Mend",p);
        command(1,input);
        command(1,p==1 ? "write page line start.affect.spell" : "write page line start.damage.spell");
        if(p==1) {
            command(1,"write page line apply: armor");
            command(1,"write page line apply: hitroll");
            command(1,"write page line apply: damroll");
            command(1,"write page line affect: sanct");
            command(1,"write page line duration: 50");
        } else if(p!=11 && p!=21) command(1,"write page line next.page");
        command(1,"write page line message.1: runic magic");
        command(1,"write page line message.2: You release runic magic toward $N.");
        command(1,"write page line message.3: $n releases runic magic toward $N.");
        command(1,p==1 ? "write page line end.green" : p<=11 ? "write page line end.red" : "write page line end.blue");
        assert(IS_SET(page->spectype,ADV_FINISHED) && !IS_SET(page->spectype,ADV_FAILED));
        command(1,"cast 'insert page' page book");
        assert(book->value[3]==p && get_page(book,p)==page);
        assert(page->specpower==p+1);
        ch->wait=0;
    }
    unequip_char(ch,pen); equip_char(ch,book,WEAR_HOLD);
    assert(book->item_type==ITEM_BOOK && !IS_SET(book->spectype,ADV_CHAOS_BOOK));
    assert(!IS_SET(get_page(book,11)->spectype,ADV_NEXT_PAGE));
    assert(!IS_SET(get_page(book,21)->spectype,ADV_NEXT_PAGE));
    /* Arrive with full mana, as the other pre-owned fixture builds do.
     * No mana refill occurs during timed buff preparation or combat. */
    ch->mana=ch->max_mana; ch->wait=0;
}
int main(int argc,char **argv)
{
    int i,pulse,opener=0,order,elapsed,demon=0,bookmage=0,use_armor=0,healing,funded,page_cost;
    long seed=1792094400;
    char *end,*opponent_key="wraith",*opponent_name="Wraith";
    char attack[100],kill[100],action[100];
    static const int warps[]={0,1,2,5,7,8,9,11,13,23};
    long before[2],spent[2]={0,0},ward_ac,ward_hr,ward_dr;
    if(argc>5) { fprintf(stderr,"Usage: duel [positive-seed] [cyborg|opponent] [wraith|demon|bookmage] [class-armor]\n"); return 2; }
    if(argc>=4) {
        if(!strcmp(argv[3],"demon")) { demon=1; opponent_key="demon"; opponent_name="Demon"; }
        else if(!strcmp(argv[3],"bookmage")) { bookmage=1; opponent_key="bookmage"; opponent_name="Bookmage"; }
        else if(strcmp(argv[3],"wraith")) { fprintf(stderr,"Invalid opponent.\n"); return 2; }
    }
    if(argc==5) {
        if(strcmp(argv[4],"class-armor")) { fprintf(stderr,"Invalid equipment mode.\n"); return 2; }
        use_armor=1;
    }
    if(argc>=2) {
        errno=0; seed=strtol(argv[1],&end,10);
        if(errno || end==argv[1] || *end || seed<=0 || seed>INT_MAX) {
            fprintf(stderr,"Invalid positive seed.\n"); return 2;
        }
    }
    if(argc>=3) {
        if(!strcmp(argv[2],opponent_key)) opener=1;
        else if(strcmp(argv[2],"cyborg")) { fprintf(stderr,"Invalid opener.\n"); return 2; }
    }
    current_time=seed; init_mm();
    for(i=0;i<MAX_SKILL && skill_table[i].name;++i)
        if(skill_table[i].pgsn) *skill_table[i].pgsn=i;
    memset(&arena,0,sizeof(arena)); memset(&area,0,sizeof(area));
    arena.area=&area; arena.name="isolated benchmark arena"; arena.sector_type=SECT_FIELD;
    area.name=area.filename="benchmark"; arena.vnum=777; area.nplayer=2;
    weather_info.sunlight=SUN_LIGHT; prototype.vnum=12345;
    if(use_armor || bookmage) {
        /* kavir.are #30037: neutral protoplasm, no affects/extra flags. */
        prototype.vnum=OBJ_VNUM_PROTOPLASM;
        prototype.name=prototype.short_descr=prototype.description=str_empty;
        prototype.item_type=ITEM_TRASH; prototype.wear_flags=ITEM_TAKE;
        obj_index_hash[prototype.vnum%MAX_KEY_HASH]=&prototype;
    }
    setup(0,CLASS_CYBORG,"Cyborg");
    setup(1,demon ? CLASS_DEMON : bookmage ? CLASS_SORCERER : CLASS_WRAITH,opponent_name);
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
    if(demon) {
        player[1].powers[0]=8; player[1].legacy.gifts=REF_GIFT_MASK;
        player[1].legacy.souls=REF_SOUL_CAP;
        for(i=0;i<10;++i) player[1].legacy.warps|=UINT32_C(1)<<warps[i];
        assert(reference_legacy_valid(&player[1].legacy));
        newclasses_restore_passives(&fighter[1]);
    } else if(!bookmage) {
        player[1].lineage.parent=GEN_PARENT_WRAITH; player[1].lineage.energy=10000;
        for(i=0;i<lineage_track_count(GEN_PARENT_WRAITH);++i) player[1].lineage.tracks[i]=5;
        assert(lineage_state_valid(&player[1].lineage)); lineage_restore(&fighter[1]);
    }
    newclasses_restore_passives(&fighter[0]);
    if(use_armor) class_armor();
    if(bookmage) book_loadout();
    printf("GENESIS: 2k26 | CYBORG vs %s | ENGINE-LINKED FIGHT SNAPSHOT\n",opponent_name);
    printf("Seed/current_time: %ld. Isolated in-memory PCs; no live-host changes.\n",seed);
    puts("Both: PK enabled, no staff flags; base HP/mana caps 100000; class-specific stat caps; all weapon/stance/style mastery 200.");
    puts("Active configuration: Tiger stance, blades (180..220), benchmark armor 15 in slots without chosen class gear.");
    if(bookmage) puts("Bookmage must hold the book: one blade plus book, legal one-hand style; Cyborg retains dual blades.");
    else puts("Both fighters use dual blades and two-weapon style.");
    if(use_armor) puts("Cyborg wears all 12 distinct fabricated Cyborgeq armor types (armor20); duplicate restrictions leave three benchmark slots. Fabrication costs paid.");
    puts("Benchmark gear is controlled/equal, NOT a claim of the strongest obtainable world equipment.");
    puts("Cyborg: rifle12 face5 limbs5 body6 demolition12, full starting nanites/bombs; 17 implants at 100/100 humanity spent.");
    puts("Implants omitted by the legal budget: both legs, ears, BoostMaster, wolvers, chameleon, radiosplice.");
    if(demon) {
        puts("Demon: mastery8, all five purchased gifts, full soul pool; legal maximum ten combat-focused warps.");
        puts("Warps: chitin, scales, strongarms, spiketail, quickness, stamina, regeneration, damcap, horns, shards.");
        puts("No standalone wearable Demon armor fabrication exists; Demon Armour is a timed +30 AC ward over benchmark gear.");
        puts("Preparation: Terminator and Demon Armour, then two real minute ticks to clear recovery; no free resource refill.");
        puts("Combat rotation: Cyborg snipe or repair below 75% HP; Demon netherflames or leech below 75% HP.");
    } else if(bookmage) {
        puts("Bookmage: normal (non-Chaos) Sorcerer; all six colors5;21 pages crafted/bound through public commands.");
        puts("Ward: green50tick duration, +50AC/+50HR/+50DR and Sanctuary; red10-page damage chain, blue10-page healing chain.");
        puts("Preparation: Terminator and War Ward, then two real minute ticks; no mana refills after preparation starts.");
        puts("Combat: Cyborg snipe/repair below75%; Bookmage red chain, or blue healing chain below75%.");
    } else {
        puts("Wraith: all 11 Arcanoi rank5, full starting pathos. Utility/travel/minion powers are not forced into a duel.");
        puts("Preparation: Terminator and Bodyshape, then two real minute ticks to clear their recovery; no free resource refill.");
        puts("Combat rotation: Cyborg snipe or repair below 75% HP; Wraith requiem or charitable below 75% HP.");
    }
    printf("Commands only attempted when wait/recovery allow them. Real violence/character updates; opener/command priority: %s.\n",
        fighter[opener].name);
    command(0,"cyborg terminator");
    if(bookmage) {
        ward_ac=fighter[1].armor; ward_hr=fighter[1].hitroll; ward_dr=fighter[1].damroll;
        command(1,"turn book 1"); command(1,"chant self");
        assert(chainspell==1 && IS_AFFECTED(&fighter[1],AFF_SANCTUARY));
        assert(fighter[1].armor==ward_ac+50 && fighter[1].hitroll==ward_hr+50
            && fighter[1].damroll==ward_dr+50);
    } else command(1,demon ? "demon demonarmour" : "wraith bodyshape");
    tick(); tick();
    discard(0); discard(1); stats(0); stats(1);
    if(demon) assert(is_affected(&fighter[1],skill_lookup("parent ward")));
    if(bookmage) assert(IS_AFFECTED(&fighter[1],AFF_SANCTUARY)
        && is_affected(&fighter[1],skill_lookup("green sorcery")));
    assert(!is_safe(&fighter[0],&fighter[1]));
    snprintf(kill,sizeof(kill),"kill %s",opener ? "Cyborg" : opponent_name);
    command(opener,kill);
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
            snprintf(attack,sizeof(attack),"cyborg snipe %s",opponent_name);
            snprintf(action,sizeof(action),"%s %s",opponent_key,
                demon ? (ch->hit<ch->max_hit*3/4 ? "leech Cyborg" : "netherflames Cyborg")
                : (ch->hit<ch->max_hit*3/4 ? "charitable" : "requiem Cyborg"));
            if(i==1 && bookmage) {
                healing=ch->hit<ch->max_hit*3/4;
                /* The engine charges per page and permits partial chains.
                 * Do not artificially reserve otherwise usable late mana. */
                page_cost=healing ? 400 : 600;
                if(ch->mana<page_cost) continue;
                funded=UMIN(10,ch->mana/page_cost);
                command(1,healing ? "turn book 12" : "turn book 2");
                command(1,healing ? "chant self" : "chant Cyborg");
                /* A short chain attempts its next page, then stops on the
                 * native insufficient-mana check without charging it. */
                assert(chainspell==UMIN(10,funded+1));
                assert(before[i]-ch->mana==page_cost*funded);
            } else command(i,i ? action : (ch->hit<ch->max_hit*3/4 ? "cyborg repair" : attack));
            if(ch->mana+(i ? player[i].lineage.energy : player[i].cyber.nanites)<before[i]) ++spent[i];
        }
        if(fighter[0].hit<=0 || fighter[1].hit<=0) break;
        violence_update();
        if(descriptor[0].outtop) { printf("\n[PULSE %d | %.2f seconds]\n",pulse,(double)pulse/PULSE_PER_SECOND); view(0); }
        discard(1);
        if(pulse%20==0 || fighter[0].hit<=0 || fighter[1].hit<=0)
            printf("[STATUS] Cyborg HP %ld | %s HP %ld\n",fighter[0].hit,opponent_name,fighter[1].hit);
        if(!fighter[0].fighting && !fighter[1].fighting) break;
    }
    puts("\n--- RESULT ---"); stats(0); stats(1);
    elapsed=UMIN(pulse,2400);
    printf("Simulated time %.2f seconds; successful resource-spending combat actions: Cyborg %ld, %s %ld.\n",
        (double)elapsed/PULSE_PER_SECOND,spent[0],opponent_name,spent[1]);
    if(fighter[0].hit<=0) printf("Outcome: Cyborg incapacitated; %s remains standing.\n",opponent_name);
    else if(fighter[1].hit<=0) printf("Outcome: %s incapacitated; Cyborg remains standing.\n",opponent_name);
    else puts("Outcome: No incapacitation within this bounded run (or native combat stopped).");
    puts("One controlled rotation/seed is not class balance proof; no ability was invented for this transcript.");
    assert(fighter[0].hit<100000 && fighter[1].hit<100000);
    assert(spent[0]>0 && spent[1]>0);
    printf("DUEL_RESULT seed=%ld opener=%s cyborg_hp=%ld %s_hp=%ld pulses=%d cyborg_actions=%ld %s_actions=%ld\n",
        seed,opener ? opponent_key : "cyborg",fighter[0].hit,opponent_key,fighter[1].hit,elapsed,spent[0],opponent_key,spent[1]);
    stop_fighting(&fighter[0],TRUE); return 0;
}
