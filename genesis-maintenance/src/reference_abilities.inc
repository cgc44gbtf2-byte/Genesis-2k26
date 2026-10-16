/*
 * Original, bounded Genesis adapters for distinct reference mechanics.
 * Included by newclasses.c to share its target/cost/affect safety boundary.
 * Never treat presence in this file as proof of full reference parity.
 */
enum { REF_FACE, REF_LIMBS, REF_BODY, REF_DEMOLITION, REF_BIONICS,
       REF_BOMB, REF_ARM, REF_DETONATE, REF_MINE, REF_LAUNCH,
       REF_STUN, REF_CUBE, REF_SIGHT, REF_SCRY, REF_AURA, REF_LLOTH,
       REF_DARK, REF_WEB, REF_TRANSMIT, REF_CLOAK, REF_EQUIPMENT,
       REF_PASSIVE, REF_EYE, REF_ASTRAL, REF_COMPANION, REF_GUARDIAN,
       REF_SERVANT, REF_UNVEIL, REF_WEAPONFORM, REF_TRANSFORM,
       REF_HUMANFORM, REF_OBJECTMOVE, REF_OBJECTLEAP, REF_OBJECTMAGIC,
       REF_LIFESPAN, REF_STEED };
typedef struct reference_power {
    const char *key;
    int classes; /* Angel=1, Demon=2, Cyborg=4 */
    int kind, system, rank;
    long mana, nano;
} REFERENCE_POWER;

static const REFERENCE_POWER reference_powers[] = {
    {"face_implants",4,REF_FACE,0,0,0,0},
    {"limb_implants",4,REF_LIMBS,0,0,0,0},
    {"body_implants",4,REF_BODY,0,0,0,0},
    {"demolitions",4,REF_DEMOLITION,0,0,0,0},
    {"bionics",4,REF_BIONICS,0,0,0,0},
    {"make_bomb",4,REF_BOMB,3,1,0,1000},
    {"arm",4,REF_ARM,3,1,0,0},
    {"detonate",4,REF_DETONATE,3,1,0,0},
    {"mine",4,REF_MINE,3,1,0,0},
    {"launch",4,REF_LAUNCH,3,1,0,1500},
    {"stuntubes",4,REF_STUN,2,5,0,1000},
    {"oilspit",4,REF_STUN,2,5,0,1000},
    {"cubeform",4,REF_CUBE,2,5,2000,2000},
    {"terminator",4,REF_CUBE,2,5,2000,2000},
    {"mechsight",4,REF_SIGHT,0,1,250,0},
    {"scry",4,REF_SCRY,0,3,500,0},
    {"read_aura",4,REF_AURA,0,4,100,0},
    {"lloth",4,REF_LLOTH,0,4,1500,0},
    {"darkness",4,REF_DARK,2,3,1000,0},
    {"web",4,REF_WEB,0,2,500,0},
    {"transmit",4,REF_TRANSMIT,0,5,1500,500},
    {"shadowwalk",4,REF_CLOAK,2,4,1000,0},
    {"upload",4,REF_CLOAK,2,4,1000,0},
    {"cyborgeq",4,REF_EQUIPMENT,2,1,0,0},
    {"armor_plating",4,REF_PASSIVE,2,1,0,0},
    {"limb_damage",4,REF_PASSIVE,1,1,0,0},
    {"limb_speed",4,REF_PASSIVE,1,1,0,0},
    {"limb_damage_cap",4,REF_PASSIVE,1,1,0,0},
    {"limb_defense",4,REF_PASSIVE,1,1,0,0},
    {"limb_accuracy",4,REF_PASSIVE,1,1,0,0},
    {"weapon_proficiency",4,REF_PASSIVE,1,1,0,0},
    {"regeneration",4,REF_PASSIVE,2,6,0,0},
    {"darkvision",4,REF_PASSIVE,0,3,0,0},
    {"scry",3,REF_SCRY,0,4,500,0},
    {"eyespy",3,REF_EYE,0,4,500,0},
    {"astral_projection",3,REF_ASTRAL,0,7,1500,0},
    {"cherub",1,REF_COMPANION,0,4,500,0},
    {"imp",2,REF_COMPANION,0,4,500,0},
    {"guardian",2,REF_GUARDIAN,0,5,1000,0},
    {"servant",2,REF_SERVANT,0,6,1500,0},
    {"unveil",3,REF_UNVEIL,0,5,750,0},
    {"weaponform",3,REF_WEAPONFORM,0,4,1000,0},
    {"transform",2,REF_TRANSFORM,0,3,1000,0},
    {"humanform",3,REF_HUMANFORM,0,0,0,0},
    {"object_move",3,REF_OBJECTMOVE,0,3,50,0},
    {"object_leap",3,REF_OBJECTLEAP,0,3,150,0},
    {"object_magic",3,REF_OBJECTMAGIC,0,5,500,0},
    {"object_lifespan",3,REF_LIFESPAN,0,3,0,0},
    {"steed",2,REF_STEED,0,4,1000,0},
    {NULL,0,0,0,0,0,0}
};

static int reference_class(CHAR_DATA *ch)
{
    return !parent_actor(ch) ? 0 : IS_ANGEL(ch) ? 1 : IS_DEMON(ch) ? 2 : 4;
}

/* Sharing a tested transient mechanism is not sharing class bits/mastery. */
static int reference_transient_actor(CHAR_DATA *ch)
{
    return (parent_actor(ch) && !IS_CYBORG(ch)) || lineage_valid(ch);
}

static int reference_sn(const char *name)
{
    return (int)skill_lookup(name);
}

static int reference_direction(const char *name)
{
    static const char *directions[] = {"north","east","south","west","up","down"};
    int i;
    for (i = 0; i < 6; ++i)
        if (name[0] && (!str_cmp(name, directions[i])
            || (name[1] == 0 && name[0] == directions[i][0]))) return i;
    return -1;
}

static void reference_dismiss(CHAR_DATA *ch)
{
    CHAR_DATA *pet;
    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL) return;
    pet = ch->pcdata->familiar;
    if (ch->pcdata->reference_familiar && pet != NULL && pet->wizard == ch) {
        ch->pcdata->reference_familiar = 0;
        ch->pcdata->familiar = NULL; pet->wizard = NULL;
        extract_char(pet, TRUE);
    } else ch->pcdata->reference_familiar = 0;
    affect_strip(ch, reference_sn("parent companion"));
}

void newclasses_object_release(OBJ_DATA *obj)
{
    CHAR_DATA *ch, *carrier;
    OBJ_DATA *outer;
    ROOM_INDEX_DATA *room;
    if (obj == NULL || (ch = obj->chobj) == NULL || IS_NPC(ch)
        || ch->pcdata == NULL || !ch->pcdata->reference_form
        || ch->pcdata->chobj != obj) return;
    room = obj->in_room;
    outer = obj;
    while (outer->in_obj != NULL) outer = outer->in_obj;
    carrier = outer->carried_by;
    if (outer->in_room != NULL) room = outer->in_room;
    if (carrier != NULL && carrier->in_room != NULL) room = carrier->in_room;
    if (room == NULL) room = ch->in_room;
    ch->pcdata->chobj = NULL; obj->chobj = NULL;
    ch->pcdata->reference_form = 0; ch->pcdata->obj_vnum = 0;
    REMOVE_BIT(ch->extra, EXTRA_OSWITCH);
    REMOVE_BIT(ch->affected_by, AFF_POLYMORPH);
    free_string(ch->morph); ch->morph = str_dup("");
    if (room != NULL && ch->in_room != room) {
        if (ch->in_room != NULL) char_from_room(ch);
        char_to_room(ch, room);
    }
    stc("You return safely to your living body.\n\r", ch);
}

void newclasses_release_transients(CHAR_DATA *ch)
{
    OBJ_DATA *obj;
    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL) return;
    obj = ch->pcdata->chobj;
    if (ch->pcdata->reference_form && obj != NULL) {
        newclasses_object_release(obj); extract_obj(obj);
    }
    ch->pcdata->reference_form = 0;
    reference_dismiss(ch);
}

static void reference_restore(CHAR_DATA *ch)
{
    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL) return;
    if (ch->pcdata->reference_familiar
        && (!reference_transient_actor(ch)
            || ch->pcdata->familiar == NULL
            || !is_affected(ch, reference_sn("parent companion"))))
        reference_dismiss(ch);
    /* A death purge also ends an object form before its object can outlive
     * the recovered player. Normal object timers call the same release hook.
     */
    if (ch->pcdata->reference_form && ch->pcdata->chobj != NULL
        && (!reference_transient_actor(ch) || ch->hit <= 0
            || !IS_EXTRA(ch, EXTRA_OSWITCH) || !IS_AFFECTED(ch, AFF_POLYMORPH))) {
        OBJ_DATA *obj = ch->pcdata->chobj;
        newclasses_object_release(obj);
        extract_obj(obj);
    }
}

static void reference_clear(CHAR_DATA *ch)
{
    const char *names[] = {"cyber sight", "cyber avatar", "cyber blessing",
        "cyber darkness", "cyber mine", "parent companion"};
    size_t i;
    newclasses_release_transients(ch);
    for (i = 0; i < sizeof(names)/sizeof(names[0]); ++i)
        affect_strip(ch, reference_sn(names[i]));
}

static void reference_loaded(CHAR_DATA *ch)
{
    newclasses_release_transients(ch);
    ch->pcdata->reference_familiar = 0;
    affect_strip(ch, reference_sn("parent companion"));
    if (reference_transient_actor(ch) && ch->pcdata->chobj == NULL
        && IS_EXTRA(ch, EXTRA_OSWITCH)) {
        REMOVE_BIT(ch->extra, EXTRA_OSWITCH);
        REMOVE_BIT(ch->affected_by, AFF_POLYMORPH);
        ch->pcdata->obj_vnum = 0; ch->pcdata->reference_form = 0;
        free_string(ch->morph); ch->morph = str_dup("");
    }
    if (!parent_actor(ch) || !IS_CYBORG(ch)) {
        reference_clear(ch); return;
    }
    if (!cyborg_systems_valid(&ch->pcdata->cybernetics))
        memset(&ch->pcdata->cybernetics, 0, sizeof(ch->pcdata->cybernetics));
    /* Ground deployments cannot survive logout or be relocated by a save. */
    affect_strip(ch, reference_sn("cyber mine"));
    if (cyborg_system_rank(ch, 0) < 1)
        affect_strip(ch, reference_sn("cyber sight"));
    if (cyborg_system_rank(ch, 0) < 4)
        affect_strip(ch, reference_sn("cyber blessing"));
    if (cyborg_system_rank(ch, 2) < 3)
        affect_strip(ch, reference_sn("cyber darkness"));
    if (cyborg_system_rank(ch, 2) < 5)
        affect_strip(ch, reference_sn("cyber avatar"));
}

static void reference_menu(CHAR_DATA *ch)
{
    const REFERENCE_POWER *p;
    char buf[256];
    const char *systems[] = {"Face", "Limbs", "Body", "Demolition"};
    int kind = reference_class(ch);
    if (kind == 4) {
        stc("\n\rCYBORG | System powers and passives\n\r"
            "Power                 System      Rank   Mana Nanites  Mastery\n\r"
            "--------------------  ----------  ----  ----- -------  -------\n\r", ch);
    } else
        stc("\n\rReference power        Rank     Mana  Mastery\n\r"
            "--------------------  ----  -------  -------\n\r", ch);
    for (p = reference_powers; p->key != NULL; ++p)
        if (p->classes & kind) {
            if (kind == 4) {
                if (p->kind <= REF_BIONICS) continue;
                snprintf(buf, sizeof(buf), "%-20s  %-10s  %4d  %5ld %7ld  %s\n\r",
                    p->key, systems[p->system], p->rank, p->mana, p->nano,
                    cyborg_system_rank(ch, p->system) < p->rank ? "Locked"
                        : p->kind == REF_PASSIVE ? "Passive" : "Known");
            } else
                snprintf(buf, sizeof(buf), "%-20s  %4d  %7ld  %s\n\r",
                    p->key, p->rank, p->mana,
                    parent_rank(ch) >= p->rank || reference_granted(ch, p->key) ? "Known" : "Locked");
            stc(buf, ch);
        }
    legacy_menu(ch);
}

int newclasses_darkvision(CHAR_DATA *ch)
{
    if (lineage_shadow_sight(ch)) return 1;
    return cyborg_system_rank(ch, 0) >= 3
        && is_affected(ch, reference_sn("cyber sight"));
}

int newclasses_room_dark(ROOM_INDEX_DATA *room)
{
    CHAR_DATA *ch;
    if (room == NULL) return 0;
    for (ch = room->people; ch != NULL; ch = ch->next_in_room)
        if (parent_actor(ch) && ch->hit > 0 && cyborg_system_rank(ch, 2) >= 3
            && is_affected(ch, reference_sn("cyber darkness"))) return 1;
    return 0;
}

/* Remote information never relocates the caster, grants HOLYLIGHT or lists
 * unrelated players. Visibility, plane, private rooms and wards remain gates.
 */
static CHAR_DATA *reference_remote(CHAR_DATA *ch, char *name)
{
    CHAR_DATA *v;
    if (!name[0] || ch->in_room == NULL) return NULL;
    v = get_char_world(ch, name);
    if (v != NULL && parent_actor(v) && !IS_CYBORG(v)
        && v->pcdata->reference_familiar == 3 && v->pcdata->familiar != NULL
        && is_affected(v, reference_sn("parent companion")))
        stc("Your companion warns you of a distant observer's attention.\n\r", v);
    if (v == NULL || v == ch || v->in_room == NULL || IS_BUI(v)
        || !can_see(ch, v) || IS_MORE(v, MORE_SHIELDED)
        || IS_SET(v->in_room->room_flags, ROOM_PRIVATE | ROOM_SOLITARY | ROOM_NO_TELEPORT)
        || (IS_AFFECTED(ch, AFF_SHADOWPLANE) != IS_AFFECTED(v, AFF_SHADOWPLANE)))
        return NULL;
    return v;
}

static void reference_scry(CHAR_DATA *ch, CHAR_DATA *v)
{
    char buf[512];
    snprintf(buf, sizeof(buf), "Your senses find %.80s at %.200s.\n\r",
        IS_NPC(v) ? v->short_descr : v->name, v->in_room->name);
    stc(buf, ch);
}

static int reference_transport(CHAR_DATA *ch, CHAR_DATA *v)
{
    ROOM_INDEX_DATA *from = ch->in_room, *to = v->in_room;
    return from != NULL && to != NULL && to != from && v->hit > 0
        && !IS_NPC(v) && is_same_group(ch, v) && ch->fighting == NULL
        && v->fighting == NULL && ch->fight_timer == 0 && v->fight_timer == 0
        && !IS_EXTRA(ch, EXTRA_OSWITCH) && !IS_EXTRA(v, EXTRA_OSWITCH)
        && !IS_MORE(ch, MORE_HEAD) && !IS_AFFECTED(ch, AFF_CURSE | AFF_WEBBED)
        && !is_affected(ch,gsn_lineage_root)
        && !IS_SET(from->room_flags, ROOM_NO_RECALL | ROOM_NO_TELEPORT)
        && !IS_SET(to->room_flags, ROOM_NO_TELEPORT | ROOM_PRIVATE | ROOM_SOLITARY);
}

static void reference_equipment(CHAR_DATA *ch, char *key)
{
    static const char *keys[] = {"whip", "ring", "collar", "armor", "helmet",
        "leggings", "boots", "gloves", "sleeves", "cloak", "belt", "bracer", "mask"};
    static const long wear[] = {ITEM_WEAR_HOLD, ITEM_WEAR_FINGER, ITEM_WEAR_NECK,
        ITEM_WEAR_BODY, ITEM_WEAR_HEAD, ITEM_WEAR_LEGS, ITEM_WEAR_FEET,
        ITEM_WEAR_HANDS, ITEM_WEAR_ARMS, ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST,
        ITEM_WEAR_WRIST, ITEM_WEAR_FACE};
    OBJ_INDEX_DATA *index;
    OBJ_DATA *obj;
    char name[100], desc[120];
    size_t i;
    for (i = 0; i < sizeof(keys)/sizeof(keys[0]) && str_cmp(keys[i], key); ++i) {}
    if (i == sizeof(keys)/sizeof(keys[0])) {
        stc("cyborgeq whip/ring/collar/armor/helmet/leggings/boots/gloves/sleeves/cloak/belt/bracer/mask.\n\r", ch);
        return;
    }
    snprintf(name, sizeof(name), "cybergeq cybernetic %.20s", keys[i]);
    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
        if (!str_cmp(obj->name, name)) {
            stc("You already carry that cybernetic item.\n\r", ch); return;
        }
    index = get_obj_index(OBJ_VNUM_PROTOPLASM);
    if (!management_ready(ch) || ch->primal < 100 || ch->carry_number >= 100 || index == NULL) {
        stc("Equipment fabrication requires 100 primal, inventory space and the Genesis protoplasm template.\n\r", ch);
        return;
    }
    obj = create_object(index, 50);
    if (obj == NULL) return;
    free_string(obj->name); obj->name = str_dup(name);
    snprintf(desc, sizeof(desc), "a cybernetic %.20s", keys[i]);
    free_string(obj->short_descr); obj->short_descr = str_dup(desc);
    snprintf(desc, sizeof(desc), "A cybernetic %.20s lies here.", keys[i]);
    free_string(obj->description); obj->description = str_dup(desc);
    free_string(obj->questowner); obj->questowner = str_dup(ch->name);
    obj->item_type = i == 0 ? ITEM_WEAPON : ITEM_ARMOR;
    obj->wear_flags = ITEM_TAKE | wear[i];
    obj->weight = 5; obj->cost = 0; obj->timer = 0;
    memset(obj->value, 0, sizeof(obj->value));
    if (i == 0) { obj->value[1] = 10; obj->value[2] = 20; obj->value[3] = TYPE_SLASH; }
    else obj->value[0] = 20;
    ch->primal -= 100; obj_to_char(obj, ch); WAIT_STATE(ch, 12);
    act("You fabricate $p.", ch, obj, NULL, TO_CHAR);
}

static void reference_familiar_power(CHAR_DATA *ch, const REFERENCE_POWER *p, char *arg)
{
    CHAR_DATA *pet = ch->pcdata->familiar, *v;
    MOB_INDEX_DATA *index;
    EXIT_DATA *exit;
    ROOM_INDEX_DATA *from;
    long mana = p->mana;
    int kind = p->kind == REF_EYE ? 1 : p->kind == REF_ASTRAL ? 2
        : p->kind == REF_GUARDIAN ? 4 : p->kind == REF_SERVANT ? 5
        : p->kind == REF_STEED ? 6 : 3;
    int dir, stat;
    char buf[160];
    if (!str_cmp(arg, "off") || !str_cmp(arg, "dismiss")) {
        if (ch->pcdata->reference_familiar != kind) {
            stc("That kind of companion is not yours to dismiss.\n\r", ch); return;
        }
        reference_dismiss(ch); stc("Your companion departs.\n\r", ch); return;
    }
    if (pet != NULL) {
        if (pet->wizard != ch || ch->pcdata->reference_familiar != kind) {
            stc("You already have another familiar or companion.\n\r", ch); return;
        }
        if (pet->in_room == NULL || IS_SET(pet->in_room->room_flags,
                ROOM_PRIVATE | ROOM_SOLITARY | ROOM_NO_TELEPORT)) {
            reference_dismiss(ch);
            stc("A protected boundary ends your familiar's link.\n\r", ch); return;
        }
        if (arg[0] == 0 || !str_cmp(arg, "look")) {
            if (pet->in_room == ch->in_room) do_look(ch, "auto");
            else do_look(pet, "auto");
            return;
        }
        if (kind == 3) {
            stc("Your companion warns you about active concealment and wards; use look or off.\n\r", ch); return;
        }
        dir = reference_direction(arg);
        if (dir >= 0) {
            exit = pet->in_room == NULL ? NULL : pet->in_room->exit[dir];
            if (ch->fighting != NULL || ch->fight_timer > 0 || pet->fighting != NULL
                || exit == NULL || exit->to_room == NULL
                || IS_SET(exit->exit_info, EX_CLOSED)
                || IS_SET(exit->to_room->room_flags, ROOM_PRIVATE | ROOM_SOLITARY | ROOM_NO_TELEPORT | ROOM_NO_MOB)
                || !power_ready(ch, 50, 0)) {
                stc("Your companion cannot scout that way.\n\r", ch); return;
            }
            from = pet->in_room;
            move_char(pet, dir);
            if (ch->pcdata->familiar != pet) return; /* extraction */
            if (pet->in_room != from) power_commit(ch, 50, 0, 1);
            return;
        }
        if (kind < 4 || pet->in_room != ch->in_room || !power_ready(ch, 100, 0)) {
            stc("Use look, a direction, off, or a local foe for combat companions.\n\r", ch); return;
        }
        v = parent_enemy(ch, arg);
        if (v == NULL || !hurt_fighting(ch, v)) return;
        power_commit(ch, 100, 0, 1);
        one_hit(pet, v, TYPE_HIT, "summoned guardian");
        return;
    }
    if (arg[0] || !management_ready(ch) || !power_ready(ch, mana, 0)) {
        stc("Summon your companion without an argument while at peace.\n\r", ch); return;
    }
    index = get_mob_index(kind == 6 ? MOB_VNUM_MOUNT : kind == 1 ? MOB_VNUM_EYE : kind == 2 ? MOB_VNUM_ASTRAL
        : MOB_VNUM_GUARDIAN);
    if (index == NULL) { stc("The companion template is unavailable; no cost was charged.\n\r", ch); return; }
    pet = create_mobile(index);
    if (pet == NULL) return;
    pet->act = ACT_IS_NPC | ACT_SENTINEL | ACT_NOEXP | ACT_COMMANDED;
    pet->affected_by = 0; pet->level = kind >= 4 ? 20 : 1;
    pet->hit = pet->max_hit = kind >= 4 ? 3000 : 500;
    pet->gold = 0; pet->hitroll = pet->damroll = kind == 5 ? 60 : 30;
    pet->armor = kind >= 4 ? 30 : 0;
    for (stat = STAT_STR; stat <= STAT_WIS; ++stat)
        pet->stats[stat] = kind >= 4 ? 5 : 1;
    free_string(pet->name);
    pet->name = str_dup(kind == 1 ? "eyespy eyeball" : kind == 2 ? "astral projection"
        : kind == 4 ? "infernal guardian" : kind == 5 ? "demonic servant" : kind == 6 ? "infernal steed"
        : IS_ANGEL(ch) ? "cherub companion" : "imp companion");
    free_string(pet->short_descr); pet->short_descr = str_dup(pet->name);
    snprintf(buf, sizeof(buf), "A %.60s waits here.\n\r", pet->name);
    free_string(pet->long_descr); pet->long_descr = str_dup(buf);
    pet->wizard = ch; ch->pcdata->familiar = pet; ch->pcdata->reference_familiar = kind;
    char_to_room(pet, ch->in_room);
    if (kind >= 4) add_follower(pet, ch);
    affect_strip(ch, reference_sn("parent companion"));
    parent_affect(ch, reference_sn("parent companion"), APPLY_NONE, kind, 3, 0);
    power_commit(ch, mana, 0, 1);
    act("You call forth $N.", ch, NULL, pet, TO_CHAR);
}

static void reference_object_power(CHAR_DATA *ch, const REFERENCE_POWER *p, char *arg)
{
    static const char *forms[] = {"sword","helm","greaves","cloak","armor","ring","shield"};
    static const long wear[] = {ITEM_WEAR_HOLD,ITEM_WEAR_HEAD,ITEM_WEAR_LEGS,
        ITEM_WEAR_ABOUT,ITEM_WEAR_BODY,ITEM_WEAR_FINGER,ITEM_WEAR_HOLD};
    OBJ_DATA *obj = ch->pcdata->chobj;
    OBJ_INDEX_DATA *index;
    CHAR_DATA *v, *carrier;
    ROOM_INDEX_DATA *from;
    char buf[160];
    size_t form;
    long mana;
    int dir, saved;
    if (p->kind == REF_HUMANFORM || (p->kind == REF_TRANSFORM && !str_cmp(arg, "back"))) {
        if (!ch->pcdata->reference_form || obj == NULL) {
            stc("You are not in a reference object form.\n\r", ch); return;
        }
        newclasses_object_release(obj); extract_obj(obj); return;
    }
    if (p->kind == REF_WEAPONFORM || p->kind == REF_TRANSFORM) {
        if (obj != NULL || IS_AFFECTED(ch, AFF_POLYMORPH | AFF_WEBBED)
            || !management_ready(ch) || !power_ready(ch, p->mana, 0)) return;
        if (p->kind == REF_WEAPONFORM) {
            if (arg[0]) { stc("Use weaponform without a target.\n\r", ch); return; }
            form = 0;
        } else {
            for (form = 0; form < 7 && str_cmp(forms[form], arg); ++form) {}
            if (form == 7) {
                stc("transform sword/helm/greaves/cloak/armor/ring/shield, or back.\n\r", ch); return;
            }
        }
        index = get_obj_index(OBJ_VNUM_PROTOPLASM);
        if (index == NULL) { stc("The form template is missing; no mana was charged.\n\r", ch); return; }
        obj = create_object(index, 50); if (obj == NULL) return;
        snprintf(buf, sizeof(buf), "%s animated %s", IS_ANGEL(ch) ? "angelic" : "demonic", forms[form]);
        free_string(obj->name); obj->name = str_dup(buf);
        snprintf(buf, sizeof(buf), "an animated %s", forms[form]);
        free_string(obj->short_descr); obj->short_descr = str_dup(buf);
        snprintf(buf, sizeof(buf), "An animated %s rests here.", forms[form]);
        free_string(obj->description); obj->description = str_dup(buf);
        obj->item_type = form == 0 ? ITEM_WEAPON : ITEM_ARMOR;
        obj->wear_flags = ITEM_TAKE | wear[form]; obj->quest |= QUEST_VANISH;
        obj->weight = 5; obj->cost = 0; obj->timer = 0;
        memset(obj->value, 0, sizeof(obj->value));
        if (form == 0) { obj->value[1] = 10; obj->value[2] = 20; obj->value[3] = TYPE_SLASH; }
        else obj->value[0] = 20;
        obj_to_room(obj, ch->in_room);
        obj->chobj = ch; ch->pcdata->chobj = obj; ch->pcdata->obj_vnum = 0;
        ch->pcdata->reference_form = form + 1;
        SET_BIT(ch->extra, EXTRA_OSWITCH); SET_BIT(ch->affected_by, AFF_POLYMORPH);
        free_string(ch->morph); ch->morph = str_dup(obj->short_descr);
        power_commit(ch, p->mana, 0, 1);
        stc(lineage_valid(ch)
            ? "Your body becomes an animated object; use wraith materialize or gargoyle statue back to return.\n\r"
            : "Your body becomes an animated object; use humanform to return.\n\r",ch);
        return;
    }
    if (obj == NULL || !ch->pcdata->reference_form || obj->chobj != ch) {
        stc("That power requires your animated object form.\n\r", ch); return;
    }
    if (p->kind == REF_LIFESPAN) {
        if (str_cmp(arg, "long") && str_cmp(arg, "short")) {
            stc("object_lifespan long/short. Short safely reforms you after one tick.\n\r", ch); return;
        }
        obj->timer = !str_cmp(arg, "short") ? 1 : 0;
        stc("Your object's lifespan changes.\n\r", ch); return;
    }
    if (!power_ready(ch, p->mana, 0)) return;
    if (p->kind == REF_OBJECTMOVE) {
        dir = reference_direction(arg);
        if (obj->in_room != ch->in_room || dir < 0) {
            stc("Roll along the ground using object_move <direction>.\n\r", ch); return;
        }
        from = ch->in_room;
        move_char(ch, dir);
        if (ch->pcdata->chobj != obj) return;
        if (ch->in_room != from) {
            obj_from_room(obj); obj_to_room(obj, ch->in_room);
            power_commit(ch, p->mana, 0, 1);
        }
        return;
    }
    if (p->kind == REF_OBJECTLEAP) {
        v = parent_ally(ch, arg);
        if (v == NULL || v == ch || IS_NPC(v) || v->fighting != NULL
            || (obj->carried_by != NULL && !is_same_group(ch, obj->carried_by))) {
            stc("Leap only into a willing party member's inventory.\n\r", ch); return;
        }
        if (obj->in_room) obj_from_room(obj);
        else if (obj->carried_by) obj_from_char(obj);
        else { stc("You cannot leap from a container.\n\r", ch); return; }
        obj_to_char(obj, v); power_commit(ch, p->mana, 0, 1);
        stc("You leap into your ally's inventory.\n\r", ch); return;
    }
    carrier = obj->carried_by;
    if (p->kind != REF_OBJECTMAGIC || carrier == NULL || IS_NPC(carrier)
        || carrier->in_room != ch->in_room || obj->wear_loc == WEAR_NONE
        || !is_same_group(ch, carrier)) {
        stc("Object magic requires a party member to equip your living form.\n\r", ch); return;
    }
    v = parent_enemy(carrier, arg);
    if (v == NULL || !can_see(ch, v) || (!IS_NPC(v) && !CAN_PK(ch))
        || !hurt_fighting(carrier, v)) return;
    saved = saves_spell(75, v); mana = 450 + 40 * parent_rank(ch);
    power_commit(ch, p->mana, 0, 1);
    damage(carrier, v, saved ? mana / 2 : mana, TYPE_SPELL, "living weapon magic");
}

static void reference_tick(CHAR_DATA *ch)
{
    CHAR_DATA *v;
    AFFECT_DATA *af;
    long heal, demolition;
    reference_restore(ch);
    legacy_tick(ch);
    if (reference_transient_actor(ch)) {
        CHAR_DATA *pet = ch->pcdata->familiar;
        if (ch->pcdata->reference_familiar >= 4 && pet != NULL && pet->wizard == ch
            && pet->in_room == ch->in_room && pet->fighting == NULL
            && (v = ch->fighting) != NULL && !IS_BUI(v) && !is_same_group(ch, v)
            && (IS_NPC(v) || (CAN_PK(ch) && CAN_PK(v))) && !is_safe(ch, v))
            one_hit(pet, v, TYPE_HIT, "protective guardian");
        return;
    }
    if (!parent_actor(ch) || !IS_CYBORG(ch) || ch->in_room == NULL
        || ch->hit <= 0 || ch->position < POS_RESTING
        || !cyborg_runtime_valid(&ch->pcdata->cyber)
        || !cyborg_systems_valid(&ch->pcdata->cybernetics)
        || is_affected(ch, gsn_lycaon)) return;
    if (cyborg_system_rank(ch, 2) >= 6 && ch->max_hit > ch->hit) {
        heal = UMIN(300, UMIN(ch->max_hit - ch->hit, ch->pcdata->cyber.nanites));
        if (heal > 0) { ch->pcdata->cyber.nanites -= heal; ch->hit += heal; }
    }
    demolition = cyborg_system_rank(ch, 3);
    if (demolition < 1) return;
    for (af = ch->affected; af != NULL; af = af->next)
        if (af->type == reference_sn("cyber mine")) {
            if (af->modifier != ch->in_room->vnum) {
                affect_strip(ch, af->type); return;
            }
            if (IS_SET(ch->in_room->room_flags, ROOM_SAFE)) return;
            for (v = ch->in_room->people; v != NULL; v = v->next_in_room)
                if (v != ch && v->fighting == ch && !IS_BUI(v)
                    && !is_same_group(ch, v) && can_see(ch, v)
                    && (IS_NPC(v) || (CAN_PK(ch) && CAN_PK(v))) && !is_safe(ch, v)) {
                    affect_strip(ch, reference_sn("cyber mine"));
                    damage(ch, v, 360 * demolition, TYPE_SPELL, "defensive mine");
                    return; /* no combatant access after damage */
                }
            return;
        }
}

static int reference_command(CHAR_DATA *ch, char *argument)
{
    char cmd[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH], extra[MAX_INPUT_LENGTH], tail[MAX_INPUT_LENGTH];
    char buf[256];
    const REFERENCE_POWER *p;
    CYBORG_SYSTEMS *s;
    CYBORG_SYSTEMS next;
    CHAR_DATA *v;
    long *track, cost, value, amount, sn;
    int kind = reference_class(ch), cap, saved;
    if (!kind || argument == NULL || strlen(argument) >= MAX_INPUT_LENGTH) return 0;
    argument = one_argument(argument, cmd);
    argument = one_argument(argument, arg);
    argument = one_argument(argument, extra); one_argument(argument, tail);
    for (p = reference_powers; p->key != NULL; ++p)
        if (!str_cmp(p->key, cmd) && (p->classes & kind)) break;
    if (p->key == NULL) {
        /* Cyborg radio owns its complete bounded message; do not apply
         * reference-command token limits to unrelated dispatchers. */
        if (kind==4) return 0;
        if (tail[0]) { stc("Too many ability arguments.\n\r",ch); return 1; }
        return legacy_command(ch, cmd, arg, extra);
    }
    if (tail[0]) { stc("Too many ability arguments.\n\r", ch); return 1; }
    if (extra[0]) { stc("Too many reference-power arguments.\n\r", ch); return 1; }
    if (IS_EXTRA(ch, EXTRA_OSWITCH) && p->kind != REF_HUMANFORM
        && p->kind != REF_OBJECTMOVE && p->kind != REF_OBJECTLEAP
        && p->kind != REF_OBJECTMAGIC && p->kind != REF_LIFESPAN
        && !(p->kind == REF_TRANSFORM && !str_cmp(arg, "back"))) {
        stc("Only object-form controls are available while transformed.\n\r", ch); return 1;
    }
    s = &ch->pcdata->cybernetics;
    if (kind == 4 && (!cyborg_systems_valid(s) || !cyborg_runtime_valid(&ch->pcdata->cyber))) {
        stc("Invalid cybernetic systems; contact an administrator.\n\r", ch); return 1;
    }
    if (p->kind == REF_BIONICS) {
        value = positive_number(arg);
        if (!management_ready(ch) || !value || value > 10000000 - s->bionic || ch->exp < value) {
            stc("bionics <positive XP amount>; requires experience and bionic capacity.\n\r", ch);
            return 1;
        }
        ch->exp -= value; s->bionic += value; WAIT_STATE(ch, 12);
        stc("Experience converted into bionic training power.\n\r", ch); return 1;
    }
    if (p->kind <= REF_DEMOLITION) {
        track = p->kind == REF_FACE ? &s->face : p->kind == REF_LIMBS ? &s->limbs
            : p->kind == REF_BODY ? &s->body : &s->demolition;
        cap = p->kind == REF_BODY ? 6 : p->kind == REF_DEMOLITION ? 12 : 5;
        cost = p->kind == REF_DEMOLITION ? 15 * (*track + 1) : 12500L << *track;
        if (!arg[0] || !str_cmp(arg, "show")) {
            snprintf(buf, sizeof(buf), "%.30s: %ld/%d; next cost %ld %s. Use improve.\n\r",
                p->key, *track, cap, cost, p->kind == REF_DEMOLITION ? "primal" : "bionics");
            stc(buf, ch); return 1;
        }
        if (str_cmp(arg, "improve") || !management_ready(ch) || *track >= cap) {
            stc("Use improve while at peace, below the system's mastery cap.\n\r", ch); return 1;
        }
        if ((p->kind == REF_BODY && s->limbs < 1)
            || (p->kind == REF_FACE && s->face == 4 && s->body < 4)
            || (p->kind == REF_DEMOLITION ? ch->primal < cost : s->bionic < cost)) {
            stc("Missing implant prerequisites or training currency.\n\r", ch); return 1;
        }
        next = *s;
        if (p->kind == REF_FACE) ++next.face;
        else if (p->kind == REF_LIMBS) ++next.limbs;
        else if (p->kind == REF_BODY) ++next.body;
        else ++next.demolition;
        if (!cyborg_systems_valid(&next)) return 1;
        if (p->kind == REF_DEMOLITION) ch->primal -= cost;
        else next.bionic -= cost;
        *s = next; cyborg_sync(ch); WAIT_STATE(ch, 12);
        stc("Your reference system training advances.\n\r", ch); return 1;
    }
    if ((kind == 4 ? cyborg_system_rank(ch, p->system) : parent_rank(ch)) < p->rank
        && !reference_granted(ch, p->key)) {
        stc("You have not mastered the required reference system.\n\r", ch); return 1;
    }
    if ((p->kind >= REF_EYE && p->kind <= REF_SERVANT) || p->kind == REF_STEED) {
        reference_familiar_power(ch, p, arg); return 1;
    }
    if (p->kind >= REF_WEAPONFORM && p->kind <= REF_LIFESPAN) {
        reference_object_power(ch, p, arg); return 1;
    }
    if (p->kind == REF_UNVEIL) {
        v = parent_ally(ch, arg);
        if (v == NULL || IS_NPC(v) || v == ch || !power_ready(ch, p->mana, 0)) {
            stc("Unveil requires a willing player in your party.\n\r", ch); return 1;
        }
        snprintf(buf, sizeof(buf), "Your ally's thoughts reveal health %ld/%ld, mana %ld/%ld, %s and %s.\n\r",
            v->hit, v->max_hit, v->mana, v->max_mana,
            IS_AFFECTED(v, AFF_CURSE) ? "a curse" : "no curse",
            IS_AFFECTED(v, AFF_POISON) ? "poison" : "no poison");
        power_commit(ch, p->mana, 0, 1); stc(buf, ch);
        stc("You consent to a brief thought link with your party's seer.\n\r", v);
        return 1;
    }
    if (p->kind == REF_PASSIVE) {
        if (arg[0]) { stc("Passive systems take no target.\n\r", ch); return 1; }
        cyborg_sync(ch); stc("That trained passive system is active automatically.\n\r", ch); return 1;
    }
    if (p->kind == REF_EQUIPMENT) { reference_equipment(ch, arg); return 1; }
    if (p->kind == REF_ARM) {
        if (!management_ready(ch) || s->bombs == 0 || (arg[0] && str_cmp(arg, "off"))) {
            stc("Arm requires a fabricated bomb; use arm or arm off.\n\r", ch); return 1;
        }
        s->armed = arg[0] ? 0 : 1;
        stc(s->armed ? "One bomb is armed; use detonate <target> or mine.\n\r"
                    : "Your bombs are safe.\n\r", ch); return 1;
    }
    if (!power_ready(ch, p->mana, p->nano)) return 1;
    if (p->kind == REF_BOMB) {
        if (arg[0] || s->bombs >= 20 || !cyborg_has_component(ch, CY_NANITE_FACTORY)) {
            stc("Bomb fabrication requires a factory and ammunition capacity.\n\r", ch); return 1;
        }
        power_commit(ch, p->mana, p->nano, 1); ++s->bombs;
        stc("You fabricate a safe explosive charge.\n\r", ch); return 1;
    }
    if (p->kind == REF_SCRY || p->kind == REF_TRANSMIT) {
        v = reference_remote(ch, arg);
        if (v == NULL || (p->kind == REF_TRANSMIT && !reference_transport(ch, v))) {
            stc("Your sensors cannot reach that target through its protection.\n\r", ch); return 1;
        }
        power_commit(ch, p->mana, p->nano, 2);
        if (p->kind == REF_TRANSMIT) {
            char_from_room(ch); char_to_room(ch, v->in_room); do_look(ch, "auto");
        } else reference_scry(ch, v);
        return 1;
    }
    if (p->kind == REF_AURA) {
        v = get_char_room(ch, arg);
        if (v == NULL || !can_see(ch, v) || !IS_NPC(v) || v->pIndexData == NULL) {
            stc("Read Aura requires a visible NPC here.\n\r", ch); return 1;
        }
        snprintf(buf, sizeof(buf), "Their alignment is %ld; health %ld/%ld.\n\r",
            v->pIndexData->alignment, v->hit, v->max_hit);
        power_commit(ch, p->mana, p->nano, 1); stc(buf, ch); return 1;
    }
    if (p->kind == REF_SIGHT || p->kind == REF_CUBE || p->kind == REF_LLOTH
        || p->kind == REF_DARK || p->kind == REF_CLOAK || p->kind == REF_MINE) {
        sn = p->kind == REF_SIGHT ? reference_sn("cyber sight")
            : p->kind == REF_CUBE ? reference_sn("cyber avatar")
            : p->kind == REF_LLOTH ? reference_sn("cyber blessing")
            : p->kind == REF_DARK ? reference_sn("cyber darkness")
            : p->kind == REF_CLOAK ? gsn_parent_cloak : reference_sn("cyber mine");
        if (arg[0] || is_affected(ch, sn)) {
            stc("That reference effect is already active or takes no argument.\n\r", ch); return 1;
        }
        if ((p->kind == REF_CLOAK && (ch->fighting || ch->fight_timer > 0))
            || (p->kind == REF_MINE && (!s->armed || s->bombs == 0
                || IS_SET(ch->in_room->room_flags, ROOM_SAFE)))) {
            stc("You cannot deploy that system here or now.\n\r", ch); return 1;
        }
        power_commit(ch, p->mana, p->nano, 1);
        if (p->kind == REF_CUBE || p->kind == REF_LLOTH) {
            amount = p->kind == REF_CUBE ? 35 : 15;
            parent_affect(ch, sn, APPLY_HITROLL, amount, 3, 0);
            parent_affect(ch, sn, APPLY_DAMROLL, amount, 3, 0);
        } else if (p->kind == REF_MINE) {
            --s->bombs; s->armed = 0;
            parent_affect(ch, sn, APPLY_NONE, ch->in_room->vnum, 3, 0);
        } else parent_affect(ch, sn, APPLY_NONE, 0, 3, 0);
        stc("Your reference system takes effect.\n\r", ch); return 1;
    }
    if ((p->kind == REF_STUN && s->limbs < 5)
        || (p->kind == REF_LAUNCH && (!cyborg_has_component(ch, CY_TARGETCOMP)
            || !cyborg_has_component(ch, CY_LEFT_LEG) || !cyborg_has_component(ch, CY_RIGHT_LEG)))
        || (p->kind == REF_DETONATE && (!s->armed || s->bombs == 0))) {
        stc("Missing weapon, ammunition or targeting prerequisites.\n\r", ch); return 1;
    }
    v = parent_enemy(ch, arg);
    if (v == NULL) return 1;
    sn = skill_lookup("web");
    if (p->kind == REF_WEB && IS_AFFECTED(v, AFF_WEBBED)) {
        stc("They are already webbed.\n\r", ch); return 1;
    }
    if (!hurt_fighting(ch, v)) return 1;
    saved = saves_spell(75, v);
    power_commit(ch, p->mana, p->nano, 1);
    if (p->kind == REF_WEB) {
        if (!saved) parent_affect(v, sn, APPLY_NONE, 0, 2, AFF_WEBBED);
        stc(saved ? "They resist the cybernetic web.\n\r" : "Your web immobilizes them.\n\r", ch);
        return 1;
    }
    if (p->kind == REF_STUN && !saved && !IS_AFFECTED(v, AFF_POISON))
        parent_affect(v, gsn_poison, APPLY_HITROLL, -4, 2, AFF_POISON);
    amount = p->kind == REF_STUN ? 1200 : p->kind == REF_LAUNCH
        ? 420 * s->demolition : 350 * s->demolition;
    if (saved) amount /= 2;
    if (p->kind == REF_DETONATE) { --s->bombs; s->armed = 0; }
    damage(ch, v, amount, TYPE_SPELL, p->kind == REF_STUN ? "stun tubes"
        : p->kind == REF_LAUNCH ? "targeted missile" : "explosive charge");
    return 1;
}
