/* Read-only, terminal-width regression for all ten older parent classes. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"
extern void init_mm(void);

static void bounded(const char *s)
{
    size_t width=0;
    for(;*s;++s) {
        if((unsigned char)*s==27 && s[1]=='[') {
            s+=2; while(*s && !(*s>='@' && *s<='~')) ++s;
            if(!*s) break;
        } else if(*s=='\n') { assert(width<=79); width=0; }
        else if(*s!='\r') ++width;
    }
    assert(width<=79);
}

int main(void)
{
    static const struct {
        long class_id;
        DO_FUN *handler;
        const char *argument;
    } probes[]={
        {CLASS_VAMPIRE,do_discipline,""},{CLASS_VAMPIRE,do_powers,""},
        {CLASS_WEREWOLF,do_powers,""},{CLASS_WEREWOLF,do_gifts,""},
        {CLASS_WEREWOLF,do_tribegifts,""},
        {CLASS_MAGE,do_spheres,""},{CLASS_MAGE,do_magepowers,""},
        {CLASS_DRAGON,do_dragonskill,""},
        {CLASS_DROW,do_prayer,""},{CLASS_DROW,do_drowupkeep,""},
        {CLASS_ELEM,do_element,""},{CLASS_ELEM,do_element,"fire"},
        {CLASS_ELEM,do_element,"water"},{CLASS_ELEM,do_element,"earth"},
        {CLASS_ELEM,do_element,"wind"},{CLASS_ELEM,do_element,"sun"},
        {CLASS_ELEM,do_element,"moon"},{CLASS_ELEM,do_element,"time"},
        {CLASS_ELEM,do_element,"infinity"},
        {CLASS_HIGHLANDER,do_increase,""},
        {CLASS_MONK,do_monklearn,""},{CLASS_MONK,do_monklearn,"mantra"},
        {CLASS_MONK,do_monklearn,"art"},{CLASS_MONK,do_monklearn,"tech"},
        {CLASS_NINJA,do_principles,""},{CLASS_NINJA,do_principles,"sora"},
        {CLASS_NINJA,do_principles,"chikyu"},{CLASS_NINJA,do_principles,"ningenno"},
        {CLASS_SORCERER,do_colors,""}
    };
    CHAR_DATA ch,before_ch;
    PC_DATA pc,before;
    DESCRIPTOR_DATA desc;
    ROOM_INDEX_DATA room;
    char output[65536],argument[128];
    size_t i,j;
    int mode;
    init_mm();
    for(i=0;i<MAX_SKILL && skill_table[i].name;++i)
        if(skill_table[i].pgsn) *skill_table[i].pgsn=i;
    memset(&room,0,sizeof(room)); room.name="menu audit";
    for(mode=0;mode<4;++mode)
        for(i=0;i<sizeof(probes)/sizeof(probes[0]);++i) {
            memset(&ch,0,sizeof(ch)); memset(&pc,0,sizeof(pc));
            memset(&desc,0,sizeof(desc)); memset(output,0,sizeof(output));
            ch.pcdata=&pc; ch.name=ch.short_descr="Tester";
            ch.morph=ch.description=ch.long_descr=ch.clan=str_empty;
            ch.class=probes[i].class_id; ch.level=LEV_AWA;
            ch.position=POS_STANDING; ch.in_room=&room; room.people=&ch;
            ch.mana=ch.max_mana=100000; ch.hit=ch.max_hit=100000;
            ch.primal=mode ? 10000 : 0; ch.exp=mode ? 100000000 : 0;
            ch.vampgen=mode ? AGE_ANCIENT : AGE_HATCHLING;
            for(j=0;j<sizeof(pc.powers)/sizeof(pc.powers[0]);++j) pc.powers[j]=mode ? 7 : 0;
            for(j=0;j<sizeof(pc.breed)/sizeof(pc.breed[0]);++j) pc.breed[j]=mode ? 5 : 0;
            for(j=0;j<sizeof(pc.auspice)/sizeof(pc.auspice[0]);++j) pc.auspice[j]=mode ? 5 : 0;
            for(j=0;j<sizeof(pc.tribes)/sizeof(pc.tribes[0]);++j) pc.tribes[j]=mode ? 5 : 0;
            for(j=0;j<sizeof(ch.spheres)/sizeof(ch.spheres[0]);++j) ch.spheres[j]=mode ? 5 : 0;
            for(j=0;j<3;++j) {
                pc.ninja[j]=mode ? 10 : 0; pc.monk[j]=mode ? 8 : 0;
                pc.tgift[j]=(int)j<mode ? 0 : -1;
                pc.cland[j]=-1;
            }
            for(j=0;j<4;++j) pc.highland[j]=mode ? 6 : 0;
            for(j=0;j<6;++j) pc.colors[j]=mode ? 5 : 0;
            if(ch.class==CLASS_ELEM) {
                memset(pc.powers,0,sizeof(pc.powers));
                for(j=ELEM_FIRE;j<=ELEM_INFIN;++j) pc.powers[j]=mode ? 8 : 0;
                if(mode==3) {
                    pc.powers[ELEM_CHOICE]=ELEM_FIRE;
                    pc.powers[ELEM_FIRE]=10; pc.powers[ELEM_WATER]=6;
                }
            }
            pc.drow[DROW_POWERS]=mode ? DROW_MAX*2-1 : 0;
            desc.outbuf=output; desc.outsize=sizeof(output); ch.desc=&desc;
            before=pc; before_ch=ch; strcpy(argument,probes[i].argument);
            probes[i].handler(&ch,argument);
            assert(!memcmp(&before,&pc,sizeof(pc)));
            assert(!memcmp(&before_ch,&ch,sizeof(ch)));
            if(strstr(output,"Huh?")) fprintf(stderr,"Bad dispatch: class %li\n",ch.class);
            assert(!strstr(output,"Huh?"));
            bounded(output);
        }
    printf("Legacy menus passed: %lu read-only, <=79-column views across ten classes\n",
        (unsigned long)(4*sizeof(probes)/sizeof(probes[0])));
    return 0;
}
