/* Real menu quotes versus real training deductions, without a live server. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"
extern void init_mm(void);

static CHAR_DATA ch;
static PC_DATA pc;
static ROOM_INDEX_DATA room;
static DESCRIPTOR_DATA desc;
static char output[65536];

static void reset(long class_id)
{
    int i;
    while(ch.affected) affect_remove(&ch,ch.affected);
    memset(&ch,0,sizeof(ch)); memset(&pc,0,sizeof(pc));
    memset(&room,0,sizeof(room)); memset(&desc,0,sizeof(desc));
    ch.pcdata=&pc; ch.class=class_id; ch.name=ch.short_descr="Tester";
    ch.clan=ch.morph=ch.description=ch.long_descr=str_empty;
    ch.level=LEV_AWA; ch.position=POS_STANDING;
    ch.hit=ch.max_hit=ch.mana=ch.max_mana=100000;
    ch.exp=100000000; ch.primal=100000;
    for(i=0;i<5;++i) ch.stats[i]=5;
    for(i=0;i<3;++i) { pc.cland[i]=-1; pc.tgift[i]=-1; }
    room.name="training test"; room.people=&ch; ch.in_room=&room;
    desc.outbuf=output; desc.outsize=sizeof(output); ch.desc=&desc;
    output[0]='\0';
}

static void run(DO_FUN *fn, const char *text)
{
    char input[128];
    strcpy(input,text); desc.outtop=0; output[0]='\0';
    fn(&ch,input);
}

static void readonly_command(DO_FUN *fn, const char *command)
{
    CHAR_DATA before=ch;
    PC_DATA saved=pc;
    const char *s;
    size_t width=0;
    run(fn,command);
    assert(!memcmp(&ch,&before,sizeof(ch)));
    assert(!memcmp(&pc,&saved,sizeof(pc)));
    for(s=output;*s;++s) {
        if((unsigned char)*s==27 && s[1]=='[') {
            s+=2; while(*s && !(*s>='@' && *s<='~')) ++s;
            if(!*s) break;
        } else if(*s=='\n') { assert(width<=79); width=0; }
        else if(*s!='\r') ++width;
    }
    assert(width<=79 && strstr(output,"----"));
}

static void readonly_menu(DO_FUN *fn)
{
    readonly_command(fn,"");
}

static void quote(const char *name,long rank,long cap,long cost,
                  const char *currency,const char *status)
{
    char ranks[32],price[32],expected[128];
    snprintf(ranks,sizeof(ranks),"%li/%li",rank,cap);
    if(rank>=cap) strcpy(price,"--");
    else snprintf(price,sizeof(price),"%li",cost);
    snprintf(expected,sizeof(expected),"%-20.20s %8.8s %12.12s %-8.8s %-16.16s",
        name,ranks,price,currency,status);
    if(!strstr(output,expected)) fprintf(stderr,"Missing quote: %s\n%s\n",expected,output);
    assert(strstr(output,expected));
}

static void linear_training(void)
{
    static const struct {
        long class_id,cap,unit;
        DO_FUN *fn;
        const char *command,*name;
        size_t offset;
    } cases[]={
        {CLASS_NINJA,10,10,do_principles,"sora improve","Sora",
            offsetof(PC_DATA,ninja)+NINJA_SORA*sizeof(long)},
        {CLASS_MONK,8,10,do_monklearn,"mantra gain","Mantras",
            offsetof(PC_DATA,monk)+MONK_MANTRA*sizeof(long)},
        {CLASS_HIGHLANDER,6,15,do_increase,"attack","Attack",
            offsetof(PC_DATA,highland)+HIGH_ATTACK*sizeof(long)},
        {CLASS_SORCERER,5,10,do_colors,"purple","Purple",
            offsetof(PC_DATA,colors)+PURPLE_MAGIC*sizeof(long)},
        {CLASS_ELEM,8,10,do_element,"fire improve","Fire",
            offsetof(PC_DATA,powers)+ELEM_FIRE*sizeof(long)}
    };
    size_t i;
    long rank,cost,*track;
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i)
        for(rank=0;rank<=cases[i].cap;++rank) {
            reset(cases[i].class_id);
            track=(long *)((char *)&pc+cases[i].offset); *track=rank;
            cost=(rank+1)*cases[i].unit; ch.primal=cost;
            readonly_menu(cases[i].fn);
            quote(cases[i].name,rank,cases[i].cap,cost,"primal",
                rank==cases[i].cap ? "MAXED" : "Affordable");
            run(cases[i].fn,cases[i].command);
            assert(*track==(rank==cases[i].cap ? rank : rank+1));
            assert(ch.primal==(rank==cases[i].cap ? cost : 0));
            if(rank<cases[i].cap) {
                *track=rank; ch.primal=cost-1;
                readonly_menu(cases[i].fn);
                quote(cases[i].name,rank,cases[i].cap,cost,"primal","Needs funds");
                run(cases[i].fn,cases[i].command);
                assert(*track==rank && ch.primal==cost-1);
            }
        }
    puts("Five legacy training tracks: every level, exact quotes/deductions, caps and one-short rejection passed");
}

static void clan_prices(void)
{
    int variant;
    long rank,cap,cost,unit;
    for(variant=0;variant<3;++variant) {
        cap=variant==0 ? 5 : variant==1 ? 7 : 6;
        unit=variant==0 ? 15 : 5;
        for(rank=-1;rank<=cap;++rank) {
            if(rank==0) continue;
            reset(CLASS_VAMPIRE);
            if(variant==1) pc.cland[0]=DISC_ANIMALISM;
            if(variant==2) ch.clan="Caitiff";
            pc.powers[DISC_ANIMALISM]=rank;
            cost=rank==-1 ? unit : (rank+1)*unit;
            ch.primal=cost; readonly_menu(do_discipline);
            quote("Animalism",rank,cap,cost,"primal",rank==cap ? "MAXED" : "Affordable");
            run(do_discipline,"animalism");
            assert(pc.powers[DISC_ANIMALISM]==(rank==cap ? rank : rank<1 ? 1 : rank+1));
            assert(ch.primal==(rank==cap ? cost : 0));
        }
    }
    for(variant=0;variant<2;++variant) {
        cap=variant ? 5 : 4; unit=variant ? 10 : 15;
        for(rank=0;rank<=cap;++rank) {
            reset(CLASS_WEREWOLF);
            if(variant) pc.tgift[0]=BREED_HOMID;
            pc.breed[BREED_HOMID]=rank; cost=(rank+1)*unit; ch.primal=cost;
            readonly_menu(do_gifts);
            quote(variant ? "HOMID" : "Homid",rank,cap,cost,"primal",
                rank==cap ? "MAXED" : "Affordable");
            run(do_gifts,"homid");
            assert(pc.breed[BREED_HOMID]==(rank==cap ? rank : rank+1));
            assert(ch.primal==(rank==cap ? cost : 0));
        }
    }
    reset(CLASS_WEREWOLF); readonly_menu(do_powers);
    assert(!strstr(output,"Huh?"));
    puts("Vampire clan/Caitiff and Werewolf selected-gift prices/caps match actual purchases; Werewolf dispatch passed");
}

static void mage_prices(void)
{
    long rank,cost,cap;
    int favoured;
    for(favoured=0;favoured<2;++favoured) {
        cap=favoured ? 5 : 4;
        for(rank=0;rank<=cap;++rank) {
            reset(CLASS_MAGE);
            if(favoured) pc.cland[0]=MCOR;
            ch.spheres[MCOR]=rank; cost=10*(rank+1); ch.primal=cost;
            readonly_menu(do_spheres);
            quote("Correspondence",rank,cap,cost,"primal",rank==cap ? "MAXED" : "Affordable");
            run(do_spheres,"correspondence learn");
            assert(ch.spheres[MCOR]==(rank==cap ? rank : rank+1));
            assert(ch.primal==(rank==cap ? cost : 0));
        }
    }
    for(rank=0;rank<=10;++rank) {
        reset(CLASS_MAGE); ch.arete=rank; cost=500000*(rank+1); ch.exp=cost;
        readonly_menu(do_spheres);
        quote("Arete",rank,10,cost,"XP",rank==10 ? "MAXED" : "Affordable");
        run(do_arete,"");
        assert(ch.arete==(rank==10 ? rank : rank+1));
        assert(ch.exp==(rank==10 ? cost : 0));
    }
    puts("Mage favoured/unfavoured spheres and all Arete prices match actual deductions");
}

static void dragon_prices(void)
{
    static const long dp[]={2500,5000,7500,10000,15000,25000,35000,50000,70000,100000};
    static const long qp[]={0,0,0,0,500,1000,1500,2000,2500,3000};
    int i;
    char price[80];
    for(i=0;i<10;++i) {
        reset(CLASS_DRAGON); ch.vampgen=AGE_HATCHLING-i;
        pc.classpoints[CP_CURRENT]=dp[i]; pc.quest=qp[i];
        readonly_menu(do_dragonskill);
        snprintf(price,sizeof(price),"%li DP + %li QP | Affordable",dp[i],qp[i]);
        assert(strstr(output,price));
        run(do_dragonskill,"age");
        assert(ch.vampgen==AGE_HATCHLING-i-1 && !pc.classpoints[CP_CURRENT] && !pc.quest);
        reset(CLASS_DRAGON); ch.vampgen=AGE_HATCHLING-i;
        pc.classpoints[CP_CURRENT]=dp[i]-1; pc.quest=qp[i];
        run(do_dragonskill,"age");
        assert(ch.vampgen==AGE_HATCHLING-i && pc.classpoints[CP_CURRENT]==dp[i]-1 && pc.quest==qp[i]);
        if(qp[i]) {
            pc.classpoints[CP_CURRENT]=dp[i]; pc.quest=qp[i]-1;
            run(do_dragonskill,"age");
            assert(ch.vampgen==AGE_HATCHLING-i && pc.classpoints[CP_CURRENT]==dp[i] && pc.quest==qp[i]-1);
        }
    }
    reset(CLASS_DRAGON); ch.vampgen=AGE_VYOUNG; pc.classpoints[CP_CURRENT]=1000;
    readonly_menu(do_dragonskill); assert(strstr(output,"1000 DP | Affordable"));
    run(do_dragonskill,"fangs"); assert(IS_DRAGAFF(&ch,DRAG_FANGS) && !pc.classpoints[CP_CURRENT]);
    readonly_menu(do_dragonskill); assert(strstr(output,"-- | Learned"));
    puts("Dragon: every age quote, dual-currency exact deductions, either-currency-short rejection and learned skills passed");
}

static void native_prices(void)
{
    static const struct {
        const char *class_name,*label,*command;
        DO_FUN *menu;
        long cap,unit;
    } cases[]={
        {"angel","angel train","train",do_angel,8,50000},
        {"demon","demon train","train",do_demon,8,50000},
        {"cyborg","train (rifle)","train",do_cyborg,12,10000},
        {"phoenix","phoenix train","train",do_phoenix,8,50000},
        {"moogle","train","train",do_moogle,8,50000},
        {"deathknight","train","train",do_deathknight,8,50000},
        {"gargoyle","visceratika","train visceratika",do_gargoyle,5,25000},
        {"kueijin","yin","train yin",do_kueijin,5,25000},
        {"wraith","argos","train argos",do_wraith,5,25000}
    };
    size_t i;
    long rank,cost,*track;
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i)
        for(rank=0;rank<=cases[i].cap;++rank) {
            reset(CLASS_NONE); run(do_autoclass,cases[i].class_name);
            assert(ch.class!=CLASS_NONE);
            if(i==2) track=&pc.cyber.rifle;
            else if(i==4 || i==5) track=&pc.crafted.rank;
            else if(i>=6) track=&pc.lineage.tracks[0];
            else track=&pc.powers[0];
            *track=rank;
            if(i==7) pc.lineage.dharma=5;
            cost=cases[i].unit*(rank+1)*(rank+1); ch.exp=cost;
            readonly_menu(cases[i].menu);
            quote(cases[i].label,rank,cases[i].cap,cost,"XP",
                rank==cases[i].cap ? "MAXED" : "Affordable");
            if(rank<cases[i].cap) {
                run(cases[i].menu,cases[i].command);
                assert(*track==rank+1 && ch.exp==0);
            }
        }
    reset(CLASS_NONE); run(do_autoclass,"cyborg");
    pc.cybernetics.face=4; pc.cybernetics.body=3; pc.cybernetics.limbs=1;
    pc.cybernetics.bionic=200000;
    readonly_menu(do_cyborg);
    quote("face_implants",4,5,200000,"bionics","Need body 4");
    run(do_cyborg,"face_implants improve");
    assert(pc.cybernetics.face==4 && pc.cybernetics.bionic==200000);
    pc.cybernetics.body=4;
    readonly_menu(do_cyborg);
    quote("face_implants",4,5,200000,"bionics","Affordable");
    run(do_cyborg,"face_implants improve");
    assert(pc.cybernetics.face==5 && !pc.cybernetics.bionic);
    reset(CLASS_NONE); run(do_autoclass,"kueijin");
    pc.lineage.tracks[0]=2;
    readonly_menu(do_kueijin); quote("yin",2,5,225000,"XP","Need Dharma");
    quote("dharma train",0,5,100000,"XP","Affordable");
    ch.exp=100000; run(do_kueijin,"dharma train");
    assert(pc.lineage.dharma==1 && !ch.exp);
    puts("Nine native classes: every mastery quote/XP deduction, caps, Cyborg bionic/body gates and Dharma training passed");
}

static void demon_purchase_prices(void)
{
    reset(CLASS_NONE); run(do_autoclass,"demon"); pc.powers[0]=8;
    ch.primal=49;
    readonly_command(do_demon,"gifts");
    quote("strength",0,1,50,"primal","Needs funds");
    run(do_demon,"gifts strength");
    assert(!pc.legacy.gifts && ch.primal==49);
    ch.primal=50;
    readonly_command(do_demon,"gifts");
    quote("strength",0,1,50,"primal","Affordable");
    run(do_demon,"gifts strength");
    assert((pc.legacy.gifts & 1U) && !ch.primal);
    readonly_command(do_demon,"gifts");
    quote("strength",1,1,50,"primal","MAXED");
    ch.exp=15000; run(do_demon,"souls 15000");
    assert(pc.legacy.souls==15000 && !ch.exp);
    readonly_command(do_demon,"warps");
    quote("obtain",0,10,15000,"souls","Affordable");
    assert(strstr(output,"10000 souls; requires an owned negative warp"));
    run(do_demon,"obtain");
    assert(!pc.legacy.souls && pc.legacy.warps);
    readonly_command(do_demon,"warps");
    quote("obtain",1,10,15000,"souls","Needs funds");
    assert(strstr(output,"20000 souls total; removes ALL warps; no refund"));
    puts("Demon: gift/warp read-only quotes, exact primal/soul payments, one-short rejection and 1:1 XP conversion passed");
}

int main(void)
{
    int i;
    init_mm();
    for(i=0;i<MAX_SKILL && skill_table[i].name;++i)
        if(skill_table[i].pgsn) *skill_table[i].pgsn=i;
    linear_training(); clan_prices(); mage_prices(); dragon_prices(); native_prices();
    demon_purchase_prices();
    reset(CLASS_NONE);
    return 0;
}
