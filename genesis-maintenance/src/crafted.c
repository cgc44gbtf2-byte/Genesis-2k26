/*
 * Original Genesis Moogle and standalone Deathknight reconstructions.
 * ADG supplies identities/concept only; its missing class implementations are
 * not claimed as recovered. No class bit composition or descriptor control.
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"

enum { CP_STRIKE, CP_WARD, CP_WINGS, CP_HEAL, CP_CHEER, CP_FORTUNE,
       CP_DANCE, CP_FLARE, CP_HOME, CP_SERENADE, CP_MELEE, CP_HARVEST,
       CP_WEAPON, CP_FEAR, CP_MIGHT, CP_REND, CP_LASTSTAND, CP_DOOM };
typedef struct crafted_power {
    int parent;
    const char *key, *name;
    int rank;
    long mana, energy;
    int kind;
    long amount, recovery;
} CRAFTED_POWER;
static const CRAFTED_POWER crafted_powers[] = {
#define C(parent,key,name,rank,mana,energy,kind,amount,recovery) \
    {GEN_PARENT_##parent,#key,name,rank,mana,energy,CP_##kind,amount,recovery},
#include "crafted_powers.def"
#undef C
};

int crafted_class_parent(CHAR_DATA *ch)
{
    if (!ch || IS_NPC(ch) || !ch->pcdata) return 0;
    return ch->class == CLASS_MOOGLE ? GEN_PARENT_MOOGLE
         : ch->class == CLASS_DEATHKNIGHT ? GEN_PARENT_DEATHKNIGHT : 0;
}

static int craft_sn(const char *name) { return (int)skill_lookup(name); }

static void craft_affect(CHAR_DATA *ch, const char *name, int where, long amount, int ticks, long bits)
{
    AFFECT_DATA af;
    memset(&af, 0, sizeof(af));
    af.type = craft_sn(name); af.location = where; af.modifier = amount;
    af.duration = ticks; af.bitvector = bits; affect_to_char(ch, &af);
}

static int craft_valid(CHAR_DATA *ch)
{
    int id = crafted_class_parent(ch);
    return id && crafted_state_valid(&ch->pcdata->crafted) && ch->pcdata->crafted.parent == id;
}

void crafted_clear(CHAR_DATA *ch)
{
    static const char *names[] = {"crafted traits","crafted recovery","crafted ward",
        "crafted fortune","crafted dance","crafted wings","crafted weapon","crafted might",NULL};
    int i;
    if (!ch || IS_NPC(ch) || !ch->pcdata) return;
    for (i = 0; names[i]; ++i) affect_strip(ch, craft_sn(names[i]));
    if (ch->class == CLASS_MOOGLE) affect_strip(ch,craft_sn("crafted morale"));
    memset(&ch->pcdata->crafted, 0, sizeof(ch->pcdata->crafted));
    /* Ally morale and hostile debuffs are not the recipient's class authority. */
}

void crafted_restore(CHAR_DATA *ch)
{
    long rank;
    int dk;
    if (!ch || IS_NPC(ch) || !ch->pcdata) return;
    affect_strip(ch, craft_sn("crafted traits"));
    if (!craft_valid(ch)) return;
    rank = ch->pcdata->crafted.rank;
    dk = ch->class == CLASS_DEATHKNIGHT;
    if (rank >= 4) craft_affect(ch,"crafted traits",dk ? APPLY_STR : APPLY_INT,1,-1,0);
    if (rank >= 6) craft_affect(ch,"crafted traits",dk ? APPLY_CON : APPLY_WIS,1,-1,0);
    if (rank >= 8) craft_affect(ch,"crafted traits",dk ? APPLY_WIS : APPLY_DEX,1,-1,0);
}

void crafted_loaded(CHAR_DATA *ch)
{
    CRAFTED_STATE saved;
    int id = crafted_class_parent(ch);
    if (!ch || IS_NPC(ch) || !ch->pcdata) return;
    saved = ch->pcdata->crafted;
    /* Rebuild authoritative traits, but never clear a saved recovery timer. */
    affect_strip(ch, craft_sn("crafted traits"));
    if (!id || !crafted_state_valid(&saved) || saved.parent != id) {
        crafted_clear(ch);
        ch->pcdata->crafted.parent = id;
    } else ch->pcdata->crafted = saved;
    if (ch->class != CLASS_MOOGLE) {
        affect_strip(ch,craft_sn("crafted wings")); affect_strip(ch,craft_sn("crafted fortune"));
        affect_strip(ch,craft_sn("crafted dance"));
    }
    if (ch->class != CLASS_DEATHKNIGHT) {
        affect_strip(ch,craft_sn("crafted weapon")); affect_strip(ch,craft_sn("crafted might"));
    }
    if (!id) { crafted_clear(ch); return; }
    if (ch->pcdata->crafted.rank < 2) {
        affect_strip(ch,craft_sn("crafted wings")); affect_strip(ch,craft_sn("crafted ward"));
    }
    if (ch->pcdata->crafted.rank < 3) affect_strip(ch,craft_sn("crafted weapon"));
    if (ch->pcdata->crafted.rank < 4) {
        affect_strip(ch,craft_sn("crafted fortune")); affect_strip(ch,craft_sn("crafted might"));
    }
    if (ch->pcdata->crafted.rank < 5) affect_strip(ch,craft_sn("crafted dance"));
    crafted_restore(ch);
}

static int craft_body_ready(CHAR_DATA *ch)
{
    return craft_valid(ch) && ch->in_room && ch->hit > 0 && ch->max_hit>0 && ch->max_mana>0 && ch->position >= POS_FIGHTING
        && !IS_MORE(ch,MORE_HEAD) && !IS_EXTRA(ch,EXTRA_OSWITCH)
        && !IS_AFFECTED(ch,AFF_POLYMORPH) && !is_affected(ch,gsn_lycaon);
}

static int craft_peace(CHAR_DATA *ch)
{
    return craft_body_ready(ch) && ch->position >= POS_STANDING && !ch->fighting && ch->fight_timer <= 0;
}

static int craft_ready(CHAR_DATA *ch, const CRAFTED_POWER *p)
{
    if (!craft_body_ready(ch) || ch->pcdata->crafted.rank < p->rank
        || is_affected(ch,craft_sn("crafted recovery"))
        || ch->mana < p->mana || ch->pcdata->crafted.energy < p->energy) {
        stc("You need mastery, mana, class energy and a recovered, living body.\n\r",ch); return 0;
    }
    return 1;
}

static void craft_commit(CHAR_DATA *ch, const CRAFTED_POWER *p)
{
    ch->mana -= p->mana; ch->pcdata->crafted.energy -= p->energy;
    WAIT_STATE(ch,12);
    combat_recovery_start(ch,craft_sn("crafted recovery"),p->recovery);
}

static CHAR_DATA *craft_ally(CHAR_DATA *ch, const char *arg)
{
    CHAR_DATA *v = arg[0] ? get_char_room(ch,(char *)arg) : ch;
    return v && !IS_NPC(v) && v->pcdata && v->in_room == ch->in_room
        && v->hit > 0 && v->max_hit>0 && v->position > POS_STUNNED && can_see(ch,v)
        && (v == ch || (!IS_BUI(v) && is_same_group(ch,v))) ? v : NULL;
}

static CHAR_DATA *craft_enemy(CHAR_DATA *ch, const char *arg)
{
    CHAR_DATA *v = arg[0] ? get_char_room(ch,(char *)arg) : ch->fighting;
    return v && v != ch && v->in_room == ch->in_room && v->hit > 0
        && can_see(ch,v) && !IS_BUI(v) && !is_same_group(ch,v)
        && !IS_SET(ch->in_room->room_flags,ROOM_SAFE)
        && (IS_NPC(v) || (CAN_PK(ch) && CAN_PK(v))) && !is_safe(ch,v) ? v : NULL;
}

static long craft_number(const char *text)
{
    long value = 0;
    if (!text || !*text) return 0;
    for (; *text; ++text) {
        if (*text < '0' || *text > '9' || value > CRAFTED_ENERGY_CAP/10) return 0;
        value = value*10 + *text-'0';
        if (value > CRAFTED_ENERGY_CAP) return 0;
    }
    return value;
}

long crafted_weapon_bonus(CHAR_DATA *ch, CHAR_DATA *v, OBJ_DATA *weapon)
{
    if (!craft_valid(ch) || ch->class != CLASS_DEATHKNIGHT || !weapon
        || weapon->item_type != ITEM_WEAPON || weapon->carried_by != ch
        || (weapon->wear_loc != WEAR_WEAPON && weapon->wear_loc != WEAR_HOLD)
        || !is_affected(ch,craft_sn("crafted weapon")) || !v || ch == v
        || ch->in_room != v->in_room || IS_BUI(v) || is_same_group(ch,v)
        || (!IS_NPC(v) && (!CAN_PK(ch) || !CAN_PK(v))) || is_safe(ch,v)) return 0;
    if (!saves_spell(60,v) && !is_affected(v,gsn_parent_chill)) {
        AFFECT_DATA af;
        memset(&af,0,sizeof(af)); af.type=gsn_parent_chill; af.duration=2;
        af.location=APPLY_STR; af.modifier=-1; affect_to_char(v,&af);
    }
    return 150;
}

static void crafted_command(CHAR_DATA *ch, char *argument, int parent)
{
    char cmd[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH], extra[MAX_INPUT_LENGTH], buf[256];
    const CRAFTED_POWER *p = NULL;
    CRAFTED_STATE *s;
    CHAR_DATA *v;
    OBJ_DATA *obj;
    ROOM_INDEX_DATA *to;
    size_t i;
    long amount, heal, cost;
    int saved;
    const char *effect = NULL;
    if (crafted_class_parent(ch) != parent) {
        if (ch) stc("That is not your standalone class.\n\r",ch);
        return;
    }
    if (!argument || strlen(argument) >= sizeof(cmd)) { stc("Input too long.\n\r",ch); return; }
    if (!craft_valid(ch)) { stc("Invalid class state; contact an administrator.\n\r",ch); return; }
    s = &ch->pcdata->crafted;
    argument=one_argument(argument,cmd); argument=one_argument(argument,arg); one_argument(argument,extra);
    if (extra[0]) { stc("Use one ability and at most one target.\n\r",ch); return; }
    if (!cmd[0] || !strcmp(cmd,"status")) {
        if (arg[0]) return;
        snprintf(buf,sizeof(buf),"Mastery %ld/8; %s %ld/%ld. train; focus <mana>.\n\r",
            s->rank,parent == GEN_PARENT_MOOGLE ? "pom energy" : "soul energy",s->energy,CRAFTED_ENERGY_CAP);
        stc(buf,ch);
        class_training_header(ch,"MASTERY TRAINING");
        class_training_row(ch,"train",s->rank,8,
            class_training_cost(s->rank,8,50000,TRUE),"XP",ch->exp,NULL);
        stc("Power                 Rank    Mana  Energy  Mastery\n\r"
            "--------------------  ----  ------  ------  -------\n\r",ch);
        for (i=0;i<sizeof(crafted_powers)/sizeof(crafted_powers[0]);++i)
            if (crafted_powers[i].parent == parent) {
                p=&crafted_powers[i];
                snprintf(buf,sizeof(buf),"%-20s  %4d  %6ld  %6ld  %s\n\r",
                    p->key,p->rank,p->mana,p->energy,s->rank>=p->rank ? "Known" : "Locked");
                stc(buf,ch);
            }
        stc("Known = mastery met; resources, targets and cooldowns still apply.\n\r"
            "Genesis reconstruction, not recovered ADG code. Hybrids remain disabled.\n\r",ch);
        return;
    }
    if (!strcmp(cmd,"train")) {
        if (arg[0]) { stc("Syntax: <class> train (no target).\n\r",ch); return; }
        if (!craft_peace(ch)) {
            stc("Training requires peace and standing in your living, natural body.\n\r",ch); return;
        }
        if (s->rank>=8) { stc("Your class mastery is already complete (8/8).\n\r",ch); return; }
        if (is_affected(ch,craft_sn("crafted recovery"))) {
            stc("Your class power is still recovering; wait before training.\n\r",ch); return;
        }
        cost=50000*(s->rank+1)*(s->rank+1);
        if (ch->exp < cost) { stc("Not enough experience.\n\r",ch); return; }
        ch->exp-=cost; ++s->rank; WAIT_STATE(ch,12); crafted_restore(ch);
        snprintf(buf,sizeof(buf),"Your class mastery improves: %ld/8; spent %ld XP.\n\r",s->rank,cost);
        stc(buf,ch); return;
    }
    if (!strcmp(cmd,"focus")) {
        amount=craft_number(arg);
        if (!amount) { stc("Syntax: focus <positive mana amount>, at most 10000.\n\r",ch); return; }
        if (!craft_peace(ch)) {
            stc("Focus requires peace and standing in your living, natural body.\n\r",ch); return;
        }
        if (is_affected(ch,craft_sn("crafted recovery"))) {
            stc("Your class power is still recovering; wait before focusing.\n\r",ch); return;
        }
        if (amount>ch->mana || amount>CRAFTED_ENERGY_CAP-s->energy) {
            stc("You lack that much mana or enough free class-energy capacity.\n\r",ch); return;
        }
        ch->mana-=amount; s->energy+=amount; WAIT_STATE(ch,12);
        combat_recovery_start(ch,craft_sn("crafted recovery"),1);
        stc("You convert mana into class energy at one to one.\n\r",ch); return;
    }
    for (i=0;i<sizeof(crafted_powers)/sizeof(crafted_powers[0]);++i)
        if (crafted_powers[i].parent == parent && !strcmp(crafted_powers[i].key,cmd)) {
            p=&crafted_powers[i]; break;
        }
    if (!p) { stc("Unknown class ability; use your class name for the menu.\n\r",ch); return; }
    if (!craft_ready(ch,p)) return;
    if (p->kind == CP_HARVEST) {
        obj=arg[0] ? get_obj_here(ch,arg) : NULL;
        if (!obj || obj->in_room != ch->in_room || obj->item_type != ITEM_CORPSE_NPC
            || obj->chobj || IS_SET(obj->quest,REF_QUEST_SOUL_CLAIMED)
            || IS_SET(ch->in_room->room_flags,ROOM_SAFE) || s->energy == CRAFTED_ENERGY_CAP) return;
        amount=UMIN(p->amount,100+10*URANGE(0,obj->level,90));
        craft_commit(ch,p); s->energy+=UMIN(amount,CRAFTED_ENERGY_CAP-s->energy);
        SET_BIT(obj->quest,REF_QUEST_SOUL_CLAIMED);
        stc("You harvest residual soul energy without destroying the corpse or loot.\n\r",ch); return;
    }
    if (p->kind == CP_HOME) {
        to=ch->home > 0 ? get_room_index(ch->home) : NULL;
        if (arg[0] || !craft_peace(ch) || !to || to == ch->in_room
            || IS_AFFECTED(ch,AFF_CURSE|AFF_WEBBED|AFF_SHADOWPLANE) || is_affected(ch,gsn_lineage_root)
            || IS_SET(ch->in_room->room_flags,ROOM_NO_RECALL|ROOM_NO_TELEPORT)
            || IS_SET(to->room_flags,ROOM_PRIVATE|ROOM_SOLITARY|ROOM_NO_RECALL|ROOM_NO_TELEPORT)) return;
        craft_commit(ch,p); char_from_room(ch); char_to_room(ch,to);
        stc("You take a careful kupostep home.\n\r",ch); return;
    }
    if (p->kind == CP_HEAL || p->kind == CP_SERENADE || p->kind == CP_CHEER) {
        v=craft_ally(ch,arg);
        if (!v) return;
        if (p->kind == CP_CHEER) {
            if (is_affected(v,craft_sn("crafted morale"))) return;
            craft_commit(ch,p);
            craft_affect(v,"crafted morale",APPLY_HITROLL,p->amount,3,0);
            craft_affect(v,"crafted morale",APPLY_DAMROLL,p->amount,3,0);
        } else {
            heal=UMIN(p->amount,UMIN(UMAX(0,v->max_hit-v->hit),UMAX(0,v->max_hit/10)));
            if (!heal && !(p->kind == CP_SERENADE && is_affected(v,gsn_poison))) return;
            craft_commit(ch,p); v->hit+=heal;
            if (p->kind == CP_SERENADE) affect_strip(v,gsn_poison);
            update_pos(v);
        }
        act("Your class power supports $N.",ch,NULL,v,TO_CHAR); return;
    }
    switch(p->kind) {
    case CP_WARD: effect="crafted ward"; break;
    case CP_WINGS: effect="crafted wings"; break;
    case CP_FORTUNE: effect="crafted fortune"; break;
    case CP_DANCE: effect="crafted dance"; break;
    case CP_WEAPON: effect="crafted weapon"; break;
    case CP_MIGHT: effect="crafted might"; break;
    default: break;
    }
    if (effect) {
        if (arg[0] || is_affected(ch,craft_sn(effect))) return;
        if (p->kind == CP_WEAPON) {
            obj=get_eq_char(ch,WEAR_WEAPON);
            if (!obj || obj->item_type != ITEM_WEAPON) obj=get_eq_char(ch,WEAR_HOLD);
            if (!obj || obj->item_type != ITEM_WEAPON) return;
        }
        craft_commit(ch,p);
        if (p->kind == CP_WINGS) craft_affect(ch,effect,APPLY_NONE,0,3,AFF_FLYING);
        else if (p->kind == CP_WARD || p->kind == CP_FORTUNE) {
            craft_affect(ch,effect,APPLY_AC,p->amount,3,0);
            craft_affect(ch,effect,APPLY_SAVING_SPELL,p->kind == CP_FORTUNE ? -10 : -5,3,0);
        } else if (p->kind == CP_DANCE) {
            craft_affect(ch,effect,APPLY_HITROLL,p->amount,3,0);
            craft_affect(ch,effect,APPLY_DAMROLL,20,3,0);
            craft_affect(ch,effect,APPLY_AC,-10,3,0);
        } else if (p->kind == CP_MIGHT) {
            craft_affect(ch,effect,APPLY_STR,1,3,0);
            craft_affect(ch,effect,APPLY_DAMROLL,p->amount,3,0);
        } else craft_affect(ch,effect,APPLY_NONE,p->amount,3,0);
        stc("Your class power takes effect.\n\r",ch); return;
    }
    if (p->kind == CP_LASTSTAND) {
        if (arg[0] || ch->max_hit <= 0 || ch->hit > ch->max_hit/4) return;
        heal=UMIN(p->amount,UMIN(ch->max_hit/10,UMAX(0,ch->max_hit-ch->hit)));
        if (heal <= 0) return;
        craft_commit(ch,p); ch->hit+=heal; update_pos(ch);
        stc("Soul energy sustains your last stand.\n\r",ch); return;
    }
    v=craft_enemy(ch,arg);
    if (!v || !hurt_fighting(ch,v)) return;
    saved=saves_spell(30+s->rank*5,v);
    craft_commit(ch,p);
    if (p->kind == CP_FEAR) {
        if (!saved && !is_affected(v,craft_sn("crafted fear")))
            craft_affect(v,"crafted fear",APPLY_HITROLL,-p->amount,2,0);
        return;
    }
    if (p->kind == CP_FLARE && !saved && !IS_AFFECTED(v,AFF_BLIND)) {
        AFFECT_DATA af;
        memset(&af,0,sizeof(af)); af.type=gsn_blindness; af.duration=1;
        af.location=APPLY_HITROLL; af.modifier=-4; af.bitvector=AFF_BLIND;
        affect_to_char(v,&af);
    }
    if ((p->kind == CP_REND || p->kind == CP_DOOM) && !saved
        && !is_affected(v,craft_sn(p->kind == CP_REND ? "crafted frailty" : "crafted doom")))
        craft_affect(v,p->kind == CP_REND ? "crafted frailty" : "crafted doom",
            p->kind == CP_REND ? APPLY_SAVING_SPELL : APPLY_DAMROLL,p->kind == CP_REND ? 10 : -15,2,0);
    amount=p->amount+s->rank*50;
    if (saved) amount/=2;
    damage(ch,v,amount,(p->kind == CP_MELEE || p->kind == CP_DOOM) ? TYPE_HIT : TYPE_SPELL,(char *)p->name);
    /* No dereferences after the native combat/death pipeline. */
}

void do_moogle(CHAR_DATA *ch,char *argument) { crafted_command(ch,argument,GEN_PARENT_MOOGLE); }
void do_deathknight(CHAR_DATA *ch,char *argument) { crafted_command(ch,argument,GEN_PARENT_DEATHKNIGHT); }
#include "lineages.inc"
