/* Observable progression benefits, not a blanket per-variant certification. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"
#include "../src/parents.h"
extern void init_mm(void);
static CHAR_DATA ch;
static PC_DATA pc;
static ROOM_INDEX_DATA room;
typedef struct benefits {
    long hit,damage,armor,cap,soak,speed,avoid,penetrate;
    int stats[5];
} BENEFITS;
static void reset(long kind)
{
    int i;
    while(ch.affected) affect_remove(&ch,ch.affected);
    memset(&ch,0,sizeof(ch)); memset(&pc,0,sizeof(pc)); memset(&room,0,sizeof(room));
    ch.pcdata=&pc; ch.class=kind; ch.name=ch.short_descr="Tester";
    ch.clan=ch.morph=ch.lord=ch.long_descr=ch.description=str_empty;
    pc.love=str_empty;
    ch.level=LEV_AWA; ch.position=POS_STANDING;
    ch.hit=ch.max_hit=ch.mana=ch.max_mana=100000;
    ch.armor=249; ch.in_room=&room; room.people=&ch; room.name="benefit test";
    ch.style[0]=STYLE_UNARMED;
    ch.primal=100000; ch.exp=100000000;
    for(i=0;i<5;++i) ch.stats[i]=5;
    char_list=&ch;
}
static BENEFITS measure(void)
{
    BENEFITS b;
    int i;
    memset(&b,0,sizeof(b));
    b.hit=char_hitroll(&ch); b.damage=char_damroll(&ch);
    b.armor=char_ac(&ch); b.cap=get_damcap(&ch); b.soak=get_soak(&ch);
    b.speed=get_speed(&ch); b.avoid=avoidblow(&ch); b.penetrate=breakavoid(&ch);
    for(i=0;i<5;++i) b.stats[i]=char_stat(&ch,i);
    return b;
}
static void improved(BENEFITS before,const char *name)
{
    BENEFITS after=measure(),again;
    PC_DATA saved;
    int i,better=after.hit>before.hit || after.damage>before.damage
        || after.armor>before.armor || after.cap>before.cap
        || after.soak>before.soak || after.speed<before.speed || after.avoid>before.avoid
        || after.penetrate>before.penetrate;
    for(i=0;i<5;++i) {
        if(after.stats[i]>before.stats[i]) better=1;
        assert(ch.stats[i]==5); /* Never train a permanent base stat by syncing. */
    }
    if(!better) fprintf(stderr,"No observable benefit: %s\n",name);
    assert(better);
    saved=pc; newclasses_restore_passives(&ch); again=measure();
    assert(!memcmp(&after,&again,sizeof(after)) && !memcmp(&saved,&pc,sizeof(pc)));
    printf("Benefit verified: %s (real rolls/cap/soak/speed/stats; repeat sync stable)\n",name);
}
static void philodox(void)
{
    long cap;
    reset(CLASS_WEREWOLF); cap=get_damcap(&ch);
    pc.tribes[AUSPICE_PHILODOX]=3; /* Same numeric slot, WRONG skill family. */
    assert(get_damcap(&ch)==cap);
    pc.tribes[AUSPICE_PHILODOX]=0; pc.auspice[AUSPICE_PHILODOX]=2;
    assert(get_damcap(&ch)==cap);
    pc.auspice[AUSPICE_PHILODOX]=3;
    assert(get_damcap(&ch)==cap+100);
    puts("Philodox: rank 3 grants +100 cap; unrelated tribe cannot grant it");
}
static void spirit_orders(void)
{
    CHAR_DATA spirit,owner;
    PC_DATA spirit_pc;
    MOB_INDEX_DATA proto;
    char named[]="spirit sleep",all[]="all sleep";
    reset(CLASS_WEREWOLF);
    memset(&spirit,0,sizeof(spirit)); memset(&spirit_pc,0,sizeof(spirit_pc));
    memset(&owner,0,sizeof(owner)); memset(&proto,0,sizeof(proto));
    spirit.name="Spirit"; spirit.short_descr="a spirit"; spirit.clan=str_empty;
    spirit.morph=spirit.lord=spirit.long_descr=spirit.description=str_empty;
    spirit.level=1; spirit.hit=spirit.max_hit=100;
    spirit.position=POS_STANDING; spirit.in_room=&room;
    spirit.pIndexData=&proto; proto.vnum=1;
    spirit.act=ACT_IS_NPC; spirit.more=MORE_SPIRIT;
    ch.next_in_room=ch.next=&spirit;
    SET_BIT(ch.vampaff,VAM_SPIRITEYES);
    pc.auspice[AUSPICE_THEURGE]=3;
    do_order(&ch,named); assert(spirit.position==POS_STANDING);
    pc.auspice[AUSPICE_THEURGE]=4;
    do_order(&ch,named); assert(spirit.position==POS_SLEEPING);
    assert(!IS_SET(spirit.act,ACT_COMMANDED));
    spirit.position=POS_STANDING; spirit.master=&owner;
    do_order(&ch,named); assert(spirit.position==POS_STANDING);
    do_order(&ch,all); assert(spirit.position==POS_STANDING);
    spirit.master=NULL; spirit.wizard=&owner;
    do_order(&ch,named); assert(spirit.position==POS_STANDING);
    do_order(&ch,all); assert(spirit.position==POS_STANDING);
    spirit.wizard=&ch;
    do_order(&ch,all); assert(spirit.position==POS_SLEEPING);
    spirit.position=POS_STANDING; spirit.wizard=NULL; spirit.pcdata=&spirit_pc;
    spirit_pc.love=str_empty; REMOVE_BIT(spirit.act,ACT_IS_NPC);
    spirit.level=LEV_AWA;
    do_order(&ch,named); assert(spirit.position==POS_STANDING);
    do_order(&ch,all); assert(spirit.position==POS_STANDING);
    SET_BIT(spirit.act,ACT_IS_NPC); spirit.pcdata=NULL; ch.class=CLASS_NONE;
    do_order(&ch,named); assert(spirit.position==POS_STANDING);
    ch.next=ch.next_in_room=NULL;
    puts("Theurge: actual NPC order at rank 4, named/all agree; no player or foreign-owner takeover");
}
static void legacy(void)
{
    BENEFITS b;
    reset(CLASS_VAMPIRE); b=measure();
    pc.powers[DISC_POTENCE]=2; pc.powers[DISC_FORTITUDE]=2;
    pc.powers[DISC_CELERITY]=2; pc.celerity=1; improved(b,"Vampire");
    reset(CLASS_WEREWOLF); b=measure();
    pc.tribes[TRIBE_STARGAZERS]=4; pc.tribes[TRIBE_BONE_GNAWERS]=2; pc.rank=5;
    improved(b,"Werewolf");
    reset(CLASS_MAGE); b=measure(); ch.spheres[MENT]=1; ch.spheres[MTIM]=1;
    improved(b,"Mage");
    reset(CLASS_HIGHLANDER); b=measure();
    pc.highland[HIGH_ATTACK]=2; pc.highland[HIGH_DEFENSE]=2;
    improved(b,"Highlander");
    reset(CLASS_MONK); b=measure(); pc.monk[MONK_ART]=8; improved(b,"Monk");
    reset(CLASS_NINJA); b=measure();
    pc.ninja[NINJA_CHIKYU]=2; pc.ninja[NINJA_NINGENNO]=2; improved(b,"Ninja");
    reset(CLASS_DROW); b=measure(); pc.drow[DROW_SPEED]=1; improved(b,"Drow");
    reset(CLASS_DRAGON); b=measure();
    ch.vampgen=2; SET_BIT(ch.polyaff,POLY_DRAGONFORM); improved(b,"Dragon form");
    reset(CLASS_ELEM); b=measure();
    pc.powers[ELEM_FIRE]=5; pc.powers[ELEM_EARTH]=5; pc.powers[ELEM_WIND]=5;
    pc.powers[ELEM_INFIN]=8; improved(b,"Elemental");
}
static void modern(void)
{
    BENEFITS b;
    int i;
    reset(CLASS_ANGEL); b=measure(); pc.powers[0]=8;
    newclasses_restore_passives(&ch); improved(b,"Angel");
    reset(CLASS_DEMON); b=measure(); pc.powers[0]=8;
    newclasses_restore_passives(&ch); improved(b,"Demon");
    reset(CLASS_CYBORG); b=measure();
    pc.cybernetics.limbs=1; pc.cybernetics.body=1;
    newclasses_restore_passives(&ch); improved(b,"Cyborg limbs/body");
    reset(CLASS_MOOGLE); pc.crafted.parent=GEN_PARENT_MOOGLE; b=measure();
    pc.crafted.rank=8; newclasses_restore_passives(&ch); improved(b,"Moogle");
    reset(CLASS_DEATHKNIGHT); pc.crafted.parent=GEN_PARENT_DEATHKNIGHT; b=measure();
    pc.crafted.rank=8; newclasses_restore_passives(&ch); improved(b,"Deathknight");
    reset(CLASS_GARGOYLE); pc.lineage.parent=GEN_PARENT_GARGOYLE; b=measure();
    pc.lineage.tracks[0]=5; newclasses_restore_passives(&ch); improved(b,"Gargoyle");
    assert(lineage_absorb(&ch,1000,TYPE_HIT)==750);
    reset(CLASS_KUEIJIN); pc.lineage.parent=GEN_PARENT_KUEIJIN; b=measure();
    pc.lineage.dharma=5; for(i=0;i<18;++i) pc.lineage.tracks[i]=5;
    newclasses_restore_passives(&ch); improved(b,"Kuei-jin");
    reset(CLASS_WRAITH); pc.lineage.parent=GEN_PARENT_WRAITH; b=measure();
    for(i=0;i<11;++i) pc.lineage.tracks[i]=5;
    newclasses_restore_passives(&ch); improved(b,"Wraith");
    reset(CLASS_PHOENIX);
    do_phoenix(&ch,"wings"); assert(!IS_AFFECTED(&ch,AFF_FLYING) && ch.mana==ch.max_mana);
    pc.powers[0]=3; do_phoenix(&ch,"wings");
    assert(IS_AFFECTED(&ch,AFF_FLYING) && ch.mana<ch.max_mana);
    puts("Benefit verified: Phoenix rank-gated flying; actual book healing/ward covered by starter suite");
}
static void getters(void)
{
    int i,r;
    reset(CLASS_VAMPIRE);
    for(i=DISC_ANIMALISM;i<=DISC_MAX;++i) for(r=0;r<=7;++r) {
        pc.powers[i]=r; assert(get_disc(&ch,i)==r);
    }
    reset(CLASS_WEREWOLF);
    for(i=BREED_HOMID;i<=BREED_METIS;++i) for(r=0;r<=5;++r) {
        pc.breed[i]=r; assert(get_breed(&ch,i)==r);
    }
    for(i=AUSPICE_RAGABASH;i<=AUSPICE_AHROUN;++i) for(r=0;r<=5;++r) {
        pc.auspice[i]=r; assert(get_auspice(&ch,i)==r);
    }
    for(i=TRIBE_BLACK_FURIES;i<=TRIBE_WENDIGOS;++i) for(r=0;r<=5;++r) {
        pc.tribes[i]=r; assert(get_tribe(&ch,i)==r);
    }
    reset(CLASS_SORCERER);
    for(i=0;i<6;++i) for(r=0;r<=5;++r) {
        pc.colors[i]=r; assert(get_color(&ch,i)==r);
    }
    reset(CLASS_HIGHLANDER);
    for(i=0;i<4;++i) for(r=0;r<=6;++r) {
        pc.highland[i]=r; assert(get_highland(&ch,i)==r);
    }
    reset(CLASS_NINJA);
    for(i=0;i<3;++i) for(r=0;r<=10;++r) {
        pc.ninja[i]=r; assert(get_ninja(&ch,i)==r);
    }
    reset(CLASS_MONK);
    for(i=0;i<3;++i) for(r=0;r<=8;++r) {
        pc.monk[i]=r; assert(get_monk(&ch,i)==r);
    }
    puts("Legacy getters: every native rank of disciplines, gifts, colors, blademastery, principles and Monk tracks");
}
static void combat_mastery(void)
{
    int i;
    BENEFITS b;
    for(i=STANCE_VIPER;i<=STANCE_DRAGON;++i) {
        reset(CLASS_NONE); ch.stance[0]=i; b=measure();
        ch.stance[i]=200; improved(b,"stance mastery");
    }
    for(i=STYLE_UNARMED;i<=STYLE_TWOWEAPON;++i) {
        reset(CLASS_NONE); ch.style[0]=i; b=measure();
        ch.style[i]=200; improved(b,"style mastery");
    }
    puts("Every combat stance/style has a derived combat benefit; real weapon routing covered by attack-style suite");
}
int main(void)
{
    int i;
    init_mm();
    for(i=0;i<MAX_SKILL && skill_table[i].name;++i)
        if(skill_table[i].pgsn) *skill_table[i].pgsn=i;
    philodox(); spirit_orders(); legacy(); modern(); getters(); combat_mastery(); reset(CLASS_NONE);
    puts("Class-benefit regression passed; full individual ability/variant certification remains separate");
    return 0;
}
