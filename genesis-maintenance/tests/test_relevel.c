#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"
int main(void)
{
    CHAR_DATA ch,original;
    PC_DATA pc;
    DESCRIPTOR_DATA desc;
    char output[4096];
    const char *denied[]={"Akuma","Chaos","Dude","Shane","Corruption",
        "Ad","Administrator","Adminx","Admin Other","Other Admin","",NULL};
    const char *allowed[]={"Admin","admin","ADMIN","aDmIn"};
    int i;
    memset(&ch,0,sizeof(ch)); memset(&pc,0,sizeof(pc)); memset(&desc,0,sizeof(desc));
    memset(&original,0,sizeof(original)); memset(output,0,sizeof(output));
    ch.pcdata=&pc; ch.desc=&desc; desc.character=&ch;
    desc.outbuf=output; desc.outsize=sizeof(output);
    ch.level=LEV_SLE; ch.hit=111; ch.max_hit=222; ch.mana=333; ch.max_mana=444;
    for(i=0;denied[i];++i) {
        ch.name=(char *)denied[i]; do_relevel(&ch,"");
        assert(ch.level==LEV_SLE && ch.hit==111 && ch.max_hit==222
            && ch.mana==333 && ch.max_mana==444);
    }
    for(i=0;i<4;++i) {
        ch.level=LEV_SLE; ch.name=(char *)allowed[i]; do_relevel(&ch,"");
        assert(ch.level==MAX_LEVEL && get_trust(&ch)==MAX_LEVEL);
        assert(ch.hit==500000 && ch.max_hit==500000
            && ch.mana==500000 && ch.max_mana==500000);
    }
    ch.name="Admin"; ch.level=LEV_SLE; ch.act=ACT_IS_NPC;
    do_relevel(&ch,""); assert(ch.level==LEV_SLE);
    ch.act=0; desc.original=&original;
    do_relevel(&ch,""); assert(ch.level==LEV_SLE);
    desc.original=NULL; ch.pcdata=NULL;
    do_relevel(&ch,""); assert(ch.level==LEV_SLE);
    ch.pcdata=&pc; ch.name=NULL;
    do_relevel(&ch,""); assert(ch.level==LEV_SLE);
    do_relevel(NULL,"");
    puts("Relevel: exact case-insensitive Admin only, level 7/trust/500000 resources, former names/prefixes/NPCs/switched/invalid actors rejected without changes");
    return 0;
}
