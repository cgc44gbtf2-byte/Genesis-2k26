/* Read-only class-menu output, navigation, eligibility and width checks. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"

extern void init_mm(void);

static void clear_output(DESCRIPTOR_DATA *d)
{
    d->outtop = 0; d->outbuf[0] = '\0';
}

static void bounded_lines(const char *text)
{
    size_t width = 0;
    const char *start = text;
    for (; *text; ++text) {
        if ((unsigned char)*text == 27 && text[1] == '[') {
            text += 2;
            while (*text && !(*text >= '@' && *text <= '~')) ++text;
            if (!*text) break;
            continue;
        }
        if (*text == '\n') {
            if (width > 79)
                fprintf(stderr, "Menu line width %lu: %.*s\n",
                    (unsigned long)width, (int)(text-start), start);
            assert(width <= 79); width = 0; start = text+1;
        } else if (*text != '\r') ++width;
    }
    assert(width <= 79);
}

int main(void)
{
    CHAR_DATA ch;
    PC_DATA pc, before;
    ROOM_INDEX_DATA room;
    DESCRIPTOR_DATA desc;
    char output[65536], command[MAX_INPUT_LENGTH];
    const char *classes[] = {"cyborg","angel","demon","phoenix","moogle",
                            "deathknight","gargoyle","kueijin","wraith"};
    DO_FUN *menus[] = {do_cyborg,do_angel,do_demon,do_phoenix,do_moogle,
                      do_deathknight,do_gargoyle,do_kueijin,do_wraith};
    const char *sections[] = {"","status","sys_status","systems","help",
                             "implants","powers","all"};
    size_t i,j;
    long primal, exp, mana;
    init_mm();
    for (i=0;i<MAX_SKILL && skill_table[i].name;++i)
        if(skill_table[i].pgsn) *skill_table[i].pgsn=i;
    memset(&room,0,sizeof(room)); room.name="menu room";
    for (i=0;i<sizeof(classes)/sizeof(classes[0]);++i) {
        memset(&ch,0,sizeof(ch)); memset(&pc,0,sizeof(pc));
        memset(&desc,0,sizeof(desc)); memset(output,0,sizeof(output));
        ch.pcdata=&pc; ch.name=ch.short_descr="Tester";
        ch.morph=ch.description=ch.long_descr=str_empty;
        ch.in_room=&room; room.people=&ch;
        ch.level=LEV_AWA; ch.position=POS_STANDING;
        ch.hit=ch.max_hit=ch.mana=ch.max_mana=100000;
        ch.primal=10000; ch.exp=1000000;
        for(j=0;j<5;++j) ch.stats[j]=5;
        desc.outbuf=output; desc.outsize=sizeof(output); ch.desc=&desc;
        strcpy(command,classes[i]); do_autoclass(&ch,command);
        assert(ch.class!=CLASS_NONE);
        clear_output(&desc);
        before=pc; primal=ch.primal; exp=ch.exp; mana=ch.mana;
        menus[i](&ch,"");
        assert(strstr(output,"----")!=NULL);
        bounded_lines(output);
        assert(!memcmp(&pc,&before,sizeof(pc)));
        assert(ch.primal==primal && ch.exp==exp && ch.mana==mana);
        if(i==0) {
            assert(strstr(output,"Overview") && strstr(output,"cyborg implants"));
            assert(strstr(output,"railgun")==NULL && strstr(output,"terminator")==NULL);
            for(j=0;j<sizeof(sections)/sizeof(sections[0]);++j) {
                clear_output(&desc); strcpy(command,sections[j]); do_cyborg(&ch,command);
                assert(strstr(output,"CYBORG |")!=NULL);
                bounded_lines(output);
                assert(!memcmp(&pc,&before,sizeof(pc)));
                assert(ch.primal==primal && ch.exp==exp && ch.mana==mana);
            }
            clear_output(&desc); do_cyborg(&ch,"implants");
            assert(strstr(output,"battery") && strstr(output,"Eligible"));
            assert(strstr(output,"Needs parts") && strstr(output,"railgun"));
            clear_output(&desc); do_cyborg(&ch,"install battery");
            assert(strstr(output,"modification complete"));
            assert(cyborg_has_component(&ch,CY_BATTERY));
            assert(ch.primal==primal-30 && pc.cyborg.humanity==95);
            clear_output(&desc); do_cyborg(&ch,"implants");
            assert(strstr(output,"Installed"));
            ch.level=LEV_ST;
            clear_output(&desc); do_cyborg(&ch,"install heart");
            assert(strstr(output,"Staff characters cannot"));
            assert(!cyborg_has_component(&ch,CY_HEART));
            assert(ch.primal==primal-30);
            clear_output(&desc); do_cyborg(&ch,"status");
            assert(strstr(output,"Staff rank blocks"));
            bounded_lines(output);
            ch.level=LEV_AWA;
            pc.cyber.rifle=12;
            pc.cybernetics.limbs=5; pc.cybernetics.body=6;
            pc.cybernetics.face=5; pc.cybernetics.demolition=12;
            clear_output(&desc); do_cyborg(&ch,"powers");
            assert(strstr(output,"Passive") && strstr(output,"Known"));
            assert(strstr(output,"armor_plating") && strstr(output,"regeneration"));
            bounded_lines(output);
        }
        while(ch.affected) affect_remove(&ch,ch.affected);
    }
    puts("Nine class menus: aligned/read-only tables and 79-column output passed");
    puts("Cyborg sections, implant states, staff feedback and real installation passed");
    return 0;
}
