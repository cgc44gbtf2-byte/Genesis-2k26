/* Genesis-native conversions of EM/ADG concepts. Included by crafted.c so the
 * same checked target, affect and numeric helpers are used by all five kits.
 * No reference bitfields, descriptor possession or private-input hooks.
 */
enum { LP_MELEE,LP_WINGS,LP_REST,LP_OBJECT,LP_PASS,LP_SELFHEAL,LP_EDGE,LP_DRAIN,
 LP_SHELL,LP_HIDE,LP_MASK,LP_POISON,LP_FORM,LP_TRAVEL,LP_SCOUT,LP_CALM,LP_AGILITY,
 LP_SENSE,LP_BLIND,LP_BURST,LP_READ,LP_HEAL,LP_RAGE,LP_REFORM,LP_WEAKEN,LP_STRIKE,
 LP_CRAFT,LP_PHASE,LP_SHIELD,LP_GUARD,LP_WAKE,LP_MENTOR,LP_AKUMA,LP_HARVEST,
 LP_ARMOR,LP_ROOT,LP_FEAR,LP_CURE,LP_ITEMJUMP,LP_CRIPPLE,LP_FIREEDGE,LP_DREAM,LP_BLESSSIGHT,LP_WEATHER };
typedef struct lineage_power {
    int parent;
    const char *key,*name;
    int track,level;
    long mana,energy;
    int kind;
    long amount;
    const char *source;
} LINEAGE_POWER;
static const LINEAGE_POWER lineage_powers[] = {
#define L(parent,key,title,track,level,mana,energy,kind,amount,source) \
 {GEN_PARENT_##parent,#key,title,track,level,mana,energy,LP_##kind,amount,source},
#include "lineage_powers.def"
#undef L
};
static const char *const lineage_tracks[3][LINEAGE_TRACKS] = {
 {"visceratika"},
 {"yin","yang","hun","po","blood","bone","jade","flesh","ghost","equilibrium",
  "tapestry","yangprana","yinprana","blackwind","chimuh","internalize","dragonscales","demonart"},
 {"argos","castigate","embody","inhabit","keening","moliate","outrage","pandemonium","phantasm","puppetry","usury"}
};
static const char *const lineage_self_effects[] = {
 "lineage traits","lineage shell","lineage body","lineage edge","lineage conceal",
 "lineage wings","lineage sight","lineage pass","lineage phase","lineage refuge",NULL};

int lineage_class_parent(CHAR_DATA *ch)
{
    if(!ch || IS_NPC(ch) || !ch->pcdata) return 0;
    return ch->class==CLASS_GARGOYLE ? GEN_PARENT_GARGOYLE
        : ch->class==CLASS_KUEIJIN ? GEN_PARENT_KUEIJIN
        : ch->class==CLASS_WRAITH ? GEN_PARENT_WRAITH : 0;
}
int lineage_valid(CHAR_DATA *ch)
{
    int id=lineage_class_parent(ch);
    return id && ch->pcdata->lineage.parent==id && lineage_state_valid(&ch->pcdata->lineage);
}
static int lineage_group(long id)
{
    return id==GEN_PARENT_GARGOYLE ? 0 : id==GEN_PARENT_KUEIJIN ? 1 : 2;
}
static void lineage_strip_self(CHAR_DATA *ch)
{
    int i;
    for(i=0;lineage_self_effects[i];++i) affect_strip(ch,craft_sn(lineage_self_effects[i]));
}
void lineage_clear(CHAR_DATA *ch)
{
    if(!ch || IS_NPC(ch) || !ch->pcdata) return;
    lineage_strip_self(ch);
    affect_strip(ch,craft_sn("lineage recovery"));
    memset(&ch->pcdata->lineage,0,sizeof(ch->pcdata->lineage));
}
void lineage_restore(CHAR_DATA *ch)
{
    LINEAGE_STATE *s;
    if(!ch || IS_NPC(ch) || !ch->pcdata) return;
    affect_strip(ch,craft_sn("lineage traits"));
    if(!lineage_valid(ch)) return;
    s=&ch->pcdata->lineage;
    if(s->parent==GEN_PARENT_GARGOYLE) {
        if(s->tracks[0]>=3) craft_affect(ch,"lineage traits",APPLY_CON,1,-1,0);
        if(s->tracks[0]>=5) craft_affect(ch,"lineage traits",APPLY_STR,1,-1,0);
        if(s->tracks[0]) craft_affect(ch,"lineage traits",APPLY_AC,s->tracks[0]*5,-1,0);
    } else if(s->parent==GEN_PARENT_KUEIJIN) {
        if(s->tracks[2]>=3) craft_affect(ch,"lineage traits",APPLY_WIS,1,-1,0);
        if(s->tracks[1]>=3) craft_affect(ch,"lineage traits",APPLY_CON,1,-1,0);
        if(s->tracks[3]>=3) craft_affect(ch,"lineage traits",APPLY_STR,1,-1,0);
    } else {
        if(s->tracks[5]>=3) craft_affect(ch,"lineage traits",APPLY_DEX,1,-1,0);
        if(s->tracks[10]>=3) craft_affect(ch,"lineage traits",APPLY_WIS,1,-1,0);
        if(s->tracks[2]>=5) craft_affect(ch,"lineage traits",APPLY_CON,1,-1,0);
    }
}
void lineage_loaded(CHAR_DATA *ch)
{
    int id=lineage_class_parent(ch);
    if(!ch || IS_NPC(ch) || !ch->pcdata) return;
    /* Temporary aspects/ownership never become persistent authority. Recovery
     * is retained on a valid reload; bad/mismatched state loses all mastery.
     */
    lineage_strip_self(ch);
    if(!id || !lineage_state_valid(&ch->pcdata->lineage) || ch->pcdata->lineage.parent!=id) {
        lineage_clear(ch); ch->pcdata->lineage.parent=id;
    }
    if(id && ch->pcdata->chobj==NULL && IS_EXTRA(ch,EXTRA_OSWITCH)) {
        REMOVE_BIT(ch->extra,EXTRA_OSWITCH); REMOVE_BIT(ch->affected_by,AFF_POLYMORPH);
        ch->pcdata->obj_vnum=0; ch->pcdata->reference_form=0;
        free_string(ch->morph); ch->morph=str_dup("");
    }
    lineage_restore(ch);
}
static long lineage_value(CHAR_DATA *ch,const char *name)
{
    AFFECT_DATA *af;
    int sn=craft_sn(name);
    if(!lineage_valid(ch)) return 0;
    for(af=ch->affected;af;af=af->next)
        if(af->type==sn && af->location==APPLY_NONE) return af->modifier;
    return 0;
}
int lineage_perception(CHAR_DATA *ch) { return (lineage_value(ch,"lineage sight") & 6)!=0; }
int lineage_nightvision(CHAR_DATA *ch) { return (lineage_value(ch,"lineage sight") & 1)!=0; }
int lineage_shadow_sight(CHAR_DATA *ch) { return (lineage_value(ch,"lineage sight") & 8)!=0; }
int lineage_concealed(CHAR_DATA *ch)
{
    return lineage_valid(ch) && is_affected(ch,gsn_lineage_conceal);
}
int lineage_ground_awareness(CHAR_DATA *ch,CHAR_DATA *v)
{
    return lineage_valid(ch) && ch->class==CLASS_GARGOYLE && ch->pcdata->lineage.tracks[0]>=2
        && v && v->in_room==ch->in_room && !IS_AFFECTED(ch,AFF_FLYING) && !IS_AFFECTED(v,AFF_FLYING);
}
long lineage_absorb(CHAR_DATA *ch,long dam,long dt)
{
    long percent,phase;
    if(dam<=0 || !lineage_valid(ch)) return dam;
    percent=(ch->class==CLASS_GARGOYLE && dt>=TYPE_HIT && dt<TYPE_SPELL)
        ? ch->pcdata->lineage.tracks[0]*5 : 0;
    phase=URANGE(0,lineage_value(ch,"lineage phase"),25);
    percent=UMIN(35,percent+phase);
    return dam-(dam/100)*percent-(dam%100)*percent/100;
}
static const LINEAGE_POWER *lineage_marker(CHAR_DATA *ch,const char *name)
{
    long v=lineage_value(ch,name);
    const LINEAGE_POWER *p;
    if(v<1 || v>(long)(sizeof(lineage_powers)/sizeof(lineage_powers[0]))) return NULL;
    p=&lineage_powers[v-1];
    return p->parent==lineage_class_parent(ch)
        && ch->pcdata->lineage.tracks[p->track]>=p->level ? p : NULL;
}
const char *lineage_aspect(CHAR_DATA *ch)
{
    const LINEAGE_POWER *p=lineage_marker(ch,"lineage body");
    return p ? p->name : NULL;
}
long lineage_weapon_bonus(CHAR_DATA *ch,CHAR_DATA *v,OBJ_DATA *weapon)
{
    const LINEAGE_POWER *p=lineage_marker(ch,"lineage edge");
    if(!p || !v || v==ch || v->in_room!=ch->in_room || !ch->in_room
        || IS_SET(ch->in_room->room_flags,ROOM_SAFE) || IS_BUI(v) || is_same_group(ch,v)
        || (!IS_NPC(v) && (!CAN_PK(ch) || !CAN_PK(v))) || is_safe(ch,v)) return 0;
    if(p->kind==LP_FIREEDGE && (!weapon || weapon->item_type!=ITEM_WEAPON
        || weapon->carried_by!=ch || (weapon->wear_loc!=WEAR_WEAPON && weapon->wear_loc!=WEAR_HOLD))) return 0;
    return URANGE(0,p->amount,250);
}
static int lineage_body_ready(CHAR_DATA *ch)
{
    return lineage_valid(ch) && ch->in_room && ch->hit>0 && ch->max_hit>0 && ch->max_mana>0 && ch->position>=POS_FIGHTING
        && !IS_MORE(ch,MORE_HEAD) && !IS_EXTRA(ch,EXTRA_OSWITCH)
        && !IS_AFFECTED(ch,AFF_POLYMORPH) && !is_affected(ch,gsn_lycaon);
}
static int lineage_peace(CHAR_DATA *ch)
{
    return lineage_body_ready(ch) && ch->position>=POS_STANDING && !ch->fighting && ch->fight_timer<=0;
}
static void lineage_commit(CHAR_DATA *ch,const LINEAGE_POWER *p,int own_mana)
{
    if(own_mana) ch->mana-=p->mana;
    ch->pcdata->lineage.energy-=p->energy;
    affect_strip(ch,gsn_lineage_conceal);
    WAIT_STATE(ch,12);
    craft_affect(ch,"lineage recovery",APPLY_NONE,0,p->kind==LP_AKUMA ? 3 : 1,0);
}
static int lineage_ready(CHAR_DATA *ch,const LINEAGE_POWER *p,int transient)
{
    return lineage_valid(ch) && (lineage_body_ready(ch)
        || (transient && ch->hit>0 && ch->pcdata->reference_form && ch->pcdata->chobj))
        && ch->pcdata->lineage.tracks[p->track]>=p->level
        && (p->kind!=LP_AKUMA || ch->pcdata->lineage.dharma==5)
        && !is_affected(ch,craft_sn("lineage recovery")) && !is_affected(ch,gsn_parent_recovery)
        && ch->mana>=p->mana && ch->pcdata->lineage.energy>=p->energy;
}
static const char *lineage_effect(int kind)
{
    switch(kind) {
    case LP_SHELL:case LP_SHIELD:return "lineage shell";
    case LP_FORM:case LP_MASK:case LP_AGILITY:case LP_RAGE:case LP_AKUMA:return "lineage body";
    case LP_EDGE:case LP_FIREEDGE:return "lineage edge";
    case LP_HIDE:return "lineage conceal";
    case LP_WINGS:return "lineage wings";
    case LP_SENSE:return "lineage sight";
    case LP_PASS:return "lineage pass";
    case LP_PHASE:return "lineage phase";
    default:return NULL;
    }
}
static void lineage_menu(CHAR_DATA *ch)
{
    LINEAGE_STATE *s=&ch->pcdata->lineage;
    const LINEAGE_POWER *p;
    char buf[256];
    int i,group=lineage_group(s->parent),n=lineage_track_count(s->parent);
    snprintf(buf,sizeof(buf),"Class energy %ld/%ld; Dharma %ld/5. train <track>; focus <mana>.\n\r",
        s->energy,LINEAGE_ENERGY_CAP,s->dharma); stc(buf,ch);
    for(i=0;i<n;++i) {
        snprintf(buf,sizeof(buf),"%-16s %ld/5  next XP %ld\n\r",lineage_tracks[group][i],s->tracks[i],
            s->tracks[i]<5 ? 25000*(s->tracks[i]+1)*(s->tracks[i]+1) : 0); stc(buf,ch);
    }
    for(i=0;i<(int)(sizeof(lineage_powers)/sizeof(lineage_powers[0]));++i) {
        p=&lineage_powers[i]; if(p->parent!=s->parent) continue;
        snprintf(buf,sizeof(buf),"%-17s %s %d; mana %ld; energy %ld%s\n\r",p->key,
            lineage_tracks[group][p->track],p->level,p->mana,p->energy,
            !strcmp(p->source,"G") ? " [Genesis design]" : " [converted]"); stc(buf,ch);
    }
    stc("Aspects use exclusive slots; most self-effects end on logout. Hybrids/ascension remain disabled.\n\r",ch);
}
static void lineage_craft(CHAR_DATA *ch,const LINEAGE_POWER *p,char *arg)
{
    OBJ_INDEX_DATA *index;
    OBJ_DATA *obj;
    long wear=ITEM_WEAR_HOLD;
    const char *label="chi blade";
    if(!lineage_peace(ch) || ch->carry_number<0 || ch->carry_number>=100
        || ch->carry_weight<0 || ch->carry_weight>UMIN(1000,char_stat(ch,STAT_STR))*20-5) return;
    if(p->kind==LP_ARMOR) {
        if(!strcmp(arg,"robe")) { wear=ITEM_WEAR_BODY; label="jade robe"; }
        else if(!strcmp(arg,"sandals") || !strcmp(arg,"sandles")) { wear=ITEM_WEAR_FEET; label="jade sandals"; }
        else if(!strcmp(arg,"pendant")) { wear=ITEM_WEAR_NECK; label="jade pendant"; }
        else if(!strcmp(arg,"bracelet")) { wear=ITEM_WEAR_WRIST; label="jade bracelet"; }
        else { stc("kotearmor robe/sandals/pendant/bracelet (70 primal; temporary equipment).\n\r",ch); return; }
        if(ch->primal<70) return;
    } else if(arg[0]) return;
    index=get_obj_index(OBJ_VNUM_PROTOPLASM);
    if(!index) { stc("Missing creation template; no cost charged.\n\r",ch); return; }
    obj=create_object(index,50); if(!obj) return;
    free_string(obj->name); obj->name=str_dup(label);
    free_string(obj->short_descr); obj->short_descr=str_dup(label);
    free_string(obj->description); obj->description=str_dup("An ephemeral chi creation rests here.");
    obj->item_type=p->kind==LP_ARMOR ? ITEM_ARMOR : ITEM_WEAPON;
    obj->wear_flags=ITEM_TAKE|wear; obj->quest|=QUEST_VANISH;
    obj->weight=5; obj->cost=0; obj->timer=5;
    obj->extra_flags=0; obj->spectype=0;
    memset(obj->value,0,sizeof(obj->value));
    if(p->kind==LP_ARMOR) obj->value[0]=p->amount;
    else { obj->value[1]=10; obj->value[2]=20; obj->value[3]=TYPE_SLASH; }
    lineage_commit(ch,p,1); if(p->kind==LP_ARMOR) ch->primal-=70;
    obj_to_char(obj,ch); act("You shape $p from chi.",ch,obj,NULL,TO_CHAR);
}
static void lineage_command(CHAR_DATA *ch,char *argument,int parent)
{
    char cmd[MAX_INPUT_LENGTH],arg[MAX_INPUT_LENGTH],extra[MAX_INPUT_LENGTH],buf[256];
    const LINEAGE_POWER *p=NULL;
    LINEAGE_STATE *s;
    CHAR_DATA *v;
    ROOM_INDEX_DATA *to;
    OBJ_DATA *obj;
    AFFECT_DATA af;
    long cost,amount,heal;
    int i,index=-1,group,kind,saved,mode;
    const char *effect;
    if(lineage_class_parent(ch)!=parent) {
        if(ch) stc("That is not your standalone class.\n\r",ch);
        return;
    }
    if(!argument || strlen(argument)>=sizeof(cmd) || !lineage_valid(ch)) {
        stc("Invalid input or class state.\n\r",ch); return;
    }
    argument=one_argument(argument,cmd); argument=one_argument(argument,arg); one_argument(argument,extra);
    if(extra[0]) { stc("Use one ability and at most one target.\n\r",ch); return; }
    s=&ch->pcdata->lineage; group=lineage_group(parent);
    if(!cmd[0] || !strcmp(cmd,"status") || !strcmp(cmd,"wraithpowers")) {
        if(!arg[0]) lineage_menu(ch);
        return;
    }
    if(!strcmp(cmd,"focus")) {
        amount=craft_number(arg);
        if(!amount || !lineage_peace(ch) || amount>ch->mana || amount>LINEAGE_ENERGY_CAP-s->energy
            || is_affected(ch,craft_sn("lineage recovery")) || is_affected(ch,gsn_parent_recovery)) return;
        ch->mana-=amount; s->energy+=amount; WAIT_STATE(ch,12);
        craft_affect(ch,"lineage recovery",APPLY_NONE,0,1,0);
        stc("You convert mana into class energy at one to one.\n\r",ch); return;
    }
    if(!strcmp(cmd,"dharma") && parent==GEN_PARENT_KUEIJIN) {
        if(!arg[0]) { snprintf(buf,sizeof(buf),"Dharma %ld/5; dharma train costs 100000*(next level)^2 XP.\n\r",s->dharma); stc(buf,ch); return; }
        cost=100000*(s->dharma+1)*(s->dharma+1);
        if(strcmp(arg,"train") || s->dharma>=5 || !lineage_peace(ch) || ch->exp<cost
            || is_affected(ch,craft_sn("lineage recovery")) || is_affected(ch,gsn_parent_recovery)) return;
        ch->exp-=cost; ++s->dharma; WAIT_STATE(ch,12); stc("Your Dharma improves.\n\r",ch); return;
    }
    if(!strcmp(cmd,"train") || !strcmp(cmd,"path") || !strcmp(cmd,"kdiscipline")
        || !strcmp(cmd,"shintai") || !strcmp(cmd,"noble") || !strcmp(cmd,"arcanoi") || !strcmp(cmd,"visceratika")) {
        if(!arg[0] && parent==GEN_PARENT_GARGOYLE) strcpy(arg,"visceratika");
        for(i=0;i<lineage_track_count(parent);++i)
            if(!strcmp(arg,lineage_tracks[group][i])) { index=i; break; }
        if(index<0) { lineage_menu(ch); return; }
        if((!strcmp(cmd,"path") && (parent!=GEN_PARENT_KUEIJIN || index>3))
            || (!strcmp(cmd,"shintai") && (parent!=GEN_PARENT_KUEIJIN || index<4 || index>8))
            || (!strcmp(cmd,"kdiscipline") && (parent!=GEN_PARENT_KUEIJIN || index<9 || index>13))
            || (!strcmp(cmd,"noble") && (parent!=GEN_PARENT_KUEIJIN || index<14))
            || (!strcmp(cmd,"arcanoi") && parent!=GEN_PARENT_WRAITH)
            || (!strcmp(cmd,"visceratika") && parent!=GEN_PARENT_GARGOYLE)) return;
        cost=25000*(s->tracks[index]+1)*(s->tracks[index]+1);
        if(!lineage_peace(ch) || s->tracks[index]>=5 || ch->exp<cost
            || (parent==GEN_PARENT_KUEIJIN && s->tracks[index]>=s->dharma+2)
            || is_affected(ch,craft_sn("lineage recovery")) || is_affected(ch,gsn_parent_recovery)) return;
        ch->exp-=cost; ++s->tracks[index]; WAIT_STATE(ch,12); lineage_restore(ch);
        stc("Your discipline improves.\n\r",ch); return;
    }
    for(i=0;i<(int)(sizeof(lineage_powers)/sizeof(lineage_powers[0]));++i)
        if(lineage_powers[i].parent==parent && !strcmp(cmd,lineage_powers[i].key)) { p=&lineage_powers[i]; index=i; break; }
    if(!p) { stc("Unknown ability; use your class name for the complete menu.\n\r",ch); return; }
    kind=p->kind; effect=lineage_effect(kind);
    if(kind==LP_REFORM) {
        if(arg[0]) return;
        if(ch->pcdata->reference_form) newclasses_lineage_reference(ch,0,"",0);
        affect_strip(ch,craft_sn("lineage body")); affect_strip(ch,craft_sn("lineage phase"));
        stc("You return to your natural aspect.\n\r",ch); return;
    }
    if(effect && !strcmp(arg,"off")) { affect_strip(ch,craft_sn(effect)); return; }
    if((kind==LP_SCOUT || kind==LP_GUARD) && (!strcmp(arg,"off") || !strcmp(arg,"dismiss"))) {
        newclasses_lineage_reference(ch,kind==LP_SCOUT ? 2 : 1,arg,0); return;
    }
    if(kind==LP_OBJECT && !strcmp(arg,"back")) { newclasses_lineage_reference(ch,0,"",0); return; }
    if(!lineage_ready(ch,p,kind==LP_ITEMJUMP)) {
        stc("You need mastery, a living body, mana, class energy and recovery; Akuma also needs Dharma 5.\n\r",ch); return;
    }
    if(kind==LP_OBJECT || kind==LP_ITEMJUMP || kind==LP_SCOUT || kind==LP_GUARD) {
        mode=kind==LP_OBJECT ? 3 : kind==LP_ITEMJUMP ? 4 : kind==LP_SCOUT ? 2 : 1;
        if(kind==LP_OBJECT && !arg[0]) strcpy(arg,"armor");
        if(newclasses_lineage_reference(ch,mode,arg,p->mana)) lineage_commit(ch,p,0);
        return;
    }
    if(kind==LP_CRAFT || kind==LP_ARMOR) { lineage_craft(ch,p,arg); return; }
    if(kind==LP_WEATHER) {
        if(!IS_OUTSIDE(ch) || (strcmp(arg,"rain") && strcmp(arg,"lightning") && strcmp(arg,"clear"))) return;
        lineage_commit(ch,p,1);
        weather_info.sky=!strcmp(arg,"rain") ? SKY_RAINING : !strcmp(arg,"lightning") ? SKY_LIGHTNING : SKY_CLOUDLESS;
        stc("Your ether reshapes the weather.\n\r",ch); return;
    }
    if(kind==LP_TRAVEL) {
        v=arg[0] ? get_char_world(ch,arg) : NULL;
        to=v ? v->in_room : NULL;
        if(!v || IS_NPC(v) || v==ch || v->hit<=0 || v->position<=POS_STUNNED
            || IS_BUI(v) || !is_same_group(ch,v) || !can_see(ch,v)
            || !to || to==ch->in_room || !lineage_peace(ch) || v->fighting || v->fight_timer>0
            || IS_AFFECTED(ch,AFF_CURSE|AFF_WEBBED|AFF_SHADOWPLANE) || is_affected(ch,gsn_lineage_root)
            || IS_SET(ch->in_room->room_flags,ROOM_NO_RECALL|ROOM_NO_TELEPORT)
            || IS_SET(to->room_flags,ROOM_PRIVATE|ROOM_SOLITARY|ROOM_NO_RECALL|ROOM_NO_TELEPORT)) return;
        lineage_commit(ch,p,1); char_from_room(ch); char_to_room(ch,to); do_look(ch,"auto"); return;
    }
    if(kind==LP_HARVEST) {
        obj=arg[0] ? get_obj_here(ch,arg) : NULL;
        if(!lineage_peace(ch) || !obj || obj->in_room!=ch->in_room || obj->item_type!=ITEM_CORPSE_NPC
            || obj->chobj || IS_SET(obj->quest,REF_QUEST_SOUL_CLAIMED) || s->energy==LINEAGE_ENERGY_CAP
            || IS_SET(ch->in_room->room_flags,ROOM_SAFE)) return;
        amount=UMIN(p->amount,100+10*URANGE(0,obj->level,90));
        lineage_commit(ch,p,1); s->energy+=UMIN(amount,LINEAGE_ENERGY_CAP-s->energy);
        SET_BIT(obj->quest,REF_QUEST_SOUL_CLAIMED);
        stc("You consume residual chi without destroying the corpse or loot.\n\r",ch); return;
    }
    if(kind==LP_READ) {
        obj=arg[0] ? get_obj_here(ch,arg) : NULL;
        if(obj && can_see_obj(ch,obj)) {
            lineage_commit(ch,p,1); spell_identify(skill_lookup("identify"),30,ch,obj); return;
        }
        v=arg[0] ? get_char_room(ch,arg) : NULL;
        if(!v || !can_see(ch,v) || v->hit<=0 || (!IS_NPC(v) && !craft_ally(ch,arg))) return;
        lineage_commit(ch,p,1);
        snprintf(buf,sizeof(buf),"Spirit reading: health %ld/%ld; mana %ld/%ld%s.\n\r",
            v->hit,v->max_hit,v->mana,v->max_mana,IS_NPC(v) ? " (NPC)" : " (willing party)");
        stc(buf,ch); return;
    }
    if(kind==LP_HEAL || kind==LP_SELFHEAL || kind==LP_REST || kind==LP_CALM || kind==LP_CURE
        || kind==LP_WAKE || kind==LP_MENTOR || kind==LP_BLESSSIGHT) {
        v=(kind==LP_SELFHEAL || kind==LP_REST) ? (!arg[0] ? ch : NULL) : craft_ally(ch,arg);
        if(!v || (kind==LP_REST && !lineage_peace(ch))) return;
        if(kind==LP_HEAL || kind==LP_SELFHEAL || kind==LP_REST) {
            heal=UMIN(p->amount,UMIN(UMAX(0,v->max_hit-v->hit),UMAX(0,v->max_hit/10)));
            if(!heal) return;
            lineage_commit(ch,p,1); v->hit+=heal; update_pos(v);
        } else if(kind==LP_MENTOR) {
            if(is_affected(v,craft_sn("lineage mentor"))) return;
            lineage_commit(ch,p,1);
            craft_affect(v,"lineage mentor",p->amount==1 ? APPLY_WIS : APPLY_STR,1,3,0);
            craft_affect(v,"lineage mentor",p->amount==1 ? APPLY_SAVING_SPELL : APPLY_HITROLL,p->amount==1 ? -5 : 10,3,0);
        } else if(kind==LP_BLESSSIGHT) {
            if(is_affected(v,gsn_detect_hidden)) return;
            lineage_commit(ch,p,1); craft_affect(v,"detect hidden",APPLY_NONE,0,3,0);
        } else {
            if(kind==LP_WAKE && (v->fighting || v->position!=POS_SLEEPING)) return;
            if((kind==LP_CALM || kind==LP_CURE) && !is_affected(v,craft_sn("lineage fear"))
                && !is_affected(v,craft_sn("parent fear")) && !is_affected(v,craft_sn("crafted fear"))
                && !is_affected(v,craft_sn("frenzy")) && !(kind==LP_CURE && is_affected(v,gsn_poison))) return;
            lineage_commit(ch,p,1);
            affect_strip(v,craft_sn("lineage fear")); affect_strip(v,craft_sn("parent fear"));
            affect_strip(v,craft_sn("crafted fear"));
            if(kind==LP_CALM) affect_strip(v,craft_sn("frenzy"));
            if(kind==LP_CURE) { affect_strip(v,craft_sn("frenzy")); affect_strip(v,gsn_poison); }
            if(kind==LP_WAKE) { affect_strip(v,gsn_sleep); v->position=POS_STANDING; }
        }
        act("Your ancestral power supports $N.",ch,NULL,v,TO_CHAR); return;
    }
    if(effect) {
        if(arg[0] || is_affected(ch,craft_sn(effect))) return;
        if(kind==LP_FIREEDGE) {
            obj=get_eq_char(ch,WEAR_WEAPON);
            if(!obj || obj->item_type!=ITEM_WEAPON) obj=get_eq_char(ch,WEAR_HOLD);
            if(!obj || obj->item_type!=ITEM_WEAPON) return;
        }
        lineage_commit(ch,p,1);
        if(kind==LP_WINGS) craft_affect(ch,effect,APPLY_NONE,0,3,AFF_FLYING);
        else if(kind==LP_SENSE || kind==LP_PHASE) craft_affect(ch,effect,APPLY_NONE,p->amount,3,0);
        else if(kind==LP_SHELL || kind==LP_SHIELD) {
            craft_affect(ch,effect,APPLY_AC,p->amount,3,0);
            craft_affect(ch,effect,APPLY_SAVING_SPELL,kind==LP_SHIELD ? -10 : -5,3,0);
        } else if(kind==LP_EDGE || kind==LP_FIREEDGE) craft_affect(ch,effect,APPLY_NONE,index+1,3,0);
        else if(kind==LP_FORM || kind==LP_AGILITY || kind==LP_MASK || kind==LP_RAGE || kind==LP_AKUMA) {
            craft_affect(ch,effect,APPLY_NONE,index+1,3,0);
            if(kind==LP_MASK || kind==LP_AGILITY) {
                craft_affect(ch,effect,APPLY_DEX,1,3,0); craft_affect(ch,effect,APPLY_AC,p->amount,3,0);
            } else {
                craft_affect(ch,effect,APPLY_STR,kind==LP_AKUMA ? 2 : 1,3,0);
                craft_affect(ch,effect,APPLY_DAMROLL,p->amount,3,0);
                craft_affect(ch,effect,APPLY_AC,kind==LP_RAGE || kind==LP_AKUMA ? -20 : 15,3,0);
            }
        } else craft_affect(ch,effect,APPLY_NONE,0,3,0);
        stc("Your ancestral power takes effect.\n\r",ch); return;
    }
    v=craft_enemy(ch,arg);
    if(!v || !hurt_fighting(ch,v)) return;
    saved=saves_spell(30+s->tracks[p->track]*5,v);
    amount=p->amount+s->tracks[p->track]*50;
    lineage_commit(ch,p,1);
    if(kind==LP_FEAR || kind==LP_DREAM) {
        if(!saved && !is_affected(v,craft_sn("lineage fear"))) {
            craft_affect(v,"lineage fear",APPLY_HITROLL,-UMAX(10,p->amount),2,0);
            if(kind==LP_DREAM && IS_NPC(v)) { stop_fighting(v,TRUE); v->position=POS_SLEEPING; }
        }
        return;
    }
    if(kind==LP_ROOT) {
        if(!saved && !is_affected(v,gsn_lineage_root)) craft_affect(v,"lineage root",APPLY_DEX,-1,1,0);
        return;
    }
    if(kind==LP_WEAKEN) {
        if(!saved && !is_affected(v,craft_sn("lineage frailty")))
            craft_affect(v,"lineage frailty",APPLY_SAVING_SPELL,p->amount,2,0);
        return;
    }
    if(kind==LP_BLIND && !saved && !IS_AFFECTED(v,AFF_BLIND)) {
        memset(&af,0,sizeof(af)); af.type=gsn_blindness; af.duration=1;
        af.location=APPLY_HITROLL; af.modifier=-4; af.bitvector=AFF_BLIND; affect_to_char(v,&af);
    }
    if(kind==LP_POISON) spell_poison(gsn_poison,10,ch,v);
    if(kind==LP_CRIPPLE && !saved && !is_affected(v,gsn_parent_chill))
        craft_affect(v,"parent chill",APPLY_STR,-1,2,0);
    if(kind==LP_DRAIN) {
        heal=UMIN(150,UMIN(UMAX(0,v->mana),UMAX(0,ch->max_mana-ch->mana)));
        v->mana-=heal; ch->mana+=heal;
    }
    if(kind==LP_BURST) {
        /* Exhaust remaining chi for a capped strike, never arbitrary damage. */
        amount+=UMIN(1000,s->energy/5); s->energy=0;
    }
    if(saved) amount/=2;
    damage(ch,v,amount,kind==LP_MELEE ? TYPE_HIT : TYPE_SPELL,(char *)p->name);
    /* Never touch the victim after native damage/death processing. */
}
void do_gargoyle(CHAR_DATA *ch,char *a) { lineage_command(ch,a,GEN_PARENT_GARGOYLE); }
void do_kueijin(CHAR_DATA *ch,char *a) { lineage_command(ch,a,GEN_PARENT_KUEIJIN); }
void do_wraith(CHAR_DATA *ch,char *a) { lineage_command(ch,a,GEN_PARENT_WRAITH); }
