/* Genesis staff balances: existing currency fields, never ability/mastery ranks. */
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include "merc.h"

enum {
    PS_EXP,PS_PRIMAL,PS_GOLD,PS_CP,PS_CPTOTAL,PS_QUEST,PS_MANA,PS_HIT,
    PS_BLOOD,PS_WILL,PS_KI,PS_QUINT,PS_NANITES,PS_BIONICS,PS_SOULS,
    PS_ENERGY,PS_HUMANITY,PS_HUMANITYCAP,PS_BOMBS
};
typedef struct {
    const char *name,*aliases;
    int kind;
    long required_class;
    bool hidden;
} PSET_CURRENCY;

static const PSET_CURRENCY currencies[]={
    {"exp","xp experience",PS_EXP,0,FALSE},
    {"primal","primalpoints",PS_PRIMAL,0,FALSE},
    {"gold","coins money",PS_GOLD,0,FALSE},
    {"classpoints","cps cp dp favor favour classcostpoints class_cost_points",PS_CP,0,FALSE},
    {"classpointstotal","cptotal cp_total",PS_CPTOTAL,0,FALSE},
    {"questpoints","quest qp quest_points",PS_QUEST,0,FALSE},
    {"mana","",PS_MANA,0,FALSE},
    {"hit","hp health",PS_HIT,0,FALSE},
    {"blood","",PS_BLOOD,0,FALSE},
    {"willpower","will",PS_WILL,0,FALSE},
    {"ki","",PS_KI,CLASS_NINJA,FALSE},
    {"quintessence","quint",PS_QUINT,CLASS_MAGE,FALSE},
    {"nanites","nanite",PS_NANITES,CLASS_CYBORG,FALSE},
    {"bionics","bionic",PS_BIONICS,CLASS_CYBORG,FALSE},
    {"souls","soulpower",PS_SOULS,CLASS_DEMON,FALSE},
    {"energy","classenergy",PS_ENERGY,0,FALSE},
    {"humanity","freehumanity",PS_HUMANITY,CLASS_CYBORG,FALSE},
    {"humanitycap","humanitycapacity",PS_HUMANITYCAP,CLASS_CYBORG,FALSE},
    {"bombs","",PS_BOMBS,CLASS_CYBORG,FALSE},
    {"pomenergy","pom",PS_ENERGY,CLASS_MOOGLE,TRUE},
    {"soulenergy","",PS_ENERGY,CLASS_DEATHKNIGHT,TRUE},
    {"pathos","",PS_ENERGY,CLASS_WRAITH,TRUE},
    {"chi","",PS_ENERGY,CLASS_KUEIJIN,TRUE},
    {"stoneenergy","",PS_ENERGY,CLASS_GARGOYLE,TRUE}
};

bool pset_parse_amount(const char *text, long *value)
{
    const unsigned char *p=(const unsigned char *)text;
    char *end;
    long n;
    if(!p || !*p || !value) return FALSE;
    for(;*p;++p) if(!isdigit(*p)) return FALSE;
    errno=0; n=strtol(text,&end,10);
    if(errno==ERANGE || *end || n<0) return FALSE;
    *value=n; return TRUE;
}

static bool alias_matches(const char *key, const char *aliases)
{
    const char *end;
    size_t length=strlen(key);
    while(*aliases) {
        while(*aliases==' ') ++aliases;
        end=strchr(aliases,' ');
        if(!end) end=aliases+strlen(aliases);
        if((size_t)(end-aliases)==length && !strncasecmp(key,aliases,length)) return TRUE;
        aliases=end;
    }
    return FALSE;
}

static bool currency_slot(CHAR_DATA *v, const PSET_CURRENCY *r,
                          long **slot, long *minimum, long *maximum)
{
    PC_DATA *p=v->pcdata;
    long parent=0,spent;
    *minimum=0; *maximum=1000000000L;
    if(r->required_class && v->class!=r->required_class) return FALSE;
    switch(r->kind) {
    case PS_EXP: *slot=&v->exp; break;
    case PS_PRIMAL: *slot=&v->primal; break;
    case PS_GOLD: *slot=&v->gold; break;
    case PS_CP: *slot=&p->classpoints[CP_CURRENT]; *maximum=10000000; break;
    case PS_CPTOTAL: *slot=&p->classpoints[CP_TOTAL]; *maximum=10000000; break;
    case PS_QUEST: *slot=&p->quest; *maximum=1000000; break;
    case PS_MANA: *slot=&v->mana; *maximum=UMIN(*maximum,v->max_mana); break;
    case PS_HIT: *slot=&v->hit; *minimum=1; *maximum=UMIN(*maximum,v->max_hit); break;
    case PS_BLOOD: *slot=&v->blood[BLOOD_CURRENT]; *maximum=UMIN(*maximum,v->blood[BLOOD_POOL]); break;
    case PS_WILL: *slot=&p->willpower[0]; *maximum=UMIN(*maximum,p->willpower[1]); break;
    case PS_KI: *slot=&p->ninja[NINJA_KI]; *maximum=300; break;
    case PS_QUINT: *slot=&v->quint[BODY]; *maximum=10000; break;
    case PS_NANITES:
        if(!cyborg_core_valid(&p->cyborg) || !cyborg_runtime_valid(&p->cyber)) return FALSE;
        *slot=&p->cyber.nanites;
        *maximum=cyborg_has_component(v,CY_BATTERY) ? 10000 : 0; break;
    case PS_BIONICS: case PS_BOMBS:
        if(!cyborg_core_valid(&p->cyborg) || !cyborg_systems_valid(&p->cybernetics)) return FALSE;
        *slot=r->kind==PS_BIONICS ? &p->cybernetics.bionic : &p->cybernetics.bombs;
        *maximum=r->kind==PS_BIONICS ? 10000000 : 20; break;
    case PS_SOULS:
        if(!reference_legacy_valid(&p->legacy)) return FALSE;
        *slot=&p->legacy.souls; *maximum=REF_SOUL_CAP; break;
    case PS_ENERGY:
        if(v->class==CLASS_MOOGLE) parent=GEN_PARENT_MOOGLE;
        else if(v->class==CLASS_DEATHKNIGHT) parent=GEN_PARENT_DEATHKNIGHT;
        if(parent) {
            if(!crafted_state_valid(&p->crafted) || p->crafted.parent!=parent) return FALSE;
            *slot=&p->crafted.energy; *maximum=CRAFTED_ENERGY_CAP; break;
        }
        if(v->class==CLASS_GARGOYLE) parent=GEN_PARENT_GARGOYLE;
        else if(v->class==CLASS_KUEIJIN) parent=GEN_PARENT_KUEIJIN;
        else if(v->class==CLASS_WRAITH) parent=GEN_PARENT_WRAITH;
        if(!parent || !lineage_state_valid(&p->lineage) || p->lineage.parent!=parent) return FALSE;
        *slot=&p->lineage.energy; *maximum=LINEAGE_ENERGY_CAP; break;
    case PS_HUMANITY: case PS_HUMANITYCAP:
        if(!cyborg_core_valid(&p->cyborg)) return FALSE;
        spent=p->cyborg.humanity_limit-p->cyborg.humanity;
        if(spent>1000) return FALSE;
        if(r->kind==PS_HUMANITY) {
            *slot=&p->cyborg.humanity; *maximum=1000-spent;
        } else {
            *slot=&p->cyborg.humanity_limit; *minimum=spent; *maximum=1000;
        }
        break;
    default: return FALSE;
    }
    return *maximum>=*minimum;
}

static void currency_view(CHAR_DATA *ch, CHAR_DATA *v)
{
    size_t i;
    long *slot,minimum,maximum;
    char detail[128];
    class_menu_header(ch,"PSET CURRENCIES","Currency","Balance / allowed staff-edit range");
    for(i=0;i<sizeof(currencies)/sizeof(currencies[0]);++i) {
        if(currencies[i].hidden) continue;
        if(currency_slot(v,&currencies[i],&slot,&minimum,&maximum))
            format_buffer(detail,sizeof(detail),"%li  [%li..%li]",*slot,minimum,maximum);
        else strcpy(detail,"Unavailable for this class/state");
        class_menu_row(ch,currencies[i].name,detail);
    }
    stc("Set an absolute balance: pset <player> <currency> <amount>\n\r"
        "Aliases: xp; cps/cp/dp/favor; quest/qp; quint; bionic; soulpower.\n\r"
        "energy aliases: pomenergy, soulenergy, chi, pathos, stoneenergy.\n\r"
        "humanity = free implant budget; humanitycap = total budget (max 1000).\n\r"
        "Positive nanites require an installed battery to survive login.\n\r"
        "Class points current/total are independent. No mastery is granted.\n\r",ch);
}

bool pset_currency_command(CHAR_DATA *ch, CHAR_DATA *v, const char *field,
                           const char *amount, bool extra)
{
    const PSET_CURRENCY *r=NULL;
    size_t i;
    long *slot,minimum,maximum,value,before,old_capacity=0;
    CYBORG_CORE core;
    char message[512];
    if(!str_cmp(field,"currencies") || !str_cmp(field,"wallet")) {
        if(amount[0] || extra) stc("Syntax: pset <player> currencies\n\r",ch);
        else currency_view(ch,v);
        return TRUE;
    }
    for(i=0;i<sizeof(currencies)/sizeof(currencies[0]);++i)
        if(!str_cmp(field,currencies[i].name) || alias_matches(field,currencies[i].aliases)) {
            r=&currencies[i]; break;
        }
    if(!r) return FALSE;
    if(!currency_slot(v,r,&slot,&minimum,&maximum)) {
        stc("That currency is unavailable for the player's class or current state.\n\r",ch);
        return TRUE;
    }
    if(extra || !pset_parse_amount(amount,&value) || value<minimum || value>maximum) {
        format_buffer(message,sizeof(message),
            "Set %s using one whole number in %li..%li; no signs or extra arguments.\n\r",
            r->name,minimum,maximum);
        stc(message,ch);
        if(r->kind==PS_NANITES && !maximum)
            stc("Install a Cyborg battery before granting positive nanites.\n\r",ch);
        return TRUE;
    }
    before=*slot;
    if(r->kind==PS_HUMANITY || r->kind==PS_HUMANITYCAP) {
        core=v->pcdata->cyborg; old_capacity=core.humanity_limit;
        if(r->kind==PS_HUMANITY) {
            core.humanity_limit=core.humanity_limit-core.humanity+value;
            core.humanity=value;
        } else {
            core.humanity=value-(core.humanity_limit-core.humanity);
            core.humanity_limit=value;
        }
        if(!cyborg_core_valid(&core)) {
            stc("That edit would invalidate the installed implant budget.\n\r",ch); return TRUE;
        }
        v->pcdata->cyborg=core;
    } else if(r->kind==PS_BOMBS && !value && v->pcdata->cybernetics.armed) {
        stc("Disarm bombs before setting their count to zero.\n\r",ch); return TRUE;
    } else *slot=value;
    format_buffer(message,sizeof(message),"PSET currency: %s -> %s %s %li -> %li",
        ch->name,v->name,r->name,before,value);
    log_string(message);
    if(r->kind==PS_HUMANITY || r->kind==PS_HUMANITYCAP) {
        format_buffer(message,sizeof(message),"PSET humanity capacity: %s %li -> %li",
            v->name,old_capacity,v->pcdata->cyborg.humanity_limit);
        log_string(message);
    }
    format_buffer(message,sizeof(message),"%s's %s: %li -> %li. Normal save/autosave rules apply.\n\r",
        v->name,r->name,before,value);
    stc(message,ch);
    return TRUE;
}
