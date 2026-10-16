/*
 * Genesis-native interpretations of incompatible reference Demon systems.
 * Saved authority is parent-scoped, validated and never a reused legacy flag.
 * Public command names are independent of the saved grant names.
 */
typedef struct legacy_power { const char *key; int rank; long mana; } LEGACY_POWER;
static const LEGACY_POWER legacy_powers[] = {
    {"firewall",4,1000}, {"entomb",6,2000}, {"infernal_portal",5,1500},
    {"chaosportal",6,1500}, {"dgate",7,2000}, {"baal",8,2500},
    {"fear",4,500}, {"confuse",4,600}, {"immolate",3,500},
    {"freezeweapon",3,500}, {"caust",3,500}, {"daemonseed",7,1000},
    {"leech",4,600}, {"blink",4,500}, {"graft",5,1000},
    {"deathsense",4,500}, {"gust",4,750}, {"bane",5,800},
    {"polymorph",3,500}, {"shadowdemon",5,1500}, {"destroy",6,1000},
    {"evileye",4,600}, {"stealsoul",3,500}, {"cone",5,800},
    {"dstake",3,0}, {"pact",4,0}, {"offersoul",0,0}, {"champions",0,0},
    {"inpart",0,0}, {"warps",0,0}, {"obtain",3,0}, {"redeem",3,0},
    {"clearwarps",0,0}, {"gifts",0,0}, {"souls",0,0},
    {"dbreath",3,600}, {"dimpsend",4,500}, {NULL,0,0}
};

typedef struct legacy_warp_info { const char *key; int location, amount; } LEGACY_WARP_INFO;
/* Stable bit assignments; do not reorder. Cosmetic warps intentionally have no modifier. */
static const LEGACY_WARP_INFO legacy_warps[30] = {
    {"chitin",APPLY_AC,20}, {"scales",APPLY_AC,20},
    {"strongarms",APPLY_STR,1}, {"stronglegs",APPLY_DEX,1},
    {"venomtongue",APPLY_NONE,0}, {"spiketail",APPLY_DAMROLL,10},
    {"badbreath",APPLY_NONE,0}, {"quickness",APPLY_DEX,1},
    {"stamina",APPLY_CON,1}, {"regeneration",APPLY_NONE,0},
    {"hunt",APPLY_NONE,0}, {"damcap",APPLY_NONE,0},
    {"terror",APPLY_SAVING_SPELL,-5}, {"horns",APPLY_HITROLL,10},
    {"clumsy",APPLY_HITROLL,-10}, {"vulnerable",APPLY_SAVING_SPELL,10},
    {"slow",APPLY_DEX,-1}, {"weak",APPLY_STR,-1},
    {"stupid",APPLY_INT,-1}, {"infirm",APPLY_CON,-1},
    {"gummy",APPLY_AC,-20}, {"sluggish",APPLY_NONE,0},
    {"wings",APPLY_NONE,0}, {"shards",APPLY_DAMROLL,15},
    {"saladbowl",APPLY_NONE,0}, {"spoon",APPLY_NONE,0},
    {"fork",APPLY_NONE,0}, {"knife",APPLY_NONE,0},
    {"glow",APPLY_SAVING_SPELL,5}, {"longtongue",APPLY_NONE,0}
};

static int legacy_parent(CHAR_DATA *ch)
{
    return ch && !IS_NPC(ch) && ch->pcdata
        ? IS_ANGEL(ch) ? GEN_PARENT_ANGEL : IS_DEMON(ch) ? GEN_PARENT_DEMON : 0
        : 0;
}

static int legacy_slot(CHAR_DATA *ch, const char *key)
{
    size_t i;
    int id = legacy_parent(ch);
    if (!id || !key) return -1;
    for (i = 0; i < genesis_parent_power_count(id) && i < 128; ++i)
        if (!strcmp(genesis_parent_power_at(id, i)->key, key)) return (int)i;
    return -1;
}

static int legacy_warp(CHAR_DATA *ch, int bit)
{
    return ch && !IS_NPC(ch) && ch->pcdata && IS_DEMON(ch) && bit >= 0 && bit < 30
        && reference_legacy_valid(&ch->pcdata->legacy)
        && (ch->pcdata->legacy.warps & (UINT32_C(1) << bit)) != 0;
}

static int reference_granted(CHAR_DATA *ch, const char *key)
{
    int slot;
    uint32_t gifts;
    if (!ch || IS_NPC(ch) || !ch->pcdata || !reference_legacy_valid(&ch->pcdata->legacy)) return 0;
    gifts = IS_DEMON(ch) ? ch->pcdata->legacy.gifts : 0;
    if ((!strcmp(key, "dstake") && (gifts & 4)) || (!strcmp(key, "cone") && (gifts & 8))
        || (!strcmp(key, "demonform") && (gifts & 16))) return 1;
    slot = legacy_slot(ch, key);
    return slot >= 0 && (ch->pcdata->legacy.grants[slot/64] & (UINT64_C(1) << (slot%64))) != 0;
}

int newclasses_read_grant(CHAR_DATA *ch, FILE *fp)
{
    char line[80];
    int slot;
    const GENESIS_PARENT_POWER *power;
    if (!reference_line(fp, line, sizeof(line)) || strncmp(line, "1 ", 2)) return 0;
    slot = legacy_slot(ch, line + 2);
    if (slot < 0) return 0;
    power = genesis_parent_power_at(legacy_parent(ch), slot);
    if (power->kind == GEN_POWER_SYSTEM) return 0;
    ch->pcdata->legacy.grants[slot/64] |= UINT64_C(1) << (slot%64);
    return 1;
}

void newclasses_write_grants(CHAR_DATA *ch, FILE *fp)
{
    size_t i;
    int id = legacy_parent(ch);
    if (!fp || !id) return;
    for (i = 0; i < genesis_parent_power_count(id) && i < 128; ++i)
        if (ch->pcdata->legacy.grants[i/64] & (UINT64_C(1) << (i%64)))
            fprintf(fp, "ParentGrant 1 %s\n", genesis_parent_power_at(id, i)->key);
}

static void legacy_shape_clear(CHAR_DATA *ch)
{
    if (!ch || IS_NPC(ch) || !ch->pcdata) return;
    if (is_affected(ch, reference_sn("parent shape"))) {
        affect_strip(ch, reference_sn("parent shape"));
        if (!ch->pcdata->chobj) {
            REMOVE_BIT(ch->affected_by, AFF_POLYMORPH);
            free_string(ch->morph); ch->morph = str_dup("");
        }
    }
}

static void legacy_clear(CHAR_DATA *ch)
{
    static const char *names[] = {"parent legacy","parent baal","parent graft",
        "parent blink","parent deathvision","parent shadow",NULL};
    AFFECT_DATA *af, *next;
    int i;
    if (!ch || IS_NPC(ch) || !ch->pcdata) return;
    legacy_shape_clear(ch);
    for (i = 0; names[i]; ++i) affect_strip(ch, reference_sn(names[i]));
    for (af = ch->affected; af; af = next) {
        next = af->next;
        if (af->type == gsn_parent_wings && af->duration < 0) affect_remove(ch, af);
    }
}

static void legacy_sync(CHAR_DATA *ch)
{
    int i, sn = reference_sn("parent legacy");
    AFFECT_DATA *af, *next;
    if (!ch || IS_NPC(ch) || !ch->pcdata) return;
    affect_strip(ch, sn);
    /* Only our permanent wing grant is rebuilt; ordinary flight is not stripped. */
    for (af = ch->affected; af; af = next) {
        next = af->next;
        if (af->type == gsn_parent_wings && af->duration < 0) affect_remove(ch, af);
    }
    if (!IS_DEMON(ch) || !reference_legacy_valid(&ch->pcdata->legacy)) return;
    if (ch->pcdata->legacy.gifts & 1) parent_affect(ch, sn, APPLY_STR, 1, -1, 0);
    for (i = 0; i < 30; ++i)
        if (legacy_warp(ch, i) && legacy_warps[i].location != APPLY_NONE)
            parent_affect(ch, sn, legacy_warps[i].location, legacy_warps[i].amount, -1, 0);
    if (legacy_warp(ch, 22))
        parent_affect(ch, gsn_parent_wings, APPLY_NONE, 0, -1, AFF_FLYING);
}

static void legacy_tick(CHAR_DATA *ch)
{
    if (!ch || IS_NPC(ch) || !ch->pcdata || !IS_DEMON(ch)) return;
    if (legacy_warp(ch, 9) && ch->position > POS_STUNNED)
        ch->hit += UMIN(25, UMAX(0, ch->max_hit - ch->hit));
}

long newclasses_speed_bonus(CHAR_DATA *ch)
{
    if (!ch || IS_NPC(ch) || !ch->pcdata || !IS_DEMON(ch)
        || !reference_legacy_valid(&ch->pcdata->legacy)) return 0;
    return ((ch->pcdata->legacy.gifts & 2) != 0) + legacy_warp(ch, 7)
        - legacy_warp(ch, 21) + (ch->pcdata->reference_familiar == 6
            && ch->pcdata->familiar && ch->pcdata->familiar->in_room == ch->in_room);
}

long newclasses_cap_bonus(CHAR_DATA *ch) { return legacy_warp(ch, 11) ? 300 : 0; }

void newclasses_item_tick(OBJ_DATA *obj)
{
    if (!obj) return;
    if (obj->reference_enchant < 0 || obj->reference_enchant > 15
        || obj->reference_ticks < 0 || obj->reference_ticks > 12
        || !obj->reference_enchant || obj->reference_ticks == 0) {
        obj->reference_enchant = obj->reference_ticks = 0; return;
    }
    if (--obj->reference_ticks == 0) {
        if (obj->reference_enchant & 8) obj->timer = 1;
        obj->reference_enchant = 0;
    }
}

long newclasses_weapon_bonus(CHAR_DATA *ch, CHAR_DATA *v, OBJ_DATA *weapon)
{
    long flags = weapon && weapon->reference_ticks > 0 && weapon->reference_ticks <= 12
        && weapon->reference_enchant > 0 && weapon->reference_enchant <= 15
        ? weapon->reference_enchant : 0;
    long bonus = 0;
    if (!ch || !v || !ch->in_room || v->in_room != ch->in_room || ch == v
        || IS_BUI(v) || is_same_group(ch, v)
        || (!IS_NPC(ch) && !IS_NPC(v) && (!CAN_PK(ch) || !CAN_PK(v))) || is_safe(ch, v))
        return 0;
    if (flags & 1) bonus += 150;
    if (weapon && (weapon->quest & REF_QUEST_STAKE) && IS_VAMPIRE(v)) bonus += 250;
    if (flags & 2) {
        bonus += 100;
        if (!is_affected(v, gsn_parent_chill))
            parent_affect(v, gsn_parent_chill, APPLY_STR, -1, 2, 0);
    }
    if (((flags & 4) || (legacy_warp(ch, 4) && number_percent() <= 10))
        && !IS_AFFECTED(v, AFF_POISON) && !saves_spell(40, v))
        parent_affect(v, gsn_poison, APPLY_STR, -1, 2, AFF_POISON);
    if (flags & 8) {
        bonus += 500; weapon->reference_enchant = weapon->reference_ticks = 0;
        weapon->timer = 1; /* Native object expiry, never free a wield pointer mid-hit. */
    }
    return bonus;
}

void newclasses_extra_attack(CHAR_DATA *ch, CHAR_DATA *v)
{
    if (ch && v && !IS_NPC(ch) && IS_DEMON(ch) && ch->position > POS_STUNNED
        && ch->fighting == v && is_affected(ch, reference_sn("parent graft")))
        one_hit(ch, v, TYPE_HIT, "grafted arm");
}

static void legacy_offer(CHAR_DATA *ch, char *arg)
{
    if (!ch || IS_NPC(ch) || !ch->pcdata || ch->class != CLASS_NONE || !management_ready(ch)) return;
    if (!strcmp(arg, "on")) {
        ch->pcdata->legacy.willing = 1;
        stc("You consent to a Demon converting you into a Demon champion. Use demon offersoul off to withdraw.\n\r", ch);
    } else if (!strcmp(arg, "off")) {
        ch->pcdata->legacy.willing = 0; stc("Your soul is no longer offered.\n\r", ch);
    } else stc("demon offersoul on/off: explicit, revocable consent to Demon enrollment.\n\r", ch);
}

static void legacy_menu(CHAR_DATA *ch)
{
    const LEGACY_POWER *p;
    char buf[160];
    if (IS_ANGEL(ch)) { stc("inpart <party member> <ability>: share a learned Angel power.\n\r", ch); return; }
    if (!IS_DEMON(ch)) return;
    for (p = legacy_powers; p->key; ++p) {
        snprintf(buf, sizeof(buf), "%-20s rank %d, mana %ld\n\r", p->key, p->rank, p->mana);
        stc(buf, ch);
    }
    stc("Use souls <XP> for soul power; warps/gifts/champions show their own state.\n\r", ch);
}

static int legacy_wall(CHAR_DATA *ch, ROOM_INDEX_DATA *room, int dir, int ice)
{
    OBJ_DATA *wall;
    OBJ_INDEX_DATA *index = get_obj_index(OBJ_VNUM_PORTAL);
    EXIT_DATA *exit;
    if (!index || !room || dir < 0 || dir > 5 || !(exit = room->exit[dir]) || !exit->to_room
        || IS_SET(room->room_flags | exit->to_room->room_flags,
            ROOM_SAFE | ROOM_PRIVATE | ROOM_SOLITARY | ROOM_NO_TELEPORT)
        || IS_SET(exit->exit_info, EX_CLOSED | EX_WALL_WATER | EX_WALL_WIND | EX_WALL_FLAME | EX_WALL_GRANITE))
        return 0;
    wall = create_object(index, 0);
    free_string(wall->name); free_string(wall->short_descr); free_string(wall->description);
    wall->name = str_dup(ice ? "infernal ice wall" : "infernal fire wall");
    wall->short_descr = str_dup(ice ? "a wall of infernal ice" : "a wall of infernal fire");
    wall->description = str_dup(ice ? "An icy barrier blocks an exit." : "A flaming barrier blocks an exit.");
    wall->item_type = ITEM_WALL; wall->wear_flags = 0; wall->timer = 3;
    wall->value[0] = dir; wall->quest = QUEST_VANISH;
    SET_BIT(exit->exit_info, ice ? EX_WALL_GRANITE : EX_WALL_FLAME);
    obj_to_room(wall, room);
    return 1;
}

static int legacy_teleport_ok(CHAR_DATA *ch, ROOM_INDEX_DATA *to)
{
    return ch && !IS_NPC(ch) && ch->pcdata && management_ready(ch) && ch->in_room && to
        && !IS_AFFECTED(ch, AFF_CURSE | AFF_WEBBED | AFF_SHADOWPLANE)
        && !IS_EXTRA(ch, EXTRA_OSWITCH) && !IS_MORE(ch, MORE_HEAD)
        && !is_affected(ch,gsn_lineage_root)
        && !IS_SET(ch->in_room->room_flags | to->room_flags,
            ROOM_PRIVATE | ROOM_SOLITARY | ROOM_NO_TELEPORT | ROOM_SAFE | ROOM_NO_RECALL)
        && !IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
        && !IS_BUI(ch);
}

static void legacy_travel(CHAR_DATA *ch, const char *key, char *arg, long mana)
{
    CHAR_DATA *v;
    ROOM_INDEX_DATA *to, *choices[6];
    int i, n = 0;
    if (!strcmp(key, "chaosportal")) {
        if (arg[0]) { stc("chaosportal takes no target.\n\r", ch); return; }
        for (i = 0; i < 6; ++i)
            if (ch->in_room->exit[i] && !IS_SET(ch->in_room->exit[i]->exit_info,
                EX_CLOSED | EX_WALL_WATER | EX_WALL_GRANITE | EX_WALL_WIND | EX_WALL_FLAME)
                && legacy_teleport_ok(ch, ch->in_room->exit[i]->to_room))
                choices[n++] = ch->in_room->exit[i]->to_room;
        if (!n || !power_ready(ch, mana, 0)) return;
        to = choices[number_range(0, n-1)];
    } else {
        v = arg[0] ? get_char_world(ch, arg) : NULL;
        if (!v || v == ch || IS_NPC(v) || !v->pcdata || !is_same_group(ch, v)
            || !can_see(ch, v) || !can_see(v, ch) || IS_BUI(v)
            || IS_AFFECTED(v, AFF_SHADOWPLANE) != IS_AFFECTED(ch, AFF_SHADOWPLANE)
            || !legacy_teleport_ok(ch, v->in_room) || !legacy_teleport_ok(v, ch->in_room)
            || !power_ready(ch, mana, 0)) {
            stc("Portals require a willing, peaceful party member and unwarded public rooms.\n\r", ch); return;
        }
        if (!strcmp(key, "dgate")) {
            power_commit(ch, mana, 0, 1); char_from_room(v); char_to_room(v, ch->in_room);
            stc("Your ally draws you through a closed infernal gate.\n\r", v); return;
        }
        to = v->in_room;
    }
    power_commit(ch, mana, 0, 1); char_from_room(ch); char_to_room(ch, to);
    stc("You cross a closed infernal portal.\n\r", ch);
}

static void legacy_shape(CHAR_DATA *ch, char *arg, int shadow, long mana)
{
    static const char *animals[] = {"wolf","bat","serpent","bear","raven"};
    static const char *demons[] = {"lemure","nupperibo","spinagon","barbazu","abishai",
        "osyluth","hamatula","erinyes","amnizu","cornugon","gelugon"};
    int i, n = shadow ? 11 : 5, sn = reference_sn("parent shape");
    char buf[160];
    if (!strcmp(arg, "back")) { legacy_shape_clear(ch); stc("You resume your own shape.\n\r", ch); return; }
    for (i = 0; i < n; ++i) if (!strcmp(arg, shadow ? demons[i] : animals[i])) break;
    if (i == n) {
        stc(shadow ? "Forms: lemure nupperibo spinagon barbazu abishai osyluth hamatula erinyes amnizu cornugon gelugon; back.\n\r"
            : "Forms: wolf bat serpent bear raven; back.\n\r", ch); return;
    }
    if (IS_AFFECTED(ch, AFF_POLYMORPH) || is_affected(ch, sn) || !power_ready(ch, mana, 0)) return;
    power_commit(ch, mana, 0, 1);
    parent_affect(ch, sn, APPLY_DAMROLL, shadow ? 10 + i * 2 : (i == 3 ? 25 : 10), 3, AFF_POLYMORPH);
    parent_affect(ch, sn, APPLY_AC, shadow ? 30 - i * 2 : (i == 1 || i == 4 ? 30 : 10), 3, 0);
    snprintf(buf, sizeof(buf), "%s the %s", ch->name, shadow ? demons[i] : animals[i]);
    free_string(ch->morph); ch->morph = str_dup(buf);
}

static int legacy_command(CHAR_DATA *ch, const char *cmd, char *arg, char *extra)
{
    const LEGACY_POWER *p;
    REFERENCE_LEGACY *s = &ch->pcdata->legacy;
    CHAR_DATA *v;
    OBJ_DATA *obj;
    OBJ_INDEX_DATA *index;
    const GENESIS_PARENT_POWER *power;
    char buf[256], line[80];
    long cost, amount, flags, before;
    int i, n, slot, candidates[30], sn, saved, dir = -1, count = 0;
    ROOM_INDEX_DATA *room;
    for (p = legacy_powers; p->key && strcmp(p->key, cmd); ++p) {}
    if (!p->key || (!IS_DEMON(ch) && !(IS_ANGEL(ch) && !strcmp(cmd, "inpart")))) return 0;
    if (!reference_legacy_valid(s) || IS_EXTRA(ch, EXTRA_OSWITCH)
        || IS_MORE(ch, MORE_HEAD) || ch->position <= POS_STUNNED) {
        stc("Your present state cannot use this power.\n\r", ch); return 1;
    }
    if (extra[0] && strcmp(cmd, "inpart") && strcmp(cmd, "dimpsend") && strcmp(cmd, "gust")) {
        stc("Too many ability arguments.\n\r", ch); return 1;
    }
    if (parent_rank(ch) < p->rank && !reference_granted(ch, cmd)) {
        stc("You have not mastered that ability.\n\r", ch); return 1;
    }
    if (!strcmp(cmd, "offersoul")) { stc("Only an unclassed player can offer their soul.\n\r", ch); return 1; }
    if (!strcmp(cmd, "souls")) {
        if (!arg[0]) {
            snprintf(buf, sizeof(buf), "Soul power: %ld/%ld. souls <XP> converts experience 1:1.\n\r", s->souls, REF_SOUL_CAP);
            stc(buf, ch); return 1;
        }
        amount = positive_number(arg);
        if (!amount || !management_ready(ch) || amount > ch->exp || amount > REF_SOUL_CAP - s->souls) return 1;
        ch->exp -= amount; s->souls += amount; WAIT_STATE(ch, 12); return 1;
    }
    if (!strcmp(cmd, "champions")) {
        if (arg[0]) return 1;
        snprintf(buf, sizeof(buf), "Your patron: %s.\n\r", s->patron[0] ? s->patron : "none"); stc(buf, ch);
        for (v = char_list; v; v = v->next)
            if (!IS_NPC(v) && v->pcdata && IS_DEMON(v) && can_see(ch, v)
                && !str_cmp(v->pcdata->legacy.patron, ch->name)) {
                snprintf(buf, sizeof(buf), "Champion: %.12s\n\r", v->name); stc(buf, ch);
            }
        return 1;
    }
    if (!strcmp(cmd, "pact")) {
        v = arg[0] ? get_char_room(ch, arg) : NULL;
        if (!management_ready(ch) || !v || v == ch || IS_NPC(v) || !v->pcdata
            || v->class != CLASS_NONE || !reference_legacy_valid(&v->pcdata->legacy)
            || v->pcdata->legacy.willing != 1 || !management_ready(v)
            || v->stats[STAT_STR]<2 || v->stats[STAT_DEX]<2 || v->stats[STAT_CON]<3
            || v->stats[STAT_INT]<3 || v->stats[STAT_WIS]<2
            || v->level != LEV_AWA || IS_BUI(v) || ch->exp < 666 || strlen(ch->name) > 12) {
            stc("A pact needs a peaceful, unclassed awakened volunteer using demon offersoul on, and 666 XP.\n\r", ch); return 1;
        }
        newclasses_clear(v); v->class = CLASS_DEMON;
        memset(v->pcdata->powers, 0, sizeof(v->pcdata->powers));
        snprintf(v->pcdata->legacy.patron, sizeof(v->pcdata->legacy.patron), "%s", ch->name);
        ch->exp -= 666; WAIT_STATE(ch, 12);
        stc("Your offered soul is accepted; you become a Demon champion with zero mastery.\n\r", v); return 1;
    }
    if (!strcmp(cmd, "inpart")) {
        v = arg[0] ? get_char_room(ch, arg) : NULL;
        slot = legacy_slot(ch, extra);
        power = slot >= 0 ? genesis_parent_power_at(legacy_parent(ch), slot) : NULL;
        if (!power || power->kind == GEN_POWER_SYSTEM || !v || v == ch || IS_NPC(v) || !v->pcdata
            || !reference_legacy_valid(&v->pcdata->legacy)
            || v->class != ch->class || !is_same_group(ch, v) || !management_ready(ch)
            || !management_ready(v) || IS_BUI(v) || ch->exp < 50000
            || (parent_rank(ch) < 8 && !reference_granted(ch, extra))
            || reference_granted(v, extra)) {
            stc("inpart <peaceful party member> <ability>: same parent, learned power, 50000 XP; no system grants.\n\r", ch); return 1;
        }
        v->pcdata->legacy.grants[slot/64] |= UINT64_C(1) << (slot%64);
        ch->exp -= 50000; parent_traits_sync(v); legacy_sync(v); WAIT_STATE(ch, 12);
        stc("Your ally imparts a permanent parent-scoped ability.\n\r", v); return 1;
    }
    if (!strcmp(cmd, "warps")) {
        if (arg[0]) { stc("Use obtain, redeem <negative warp>, or clearwarps confirm.\n\r", ch); return 1; }
        for (i = 0; i < 30; ++i) if (legacy_warp(ch, i)) {
            snprintf(buf, sizeof(buf), "%s%s\n\r", legacy_warps[i].key,
                legacy_warps[i].location == APPLY_NONE ? " (special or cosmetic)" : "");
            stc(buf, ch); ++count;
        }
        snprintf(buf, sizeof(buf), "Warps: %d/10. Obtain costs 15000 soul power; clearing costs 20000 per warp.\n\r", count);
        stc(buf, ch); return 1;
    }
    if (!strcmp(cmd, "obtain") || !strcmp(cmd, "redeem") || !strcmp(cmd, "clearwarps")) {
        if (!management_ready(ch)) return 1;
        for (i = 0; i < 30; ++i) if (legacy_warp(ch, i)) ++count;
        if (!strcmp(cmd, "clearwarps")) {
            cost = 20000L * count;
            if (strcmp(arg, "confirm") || !count || s->souls < cost) {
                stc("clearwarps confirm permanently removes your warps; costs 20000 soul power each, no refund.\n\r", ch); return 1;
            }
            s->souls -= cost; s->warps = 0;
        } else if (!strcmp(cmd, "redeem")) {
            for (i = 14; i <= 21; ++i) if (!strcmp(arg, legacy_warps[i].key)) break;
            if (i > 21 || !legacy_warp(ch, i) || s->souls < 10000) return 1;
            s->souls -= 10000; s->warps &= ~(UINT32_C(1) << i);
        } else {
            if (arg[0] || count >= 10 || s->souls < 15000) return 1;
            for (i = n = 0; i < 30; ++i) if (!legacy_warp(ch, i)) candidates[n++] = i;
            if (!n) return 1;
            s->souls -= 15000; s->warps |= UINT32_C(1) << candidates[number_range(0,n-1)];
        }
        legacy_sync(ch); WAIT_STATE(ch, 12); return 1;
    }
    if (!strcmp(cmd, "gifts")) {
        static const char *names[] = {"strength","speed","stake","cone","form"};
        for (i = 0; i < 5; ++i) if (!strcmp(arg, names[i])) break;
        if (i == 5) {
            for (i = 0; i < 5; ++i) {
                snprintf(buf, sizeof(buf), "%s: %s (50 primal)\n\r", names[i], (s->gifts & (1U<<i)) ? "owned" : "available");
                stc(buf, ch);
            }
            return 1;
        }
        if (!management_ready(ch) || (s->gifts & (1U<<i)) || ch->primal < 50) return 1;
        ch->primal -= 50; s->gifts |= 1U<<i; legacy_sync(ch); WAIT_STATE(ch, 12); return 1;
    }
    if (!strcmp(cmd, "infernal_portal") || !strcmp(cmd, "chaosportal") || !strcmp(cmd, "dgate")) {
        legacy_travel(ch, cmd, arg, p->mana); return 1;
    }
    if (!strcmp(cmd, "polymorph") || !strcmp(cmd, "shadowdemon")) {
        legacy_shape(ch, arg, !strcmp(cmd, "shadowdemon"), p->mana); return 1;
    }
    if (!strcmp(cmd, "dstake")) {
        if (arg[0] || !management_ready(ch) || ch->primal < 60
            || !(index = get_obj_index(OBJ_VNUM_PROTOPLASM)) || ch->carry_number >= 100) return 1;
        for (obj = ch->carrying; obj; obj = obj->next_content)
            if (obj->quest & REF_QUEST_STAKE) return 1;
        obj = create_object(index, 0);
        free_string(obj->name); free_string(obj->short_descr); free_string(obj->description);
        obj->name = str_dup("infernal wooden stake");
        obj->short_descr = str_dup("an infernal wooden stake");
        obj->description = str_dup("An infernal wooden stake lies here.");
        obj->item_type = ITEM_WEAPON; obj->wear_flags = ITEM_TAKE | ITEM_WEAR_HOLD;
        obj->value[0] = 0; obj->value[1] = 10; obj->value[2] = 20; obj->value[3] = TYPE_PIERCE;
        obj->weight = 5; SET_BIT(obj->quest, REF_QUEST_STAKE);
        free_string(obj->questowner); obj->questowner = str_dup(ch->name);
        obj_to_char(obj, ch); ch->primal -= 60; WAIT_STATE(ch, 12); return 1;
    }
    if (!strcmp(cmd, "stealsoul")) {
        obj = arg[0] ? get_obj_here(ch, arg) : NULL;
        if (!obj || obj->in_room != ch->in_room || obj->item_type != ITEM_CORPSE_NPC
            || (obj->quest & REF_QUEST_SOUL_CLAIMED) || IS_SET(ch->in_room->room_flags, ROOM_SAFE)
            || s->souls >= REF_SOUL_CAP || !power_ready(ch, p->mana, 0)) return 1;
        amount = UMIN(1000, 100 + UMAX(0, UMIN(90, obj->level)) * 10);
        power_commit(ch, p->mana, 0, 1); SET_BIT(obj->quest, REF_QUEST_SOUL_CLAIMED);
        s->souls += UMIN(amount, REF_SOUL_CAP - s->souls);
        stc("You claim the remaining soul energy; the corpse and its loot remain.\n\r", ch); return 1;
    }
    if (!strcmp(cmd, "dimpsend")) {
        obj = arg[0] ? get_obj_carry(ch, arg) : NULL;
        v = extra[0] ? get_char_world(ch, extra) : NULL;
        if (!obj || obj->chobj || obj->contains || obj->wear_loc != WEAR_NONE
            || IS_SET(obj->quest, QUEST_ARTIFACT | QUEST_RELIC | QUEST_VANISH)
            || (obj->questowner && obj->questowner[0] && str_cmp(obj->questowner, ch->name))
            || !v || v == ch || IS_NPC(v) || !v->pcdata || !is_same_group(ch, v)
            || get_obj_weight(obj)<0 || get_obj_weight(obj)>10000
            || !can_see(ch, v) || IS_BUI(v) || !legacy_teleport_ok(v, ch->in_room)
            || !legacy_teleport_ok(ch, v->in_room) || v->carry_number >= 100
            || v->carry_weight > 10000 - get_obj_weight(obj)
            || ch->pcdata->reference_familiar != 3 || !ch->pcdata->familiar
            || IS_SET(obj->extra_flags, ITEM_NODROP) || !power_ready(ch, p->mana, 0)) {
            stc("An active imp can courier one unequipped, unbound item to a willing party member.\n\r", ch); return 1;
        }
        power_commit(ch, p->mana, 0, 1); obj_from_char(obj); obj_to_char(obj, v);
        stc("Your ally's imp delivers an item.\n\r", v); return 1;
    }
    if (!strcmp(cmd, "firewall") || !strcmp(cmd, "entomb")) {
        dir = reference_direction(arg);
        if ((!strcmp(cmd,"firewall") && dir < 0) || (!strcmp(cmd,"entomb") && arg[0])
            || !power_ready(ch, p->mana, 0)) return 1;
        room = ch->in_room;
        if (dir >= 0) count = legacy_wall(ch, room, dir, 0);
        else for (i = 0; i < 6; ++i) count += legacy_wall(ch, room, i, 1);
        if (!count) { stc("No eligible open, public exits or wall template.\n\r", ch); return 1; }
        power_commit(ch, p->mana, 0, 1);
        if (dir < 0) ch->hit += UMIN(500, UMAX(0, ch->max_hit - ch->hit));
        return 1;
    }
    if (!strcmp(cmd, "immolate") || !strcmp(cmd, "freezeweapon") || !strcmp(cmd, "caust") || !strcmp(cmd, "daemonseed")) {
        obj = arg[0] ? get_obj_carry(ch, arg) : get_eq_char(ch, WEAR_WEAPON);
        flags = !strcmp(cmd,"immolate") ? 1 : !strcmp(cmd,"freezeweapon") ? 2 : !strcmp(cmd,"caust") ? 4 : 8;
        if (!obj || obj->item_type != ITEM_WEAPON || obj->chobj
            || IS_SET(obj->quest, QUEST_ARTIFACT | QUEST_RELIC)
            || IS_SET(obj->extra_flags, ITEM_BLESS | ITEM_ANTI_EVIL)
            || (obj->questowner && obj->questowner[0] && str_cmp(obj->questowner, ch->name))
            || (obj->reference_enchant & flags) || !power_ready(ch, p->mana, 0)) return 1;
        power_commit(ch, p->mana, 0, 1);
        obj->reference_enchant |= flags; obj->reference_ticks = 12;
        stc(flags == 8 ? "Seed armed: the next legal strike detonates it and the weapon then crumbles.\n\r"
            : "Your weapon holds a temporary infernal enchantment.\n\r", ch); return 1;
    }
    if (!strcmp(cmd, "baal") || !strcmp(cmd, "graft") || !strcmp(cmd, "deathsense")) {
        sn = reference_sn(!strcmp(cmd,"baal") ? "parent baal" : !strcmp(cmd,"graft") ? "parent graft" : "parent deathvision");
        if (is_affected(ch, sn) || !power_ready(ch, p->mana, 0)) return 1;
        if (!strcmp(cmd,"graft")) {
            if (arg[0] || s->souls < 1000) {
                stc("Graft grows an infernal arm for 1000 soul power and 1000 mana.\n\r", ch); return 1;
            }
            s->souls -= 1000;
        } else if (arg[0]) return 1;
        power_commit(ch, p->mana, 0, 1); parent_affect(ch, sn, APPLY_NONE, 0, 3, 0);
        if (!strcmp(cmd,"baal")) {
            parent_affect(ch, sn, APPLY_HITROLL, 30, 3, 0);
            parent_affect(ch, sn, APPLY_DAMROLL, 30, 3, 0);
            parent_affect(ch, sn, APPLY_AC, 30, 3, 0);
        }
        return 1;
    }
    if (!strcmp(cmd,"dbreath") && !legacy_warp(ch,6)) {
        stc("Putrid breath requires the badbreath warp.\n\r", ch); return 1;
    }
    v = parent_enemy(ch, arg);
    if (!v) return 1;
    if (!strcmp(cmd,"destroy") && (!IS_NPC(v) || !v->pIndexData || v->pIndexData->alignment > -350)) return 1;
    if (!strcmp(cmd,"bane") && (!IS_NPC(v) || !v->pIndexData || v->pIndexData->alignment < 350
        || !IS_OUTSIDE(ch))) return 1;
    if (!strcmp(cmd,"gust") && ((dir = reference_direction(extra)) < 0 || !v->in_room->exit[dir]
        || !v->in_room->exit[dir]->to_room
        || IS_SET(v->in_room->exit[dir]->exit_info, EX_CLOSED | EX_WALL_GRANITE | EX_WALL_WIND | EX_WALL_FLAME | EX_WALL_WATER)
        || IS_SET(v->in_room->exit[dir]->to_room->room_flags, ROOM_PRIVATE | ROOM_SOLITARY | ROOM_SAFE | ROOM_NO_MOB))) return 1;
    if (!power_ready(ch, p->mana, 0) || !hurt_fighting(ch,v)) return 1;
    saved = saves_spell(30 + parent_rank(ch)*5, v);
    power_commit(ch, p->mana, 0, 1);
    if (!strcmp(cmd,"fear") || !strcmp(cmd,"confuse")) {
        sn = reference_sn("parent fear");
        if (!saved && !is_affected(v,sn)) parent_affect(v,sn,APPLY_HITROLL,-5,2,0);
        if (!saved) do_flee(v,""); /* Never redirect combat onto an uninvolved person. */
        return 1;
    }
    if (!strcmp(cmd,"gust")) {
        if (!saved && dir >= 0) { stop_fighting(v,TRUE); move_char(v,dir); }
        return 1;
    }
    if (!strcmp(cmd,"evileye")) {
        sn = reference_sn("parent evileye");
        if (!saved && !is_affected(v,sn)) {
            parent_affect(v,sn,APPLY_SAVING_SPELL,10,2,0);
            parent_affect(v,sn,APPLY_HITROLL,-5,2,0);
        }
        return 1;
    }
    amount = 500 + 75 * parent_rank(ch);
    if (saved) amount /= 2;
    if (!strcmp(cmd,"blink") && !is_affected(ch, reference_sn("parent blink")))
        parent_affect(ch, reference_sn("parent blink"), APPLY_AC, 25, 2, 0);
    if (!strcmp(cmd,"leech")) {
        before = UMAX(0, ch->max_hit - ch->hit);
        ch->hit += UMIN(before, UMIN(150, UMIN(amount/4, UMAX(0, v->hit-1))));
    }
    if (!strcmp(cmd,"bane") && !saved && !is_affected(v,reference_sn("parent bane")))
        parent_affect(v,reference_sn("parent bane"),APPLY_HITROLL,-10,2,0);
    if ((!strcmp(cmd,"cone") || !strcmp(cmd,"dbreath")) && !saved && !IS_AFFECTED(v,AFF_POISON))
        parent_affect(v,gsn_poison,APPLY_STR,-1,2,AFF_POISON);
    snprintf(line,sizeof(line),"%.40s",p->key);
    damage(ch,v,amount,TYPE_SPELL,line);
    return 1; /* No combatant access after damage or forced movement. */
}

/* Cosmetic warps and combat aspects must have visible presentation, not just
 * saved bits. Called only after native look/visibility checks find the target.
 */
void newclasses_describe(CHAR_DATA *target, CHAR_DATA *viewer)
{
    char buf[512];
    size_t used;
    int i;
    if (!viewer || !parent_actor(target)) return;
    if (IS_CYBORG(target) && is_affected(target,reference_sn("cyber avatar")))
        stc("Cybernetic configuration: armored combat cube.\n\r",viewer);
    if (IS_DEMON(target) && is_affected(target,reference_sn("parent baal")))
        stc("Infernal aspect: avatar of Baal.\n\r",viewer);
    if (!IS_DEMON(target) || !reference_legacy_valid(&target->pcdata->legacy)
        || !target->pcdata->legacy.warps) return;
    strcpy(buf,"Warped features: "); used=strlen(buf);
    for (i=0;i<30;++i) if (legacy_warp(target,i)) {
        used += snprintf(buf+used,sizeof(buf)-used,"%s%s",
            used>17 ? ", " : "",legacy_warps[i].key);
    }
    snprintf(buf+used,sizeof(buf)-used,".\n\r"); stc(buf,viewer);
}
