/* Staff/class display regression: no permission, wallet or enrollment changes. */
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"
extern void init_mm(void);
extern HELP_DATA *help_first;
static CHAR_DATA ch;
static PC_DATA pc;
static DESCRIPTOR_DATA desc;
static char output[65536];
static void reset(long level,int ansi)
{
    int i;
    memset(&ch,0,sizeof(ch)); memset(&pc,0,sizeof(pc)); memset(&desc,0,sizeof(desc));
    ch.pcdata=&pc; ch.name=ch.short_descr="Viewer"; ch.clan=ch.morph=str_empty;
    ch.level=level; ch.position=POS_STANDING;
    ch.hit=ch.max_hit=100; ch.mana=ch.max_mana=100; ch.primal=100; ch.exp=1000;
    for(i=0;i<5;++i) ch.stats[i]=5;
    if(ansi) SET_BIT(ch.act,PLR_ANSI);
    desc.outbuf=output; desc.outsize=sizeof(output); desc.character=&ch; ch.desc=&desc;
    output[0]='\0';
}
static void run(DO_FUN *fn,const char *text)
{
    CHAR_DATA before=ch;
    PC_DATA saved=pc;
    char input[128];
    size_t width=0;
    const char *s;
    assert(strlen(text)<sizeof(input)); strcpy(input,text);
    desc.outtop=0; output[0]='\0'; fn(&ch,input); assert(desc.outtop>0);
    assert((size_t)desc.outtop<sizeof(output)); output[desc.outtop]='\0';
    assert(!memcmp(&ch,&before,sizeof(ch)) && !memcmp(&pc,&saved,sizeof(pc)));
    for(s=output;*s;++s) {
        if((unsigned char)*s==27 && s[1]=='[') {
            s+=2; while(*s && !(*s>='@' && *s<='~')) ++s;
            if(!*s) break;
        } else if(*s=='\n') { assert(width<=79); width=0; }
        else if(*s!='\r') ++width;
    }
    assert(width<=79);
}
static const char *plain_text(const char *text)
{
    static char plain[65536];
    size_t n=0;
    for(;*text;++text) {
        if((unsigned char)*text==27 && text[1]=='[') {
            text+=2; while(*text && !(*text>='@' && *text<='~')) ++text;
            if(!*text) break;
        } else if(*text!='\r') { assert(n+1<sizeof(plain)); plain[n++]=*text; }
    }
    plain[n]='\0'; return plain;
}
static const char *word(const char *text,const char *name)
{
    const char *p;
    size_t n=strlen(name);
    text=plain_text(text); p=text;
    while((p=strstr(p,name))!=NULL) {
        if((p==text || (!isalnum((unsigned char)p[-1]) && p[-1]!='_'))
            && !isalnum((unsigned char)p[n]) && p[n]!='_') return p;
        ++p;
    }
    return NULL;
}
static void staff(void)
{
    long level,i;
    CHAR_DATA original;
    for(level=LEV_AWA;level<=LEV_ST;++level) {
        reset(level,0); run(do_wizhelp,"");
        for(i=0;cmd_table[i].name[0];++i) if(cmd_table[i].level>=LEV_BUI) {
            int allowed=level>=cmd_table[i].level && cmd_table[i].do_fun!=do_huh;
            if((word(output,cmd_table[i].name)!=NULL)!=allowed) {
                fprintf(stderr,"Staff listing mismatch: trust=%ld command=%s required=%ld expected=%d\n%s\n",
                    level,cmd_table[i].name,cmd_table[i].level,allowed,output);
            }
            assert((word(output,cmd_table[i].name)!=NULL)==allowed);
        }
        if(level>=LEV_BUI) assert(strstr(output,"Builder | Required level 4"));
        if(level>=LEV_IMM) assert(strstr(output,"Immortal | Required level 5"));
        if(level>=LEV_IMP) {
            assert(strstr(output,"Implementor | Required level 6"));
            assert(word(output,"allow")<word(output,"ban"));
        }
        if(level==LEV_ST) {
            assert(strstr(output,"Storyteller | Required level 7"));
            assert(word(output,"nuke")<word(output,"superat"));
        }
    }
    reset(LEV_ST,0); run(do_wizhelp,"immortal");
    if(!word(output,"pset") || word(output,"ban") || word(output,"nuke"))
        fprintf(stderr,"Unexpected filtered staff output:\n%s\n",output);
    assert(word(output,"pset") && !word(output,"ban") && !word(output,"nuke"));
    assert(!strstr(output,"Builder |") && !strstr(output,"Implementor |"));
    run(do_wizhelp,"5"); assert(word(output,"pset") && !word(output,"ban"));
    run(do_wizhelp,"bogus"); assert(strstr(output,"Syntax:"));
    run(do_wizhelp,"builder extra"); assert(strstr(output,"Syntax:"));
    reset(LEV_BUI,0); run(do_wizhelp,"storyteller");
    assert(strstr(output,"above") && !word(output,"nuke"));
    reset(LEV_AWA,0); memset(&original,0,sizeof(original)); original.level=LEV_IMM;
    SET_BIT(ch.act,ACT_IS_NPC); desc.original=&original; run(do_wizhelp,"");
    assert(word(output,"pset") && !word(output,"ban"));
    puts("Wizhelp: every registered staff command checked per effective level; sorted role groups, filters, guard aliases hidden; no permission changes");
}
static void enrollment(void)
{
    const char *names[]={"Angel","Cyborg","Deathknight","Demon","Dragon","Drow",
        "Elemental","Gargoyle","Highlander","Kuei-jin","Mage","Monk","Moogle",
        "Ninja","Phoenix","Sorcerer","Vampire","Werewolf","Wraith"};
    size_t i;
    int ansi;
    for(ansi=0;ansi<=1;++ansi) {
        reset(LEV_AWA,ansi); run(do_autoclass,"");
        for(i=0;i<sizeof(names)/sizeof(names[0]);++i) assert(strstr(output,names[i]));
        assert((strstr(output,"\033[1;36mCyborg")!=NULL)==ansi);
        assert(!strstr(output,"Malkavian")); /* No enrollment handler exists. */
        assert(strstr(output,"Clan / faction") && strstr(output,"HELP CLASS"));
    }
    puts("Autoclass: all 19 displayed identities, ANSI on/off, bounded columns, supported clans only; display cannot enroll/spend");
}
static void help_body(const char *file,const char *marker,char *body,size_t size)
{
    char path[1024],source[160000],*slash,*start,*end;
    size_t bytes;
    FILE *fp;
    snprintf(path,sizeof(path),"%s",__FILE__); slash=strrchr(path,'/'); assert(slash); *slash='\0';
    assert(strlen(path)+strlen(file)+10<sizeof(path)); strcat(path,"/../area/"); strcat(path,file);
    fp=fopen(path,"rb"); assert(fp);
    bytes=fread(source,1,sizeof(source)-1,fp); assert(!ferror(fp) && feof(fp)); fclose(fp);
    source[bytes]='\0'; start=strstr(source,marker); assert(start); start+=strlen(marker);
    end=strchr(start,'~'); assert(end && (size_t)(end-start)<size);
    memcpy(body,start,(size_t)(end-start)); body[end-start]='\0';
}
static void help_pages(void)
{
    HELP_DATA entry,*old=help_first;
    char body[8192];
    const char *p;
    int bullets=0;
    memset(&entry,0,sizeof(entry)); help_first=&entry;
    help_body("main.hlp","-1 WIZLIST~\n",body,sizeof(body));
    entry.keyword="WIZLIST"; entry.text=body; entry.level=-1;
    reset(LEV_AWA,0); run(do_wizlist,"");
    assert(strstr(output,"LaWaylon") && strstr(output,"Admin") && strstr(output,"Builder"));
    assert(!strstr(output,"Akuma") && !strstr(output,"Corruption"));
    help_body("kits.hlp","0 CLASS CLASSES KITS CLASSKITS~\n",body,sizeof(body));
    entry.keyword="CLASS CLASSES KITS CLASSKITS"; entry.level=0;
    run(do_help,"class"); assert(strstr(output,"Available classes"));
    for(p=plain_text(output);*p;++p) if(*p=='\n' && p[1]==' ' && p[2]=='-' && p[3]==' ') ++bullets;
    assert(bullets==19 && strstr(output,"HELP KUEIJIN") && strstr(output,"HELP SORCERER"));
    run(do_help,"classes"); assert(strstr(output,"Available classes"));
    help_first=old;
    puts("Actual WIZLIST/CLASS help bodies loaded: current curated roster, 19 linked bullets, singular/plural aliases; <=79 columns");
}
int main(void)
{
    init_mm(); staff(); enrollment(); help_pages();
    return 0;
}
