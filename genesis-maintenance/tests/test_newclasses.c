#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"

extern void init_mm(void);
extern void fread_char(CHAR_DATA *ch, FILE *fp);

static void recover(CHAR_DATA *ch)
{
    affect_strip(ch, gsn_parent_recovery);
    ch->wait = 0;
}

static void expanded_kit(CHAR_DATA *ch, CHAR_DATA *v, MOB_INDEX_DATA *mob, int demon)
{
    PC_DATA ally_pc;
    AFFECT_DATA *af;
    long mana, hp, armor, hitroll, damroll;
    int n;
    const char *angel_new[] = {"feet", "might", "toughness", "speed",
        "shadowsight", "angelarmour", "spiritshare Target", "strengthen",
        "guardianangel", "purifyspirit Target", "empower"};
    const char *demon_new[] = {"hooves", "horns", "tail", "might", "toughness",
        "speed", "shadowsight", "truesight", "soultwist Target",
        "frostbreath Target", "netherflames Target"};
    const char **commands = demon ? demon_new : angel_new;
    memset(&ally_pc, 0, sizeof(ally_pc));
    ch->mana = ch->max_mana = 1000000;
    ch->pcdata->powers[0] = 0; newclasses_loaded(ch); recover(ch);
    mana = ch->mana;
    for (n = 0; n < 11; ++n) {
        if (demon) do_demon(ch, (char *)commands[n]);
        else do_angel(ch, (char *)commands[n]);
        assert(ch->mana == mana && !is_affected(ch, gsn_parent_recovery));
    }
    ch->pcdata->powers[0] = 3; newclasses_loaded(ch);
    assert(char_stat(ch, STAT_STR) == 5 && char_stat(ch, STAT_CON) == 5);
    ch->exp = 799999;
    if (demon) do_demon(ch, "train"); else do_angel(ch, "train");
    assert(ch->pcdata->powers[0] == 3 && ch->exp == 799999);
    ch->exp = 800000;
    if (demon) do_demon(ch, "train"); else do_angel(ch, "train");
    assert(ch->pcdata->powers[0] == 4 && ch->exp == 0);
    assert(char_stat(ch, STAT_STR) == 6 && ch->stats[STAT_STR] == 5);
    ch->pcdata->powers[0] = 8; newclasses_loaded(ch); newclasses_loaded(ch);
    assert(char_stat(ch, STAT_STR) == 6 && char_stat(ch, STAT_DEX) == 6);
    assert(char_stat(ch, STAT_CON) == 6 && ch->stats[STAT_CON] == 5);
    mana = ch->mana;
    if (demon) do_demon(ch, "might"); else do_angel(ch, "speed");
    assert(ch->mana == mana); /* Passives are idempotent and cost-free. */
    recover(ch); damroll = ch->damroll;
    if (demon) do_demon(ch, "hooves"); else do_angel(ch, "feet");
    assert(ch->damroll == damroll + 8 && ch->mana == mana - 750);
    recover(ch); mana = ch->mana;
    if (demon) do_demon(ch, "hooves"); else do_angel(ch, "feet");
    assert(ch->damroll == damroll + 8 && ch->mana == mana);
    skill_table[skill_lookup("dispel magic")].spell_fun(
        skill_lookup("dispel magic"), 1000, ch, ch);
    assert(!is_affected(ch, gsn_parent_feet) && ch->damroll == damroll);
    assert(char_stat(ch, STAT_STR) == 6 && char_stat(ch, STAT_CON) == 6);
    assert(char_stat(ch, STAT_DEX) == 6); /* real dispel preserves passives */
    affect_strip(ch, gsn_parent_senses); recover(ch);
    if (demon) do_demon(ch, "shadowsight"); else do_angel(ch, "shadowsight");
    assert(newclasses_nightvision(ch) && newclasses_perception(ch));
    if (demon) {
        recover(ch); hitroll = ch->hitroll; mana = ch->mana;
        do_demon(ch, "horns");
        assert(ch->hitroll == hitroll + 10 && ch->mana == mana - 750);
        recover(ch); mana = ch->mana; do_demon(ch, "horns");
        assert(ch->hitroll == hitroll + 10 && ch->mana == mana);
        recover(ch); armor = ch->armor; do_demon(ch, "tail");
        assert(ch->armor == armor + 10);
        recover(ch); mana = ch->mana; do_demon(ch, "tail");
        assert(ch->armor == armor + 10 && ch->mana == mana);
        affect_strip(ch, gsn_parent_senses); recover(ch);
        do_demon(ch, "truesight"); assert(newclasses_perception(ch));
        recover(ch); mana = ch->mana; hp = v->hit;
        do_demon(ch, "soultwist Target"); /* evil NPC cannot be twisted */
        assert(ch->mana == mana && v->hit == hp);
        mob->alignment = 500;
        SET_BIT(ch->in_room->room_flags, ROOM_SAFE);
        do_demon(ch, "soultwist Target");
        assert(ch->mana == mana && v->hit == hp);
        REMOVE_BIT(ch->in_room->room_flags, ROOM_SAFE);
        do_demon(ch, "soultwist Target");
        assert(ch->mana == mana - 1800 && v->hit < hp);
        stop_fighting(ch, TRUE); ch->fight_timer = 0; ch->position = POS_STANDING;
        mob->alignment = -500;
        for (n = 0; n < 64 && !is_affected(v, gsn_parent_chill); ++n) {
            recover(ch); mana = ch->mana; hp = v->hit;
            do_demon(ch, "frostbreath Target");
            assert(v->hit < hp && ch->mana == mana - 1200);
            stop_fighting(ch, TRUE); ch->fight_timer = 0; ch->position = POS_STANDING;
        }
        assert(is_affected(v, gsn_parent_chill));
        for (af = v->affected; af != NULL; af = af->next)
            if (af->type == gsn_parent_chill)
                assert(af->location == APPLY_STR && af->modifier == -1 && af->duration == 2);
        assert(v->stats[STAT_STR] == 5); /* no permanent victim-stat mutation */
        recover(ch); mana = ch->mana; hp = v->hit;
        do_demon(ch, "netherflames Target");
        assert(v->hit < hp && ch->mana == mana - 2500);
        stop_fighting(ch, TRUE); ch->fight_timer = 0; ch->position = POS_STANDING;
        /* Reload/class cleanup must not erase enemy-applied chill on targets. */
        v->act = 0; v->class = CLASS_NONE; v->pcdata = &ally_pc;
        newclasses_loaded(v); assert(is_affected(v, gsn_parent_chill));
        v->act = ACT_IS_NPC; v->pcdata = NULL;
        affect_strip(v, gsn_parent_chill);
    } else {
        affect_strip(ch, gsn_parent_ward); recover(ch); armor = ch->armor;
        do_angel(ch, "angelarmour"); assert(ch->armor == armor + 35);
        affect_strip(ch, gsn_parent_ward); recover(ch);
        do_angel(ch, "guardianangel"); assert(ch->armor == armor + 40);
        recover(ch); mana = ch->mana;
        do_angel(ch, "guardianangel");
        assert(ch->armor == armor + 40 && ch->mana == mana);
        recover(ch); ch->hit = ch->max_hit - 1; mana = ch->mana;
        do_angel(ch, "strengthen"); /* angel's own wings were removed above */
        assert(ch->hit == ch->max_hit - 1 && ch->mana == mana);
        do_angel(ch, "wings"); recover(ch);
        do_angel(ch, "strengthen");
        assert(ch->hit == ch->max_hit);
        recover(ch); mana = ch->mana;
        do_angel(ch, "strengthen");
        assert(ch->mana == mana); /* full health, no mana charged */
        v->leader = ch; v->hit = v->max_hit - 1;
        do_angel(ch, "spiritshare Target"); /* no aid to evil NPC */
        assert(ch->mana == mana && v->hit == v->max_hit - 1);
        v->act = 0; v->class = CLASS_VAMPIRE; v->pcdata = &ally_pc;
        v->level = LEV_AWA;
        hp = ch->hit;
        do_angel(ch, "spiritshare Target");
        assert(v->hit == v->max_hit && ch->hit == hp - 1 && ch->mana == mana - 500);
        mana = ch->mana; do_angel(ch, "spiritshare Target");
        assert(ch->mana == mana); /* recovery prevents repeated transfer */
        recover(ch); do_angel(ch, "spiritshare Target");
        do_angel(ch, "spiritshare self");
        assert(ch->mana == mana);
        v->hit -= 5000; hp = v->hit;
        v->leader = NULL; do_angel(ch, "empower Target");
        assert(ch->mana == mana && v->hit == hp);
        v->leader = ch; v->fighting = ch;
        do_angel(ch, "empower Target"); assert(ch->mana == mana && v->hit == hp);
        v->fighting = NULL; do_angel(ch, "empower Target");
        assert(v->hit == hp + 2000 && ch->mana == mana - 3000);
        recover(ch); v->hit = 0; mana = ch->mana;
        do_angel(ch, "strengthen Target"); assert(ch->mana == mana && v->hit == 0);
        v->hit = v->max_hit; v->beast = 7;
        do_angel(ch, "purifyspirit Target");
        assert(v->beast == 5 && ch->mana == mana - 1000);
        recover(ch); mana = ch->mana;
        do_angel(ch, "purifyspirit Target"); assert(ch->mana == mana && v->beast == 5);
        v->beast = 88; do_angel(ch, "purifyspirit Target"); assert(v->beast == 90);
        recover(ch); v->beast = 50; mana = ch->mana;
        do_angel(ch, "purifyspirit Target"); assert(v->beast == 50 && ch->mana == mana);
        v->class = CLASS_DEMON; v->beast = 30;
        do_angel(ch, "purifyspirit Target"); assert(ch->mana == mana && v->beast == 30);
        v->act = ACT_IS_NPC; v->class = CLASS_NONE; v->pcdata = NULL;
        v->leader = NULL; v->level = 20;
    }
    /* Low-rank reload discards illegal self-traits, but never trained stats. */
    ch->pcdata->powers[0] = 0; newclasses_loaded(ch);
    assert(!is_affected(ch, gsn_parent_feet) && !is_affected(ch, gsn_parent_horns));
    assert(!is_affected(ch, gsn_parent_tail) && !is_affected(ch, gsn_parent_traits));
    assert(char_stat(ch, STAT_STR) == 5 && ch->stats[STAT_STR] == 5);
    puts(demon ? "Expanded Demon powers: mastery gates, derived passives, body effects, sight, legal attacks and chill cleanup passed"
               : "Expanded Angel powers: mastery gates, derived passives, body effects, wards, ally-only healing, wing gates, Beast bounds and costs passed");
}

static void reference_cyborg_kit(CHAR_DATA *ch, CHAR_DATA *v)
{
    CYBORG_SYSTEMS *s = &ch->pcdata->cybernetics;
    CYBORG_RUNTIME *r = &ch->pcdata->cyber;
    FILE *fp;
    long cost, mana, nano, hp, armor, hitroll, speed, cap;
    int i;
    const char *tracks[] = {"limb_implants improve", "body_implants improve", "face_implants improve"};
    ch->exp = 10000000; ch->primal = 10000; ch->position = POS_STANDING;
    ch->fighting = NULL; ch->fight_timer = 0; r->factory = 0;
    ch->in_room->name = "reference test room";
    recover(ch); do_cyborg(ch, "bionics -1");
    assert(s->bionic == 0 && ch->exp == 10000000);
    do_cyborg(ch, "bionics 3000000");
    assert(s->bionic == 3000000 && ch->exp == 7000000);
    nano = s->bionic;
    do_cyborg(ch, "body_implants improve");
    assert(s->body == 0 && s->bionic == nano); /* requires one limb */
    for (i = 0; i < 3; ++i) {
        int n, count = i == 1 ? 6 : 5;
        for (n = 0; n < count; ++n) {
            cost = 12500L << n; nano = s->bionic;
            do_cyborg(ch, (char *)tracks[i]);
            assert(cyborg_system_rank(ch, i == 0 ? 1 : i == 1 ? 2 : 0) == n + 1);
            assert(s->bionic == nano - cost && ch->pcdata->powers[19] == 123);
        }
    }
    armor = ch->armor; hitroll = ch->hitroll; nano = s->bionic;
    do_cyborg(ch, "body_implants improve"); do_cyborg(ch, "face_implants improve");
    assert(ch->armor == armor && ch->hitroll == hitroll && s->bionic == nano);
    do_cyborg(ch, "limb_accuracy"); do_cyborg(ch, "armor_plating");
    assert(ch->armor == armor && ch->hitroll == hitroll); /* rebuild, never stack */
    assert(cyborg_melee_damage(ch, 1000) == 1950);
    assert(cyborg_absorb_damage(ch, 700) == 400);
    speed = get_speed(ch); cap = get_damcap(ch);
    s->limbs = 0; s->body = 0; s->face = 0;
    assert(cyborg_melee_damage(ch, 1000) == 1000);
    assert(cyborg_absorb_damage(ch, 700) == 700);
    assert(get_speed(ch) == speed + 5 && get_damcap(ch) == cap - 2300);
    s->limbs = 5; s->body = 6; s->face = 5;
    ch->mana = ch->max_mana = 100000; r->nanites = 10000;
    recover(ch); do_cyborg(ch, "mechsight");
    assert(newclasses_darkvision(ch) && !IS_SET(ch->act, PLR_HOLYLIGHT));
    SET_BIT(ch->in_room->added_flags, ROOM2_DARKNESS);
    assert(can_see(ch, v));
    REMOVE_BIT(ch->in_room->added_flags, ROOM2_DARKNESS);
    mana = ch->mana; recover(ch); do_cyborg(ch, "mechsight");
    assert(ch->mana == mana);
    recover(ch); do_cyborg(ch, "cubeform");
    assert(is_affected(ch, skill_lookup("cyber avatar")));
    assert(r->nanites == 8000);
    recover(ch); do_cyborg(ch, "lloth");
    assert(is_affected(ch, skill_lookup("cyber blessing")));
    recover(ch); do_cyborg(ch, "darkness");
    assert(newclasses_room_dark(ch->in_room) && room_is_dark(ch->in_room));
    affect_strip(ch, skill_lookup("cyber darkness"));
    recover(ch); do_cyborg(ch, "scry Target");
    recover(ch); mana = ch->mana;
    SET_BIT(ch->in_room->room_flags, ROOM_PRIVATE);
    do_cyborg(ch, "scry Target"); assert(ch->mana == mana);
    REMOVE_BIT(ch->in_room->room_flags, ROOM_PRIVATE);
    do_cyborg(ch, "transmit Target"); assert(ch->mana == mana); /* NPC is not a consenting ally */
    do_cyborg(ch, "cyborgeq armor"); assert(ch->primal == 10000); /* missing template */
    do_cyborg(ch, "demolitions improve"); assert(s->demolition == 1 && ch->primal == 9985);
    do_cyborg(ch, "demolitions show"); assert(s->demolition == 1 && ch->primal == 9985);
    recover(ch); do_cyborg(ch, "make_bomb");
    assert(s->bombs == 1 && !s->armed && r->nanites == 7000);
    recover(ch); nano = r->nanites; hp = v->hit;
    do_cyborg(ch, "detonate Target");
    assert(s->bombs == 1 && v->hit == hp && r->nanites == nano);
    do_cyborg(ch, "arm"); assert(s->armed);
    SET_BIT(ch->in_room->room_flags, ROOM_SAFE);
    do_cyborg(ch, "detonate Target"); assert(s->bombs == 1 && s->armed && v->hit == hp);
    REMOVE_BIT(ch->in_room->room_flags, ROOM_SAFE);
    do_cyborg(ch, "detonate Target"); assert(s->bombs == 0 && !s->armed && v->hit < hp);
    stop_fighting(ch, TRUE); ch->fight_timer = 0; ch->position = POS_STANDING;
    recover(ch); do_cyborg(ch, "make_bomb");
    recover(ch); do_cyborg(ch, "arm"); do_cyborg(ch, "mine");
    assert(s->bombs == 0 && !s->armed && is_affected(ch, skill_lookup("cyber mine")));
    v->fighting = ch; hp = v->hit; cyborg_update(ch);
    assert(v->hit < hp && !is_affected(ch, skill_lookup("cyber mine")));
    stop_fighting(ch, TRUE); ch->fight_timer = 0; ch->position = POS_STANDING;
    recover(ch); do_cyborg(ch, "web Target");
    assert(ch->fighting == v);
    stop_fighting(ch, TRUE); ch->fight_timer = 0; ch->position = POS_STANDING;
    recover(ch); nano = r->nanites; hp = v->hit;
    do_cyborg(ch, "stuntubes Target");
    assert(v->hit < hp && r->nanites == nano - 1000);
    stop_fighting(ch, TRUE); ch->fight_timer = 0; ch->position = POS_STANDING;
    recover(ch); hp = v->hit; nano = r->nanites;
    do_cyborg(ch, "launch Target");
    assert(v->hit < hp && r->nanites == nano - 1500);
    stop_fighting(ch, TRUE); ch->fight_timer = 0; ch->position = POS_STANDING;
    recover(ch); do_cyborg(ch, "shadowwalk"); assert(is_affected(ch, gsn_parent_cloak));
    nano = r->nanites; mana = ch->mana; ch->hit = ch->max_hit - 500;
    cyborg_update(ch);
    assert(ch->hit == ch->max_hit - 200 && r->nanites == nano - 300 && ch->mana == mana);
    ch->hit = ch->max_hit;
    fp = tmpfile(); assert(fp != NULL);
    fputs("CyborgSystems 1 5 5 6 12 20 1 123456\nExp 789\nEnd\n", fp); rewind(fp);
    fread_char(ch, fp); fclose(fp);
    assert(s->face == 5 && s->limbs == 5 && s->body == 6 && s->demolition == 12);
    assert(s->bombs == 20 && s->armed == 1 && s->bionic == 123456 && ch->exp == 789);
    assert(ch->armor == armor && ch->hitroll == hitroll + 50);
    puts("Reference Cyborg training, derived combat hooks, vision, effects, targets, explosive state, regeneration, save/load and no-stack behavior: passed");
}

static void run_parent(CHAR_DATA *ch, const char *command)
{
    if (IS_ANGEL(ch)) do_angel(ch, (char *)command);
    else do_demon(ch, (char *)command);
}

static void reference_shared_kit(CHAR_DATA *ch, CHAR_DATA *v)
{
    extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
    extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
    MOB_INDEX_DATA mobs[3];
    OBJ_INDEX_DATA item;
    ROOM_INDEX_DATA remote;
    EXIT_DATA north, south;
    OBJ_DATA *obj;
    CHAR_DATA *pet;
    ROOM_INDEX_DATA *room = ch->in_room;
    long mana, hp;
    size_t i;
    int ids[] = {MOB_VNUM_EYE, MOB_VNUM_ASTRAL, MOB_VNUM_GUARDIAN};
    const char *controls[] = {"eyespy", "astral_projection"};
    ch->pcdata->powers[0] = 8; newclasses_loaded(ch);
    recover(ch); ch->position = POS_STANDING; ch->fight_timer = 0;
    ch->mana = ch->max_mana = 100000;
    room->light = 1; room->description = ""; room->name = "shared ability test";
    mana = ch->mana;
    run_parent(ch, "eyespy"); run_parent(ch, "weaponform");
    assert(ch->mana == mana && ch->pcdata->familiar == NULL && ch->pcdata->chobj == NULL);
    memset(mobs, 0, sizeof(mobs)); memset(&item, 0, sizeof(item));
    memset(&remote, 0, sizeof(remote)); memset(&north, 0, sizeof(north));
    memset(&south, 0, sizeof(south));
    item.vnum = OBJ_VNUM_PROTOPLASM; item.name = item.short_descr = item.description = str_empty;
    item.item_type = ITEM_TRASH; item.wear_flags = ITEM_TAKE;
    assert(obj_index_hash[item.vnum % MAX_KEY_HASH] == NULL);
    obj_index_hash[item.vnum % MAX_KEY_HASH] = &item;
    for (i = 0; i < 3; ++i) {
        mobs[i].vnum = ids[i]; mobs[i].level = 1; mobs[i].act = ACT_IS_NPC;
        mobs[i].player_name = mobs[i].short_descr = mobs[i].long_descr = mobs[i].description = str_empty;
        assert(mob_index_hash[ids[i] % MAX_KEY_HASH] == NULL);
        mob_index_hash[ids[i] % MAX_KEY_HASH] = &mobs[i];
    }
    remote.area = room->area; remote.vnum = 99123; remote.name = "scouting destination";
    remote.description = ""; remote.light = 1;
    north.to_room = &remote; south.to_room = room;
    room->exit[DIR_NORTH] = &north; remote.exit[DIR_SOUTH] = &south;
    for (i = 0; i < 2; ++i) {
        recover(ch); run_parent(ch, controls[i]);
        pet = ch->pcdata->familiar;
        assert(pet != NULL && pet->wizard == ch && pet->in_room == room);
        assert(IS_SET(pet->act, ACT_NOEXP) && !IS_SET(pet->act, PLR_HOLYLIGHT));
        mana = ch->mana; run_parent(ch, controls[i]);
        assert(ch->pcdata->familiar == pet && ch->mana == mana);
        recover(ch);
        SET_BIT(remote.room_flags, ROOM_PRIVATE);
        run_parent(ch, i ? "astral_projection north" : "eyespy north");
        assert(pet->in_room == room && ch->mana == mana);
        REMOVE_BIT(remote.room_flags, ROOM_PRIVATE);
        run_parent(ch, i ? "astral_projection north" : "eyespy north");
        assert(pet->in_room == &remote && ch->in_room == room && ch->mana == mana - 50);
        run_parent(ch, i ? "astral_projection look" : "eyespy look");
        run_parent(ch, i ? "astral_projection off" : "eyespy off");
        assert(ch->pcdata->familiar == NULL && ch->pcdata->reference_familiar == 0);
    }
    recover(ch); run_parent(ch, IS_ANGEL(ch) ? "cherub" : "imp");
    assert(ch->pcdata->familiar && newclasses_perception(ch));
    affect_strip(ch, skill_lookup("parent companion")); cyborg_update(ch);
    assert(ch->pcdata->familiar == NULL && ch->pcdata->reference_familiar == 0);
    if (IS_DEMON(ch)) {
        recover(ch); run_parent(ch, "guardian");
        assert(ch->pcdata->familiar && ch->pcdata->familiar->master == ch);
        recover(ch); hp = v->hit; run_parent(ch, "guardian Target");
        assert(ch->fighting == v && v->hit < hp);
        run_parent(ch, "guardian off");
        stop_fighting(ch, TRUE); ch->fight_timer = 0; ch->position = POS_STANDING;
        recover(ch); run_parent(ch, "servant"); assert(ch->pcdata->familiar);
        newclasses_release_transients(ch); assert(ch->pcdata->familiar == NULL);
    }
    recover(ch); mana = ch->mana; run_parent(ch, "weaponform");
    obj = ch->pcdata->chobj;
    assert(obj && obj->chobj == ch && ch->pcdata->reference_form == 1);
    assert(ch->mana == mana - 1000 && IS_EXTRA(ch, EXTRA_OSWITCH));
    mana = ch->mana; run_parent(ch, IS_ANGEL(ch) ? "aura" : "shield");
    assert(ch->mana == mana); /* no unrelated abilities while an object */
    recover(ch); run_parent(ch, "object_move north");
    assert(ch->in_room == &remote && obj->in_room == &remote && ch->mana == mana - 50);
    recover(ch); run_parent(ch, "object_move south");
    assert(ch->in_room == room && obj->in_room == room);
    run_parent(ch, "object_lifespan short"); assert(obj->timer == 1);
    extract_obj(obj); /* same callback used by expiry or destruction */
    assert(ch->pcdata->chobj == NULL && ch->pcdata->reference_form == 0);
    assert(!IS_EXTRA(ch, EXTRA_OSWITCH) && !IS_AFFECTED(ch, AFF_POLYMORPH) && ch->in_room == room);
    recover(ch); run_parent(ch, "weaponform"); assert(ch->pcdata->chobj);
    run_parent(ch, "humanform"); assert(ch->pcdata->chobj == NULL);
    if (IS_DEMON(ch)) {
        recover(ch); run_parent(ch, "transform armor");
        obj = ch->pcdata->chobj; assert(obj && obj->item_type == ITEM_ARMOR);
        assert(CAN_WEAR(obj, ITEM_WEAR_BODY));
        run_parent(ch, "transform back"); assert(ch->pcdata->chobj == NULL);
    }
    room->exit[DIR_NORTH] = NULL;
    obj_index_hash[item.vnum % MAX_KEY_HASH] = NULL;
    for (i = 0; i < 3; ++i) {
        assert(mobs[i].count == 0);
        mob_index_hash[ids[i] % MAX_KEY_HASH] = NULL;
    }
    assert(char_list == ch && ch->next == v);
    newclasses_clear(ch);
    puts("Shared reference powers: real familiar/mobile ownership, privacy gates, remote scouting, guardian combat, template failures, object movement, safe expiry/reformation and cleanup passed");
}

int main(void)
{
    CHAR_DATA actor, victim;
    PC_DATA pc;
    ROOM_INDEX_DATA room;
    AREA_DATA area;
    MOB_INDEX_DATA mob;
    DESCRIPTOR_DATA desc;
    AFFECT_DATA af;
    CYBORG_RUNTIME runtime;
    FILE *fp;
    char output[32768], following[64], word[32];
    char oversized[MAX_INPUT_LENGTH + 20];
    char argument[80];
    long mana, nano, hp, armor, hitroll, primal;
    size_t i;
    memset(&actor, 0, sizeof(actor)); memset(&victim, 0, sizeof(victim));
    memset(&pc, 0, sizeof(pc)); memset(&room, 0, sizeof(room));
    memset(&area, 0, sizeof(area)); memset(&mob, 0, sizeof(mob));
    memset(&desc, 0, sizeof(desc)); memset(output, 0, sizeof(output));
    init_mm();
    for (i = 0; i < MAX_SKILL && skill_table[i].name != NULL; ++i)
        if (skill_table[i].pgsn != NULL) *skill_table[i].pgsn = i;
    actor.pcdata = &pc; actor.name = "Actor"; actor.short_descr = "the actor";
    victim.name = "Target"; victim.short_descr = "the target";
    actor.morph = victim.morph = str_empty;
    actor.long_descr = victim.long_descr = str_empty;
    actor.description = victim.description = str_empty;
    actor.in_room = victim.in_room = &room;
    actor.position = victim.position = POS_STANDING;
    actor.level = LEV_AWA;
    actor.hit = actor.max_hit = victim.hit = victim.max_hit = 1000000;
    actor.mana = actor.max_mana = 100000;
    actor.style[0] = victim.style[0] = STYLE_UNARMED;
    actor.primal = 10000;
    victim.act = ACT_IS_NPC; victim.level = 20; victim.pIndexData = &mob;
    mob.vnum = 1234; mob.alignment = -500;
    room.area = &area; area.nplayer = 1; room.people = &actor;
    actor.next_in_room = &victim; char_list = &actor; actor.next = &victim;
    desc.outbuf = output; desc.outsize = sizeof(output); actor.desc = &desc;
    for (i = 0; i < 5; ++i) actor.stats[i] = victim.stats[i] = 5;

    do_autoclass(&actor, "cyborg");
    assert(actor.class == CLASS_CYBORG);
    assert(pc.cyborg.permitted == CY_COMPONENT_MASK && pc.cyborg.humanity == 100);
    assert(stat_max(&actor, STAT_STR) == 10 && stat_max(&actor, STAT_DEX) == 10);
    assert(stat_max(&actor, STAT_CON) == 11 && stat_max(&actor, STAT_INT) == 10);
    assert(stat_max(&actor, STAT_WIS) == 9);
    pc.powers[19] = 123;
    primal = actor.primal;
    do_cyborg(&actor, "install taser"); /* missing prerequisites, atomic */
    assert(pc.cyborg.installed == 0 && actor.primal == primal);
    do_cyborg(&actor, "install heart");
    assert(cyborg_has_component(&actor, CY_HEART) && actor.primal == primal - 30);
    primal = actor.primal;
    do_cyborg(&actor, "install heart");
    assert(actor.primal == primal);
    assert(pc.powers[19] == 123);
    do_cyborg(&actor, "uninstall heart");
    assert(pc.cyborg.installed == 0 && pc.cyborg.humanity == 100);
    assert(actor.primal == primal);
    actor.exp = 9999; do_cyborg(&actor, "train");
    assert(pc.cyber.rifle == 0 && actor.exp == 9999);
    actor.exp = 10000; do_cyborg(&actor, "train");
    assert(pc.cyber.rifle == 1 && actor.exp == 0);
    /* A larger explicit test-only budget exercises every real adapter. */
    assert(cyborg_core_init(&pc.cyborg, CY_COMPONENT_MASK, 1000) == CY_OK);
    for (i = 0; i < CY_COMPONENT_COUNT; ++i) {
        snprintf(argument, sizeof(argument), "install %s", cyborg_component_at(i)->key);
        do_cyborg(&actor, argument);
        assert(cyborg_has_component(&actor, i));
    }
    assert(cyborg_core_valid(&pc.cyborg));
    armor = actor.armor; hitroll = actor.hitroll;
    newclasses_loaded(&actor);
    assert(actor.armor == armor && actor.hitroll == hitroll); /* no trait stacking */
    skill_table[skill_lookup("dispel magic")].spell_fun(
        skill_lookup("dispel magic"), 1000, &actor, &actor);
    assert(actor.armor == armor && actor.hitroll == hitroll);
    assert(cyborg_has_component(&actor, CY_HEART)); /* hardware is not magic */
    pc.cyber.nanites = 0;
    do_cyborg(&actor, "convert 1000");
    assert(pc.cyber.nanites == 1000);
    do_cyborg(&actor, "factory on");
    mana = actor.mana;
    cyborg_update(&actor);
    assert(pc.cyber.nanites == 1100 && actor.mana == mana - 100);
    pc.cyber.nanites = 9999; cyborg_update(&actor);
    assert(pc.cyber.nanites == 10000);
    memset(&af, 0, sizeof(af)); af.type = gsn_poison; af.duration = 3;
    af.bitvector = AFF_POISON; affect_to_char(&actor, &af);
    cyborg_update(&actor);
    assert(!is_affected(&actor, gsn_poison) && pc.cyber.nanites == 9975);
    do_cyborg(&actor, "factory off");
    actor.hit = actor.max_hit - 1; nano = pc.cyber.nanites;
    recover(&actor); do_cyborg(&actor, "repair");
    assert(actor.hit == actor.max_hit && pc.cyber.nanites == nano - 400);
    recover(&actor); nano = pc.cyber.nanites;
    do_cyborg(&actor, "repair");
    assert(pc.cyber.nanites == nano); /* full health is free */
    SET_BIT(room.room_flags, ROOM_SAFE);
    do_cyborg(&actor, "taser Target");
    assert(victim.hit == victim.max_hit && pc.cyber.nanites == nano);
    REMOVE_BIT(room.room_flags, ROOM_SAFE);
    do_cyborg(&actor, "taser Target");
    assert(victim.hit < victim.max_hit && pc.cyber.nanites == nano - 200);
    stop_fighting(&actor, TRUE); actor.fight_timer = 0; actor.position = POS_STANDING;
    recover(&actor); nano = pc.cyber.nanites; hp = victim.hit;
    do_cyborg(&actor, "snipe Target");
    assert(victim.hit < hp && pc.cyber.nanites == nano - 300);
    stop_fighting(&actor, TRUE); actor.fight_timer = 0; actor.position = POS_STANDING;
    recover(&actor); do_cyborg(&actor, "cloak");
    assert(is_affected(&actor, gsn_parent_cloak));
    assert(!can_see(&victim, &actor));
    do_cyborg(&actor, "cloak");
    assert(!is_affected(&actor, gsn_parent_cloak));
    SET_BIT(room.room_flags, ROOM_DARK);
    assert(newclasses_nightvision(&actor) && can_see(&actor, &victim));
    REMOVE_BIT(room.room_flags, ROOM_DARK);
    memset(oversized, 'x', sizeof(oversized) - 1); oversized[sizeof(oversized) - 1] = 0;
    nano = pc.cyber.nanites; primal = actor.primal;
    do_cyborg(&actor, oversized); do_cyborg(&actor, "convert 999999999999999999999999");
    do_cyborg(&actor, "install heart extra");
    assert(pc.cyber.nanites == nano && actor.primal == primal);
    do_cyborg(&actor, "radio probe");
    assert(strstr(output, "[Cybernet] Actor: probe") != NULL);
    puts("Cyborg enrollment, all 24 component adapters, costs, training, capped factory/repair, combat legality, visibility and input bounds: passed");

    fp = tmpfile(); assert(fp != NULL);
    assert(cyborg_runtime_write_player(&pc.cyber, fp) == CY_OK);
    fputs("Exp 789\n", fp); rewind(fp);
    assert(fscanf(fp, "%31s", word) == 1 && strcmp(word, "CyborgRuntime") == 0);
    assert(cyborg_runtime_read_player(&runtime, fp) == CY_OK);
    assert(memcmp(&runtime, &pc.cyber, sizeof(runtime)) == 0);
    assert(fgets(following, sizeof(following), fp) != NULL && strcmp(following, "Exp 789\n") == 0);
    fclose(fp);
    fp = tmpfile(); assert(fp != NULL);
    fputs("CyborgRuntime 1 10001 12 1\nExp 456\nEnd\n", fp); rewind(fp);
    fread_char(&actor, fp);
    assert(pc.cyber.nanites == 0 && pc.cyber.rifle == 0 && actor.exp == 456);
    assert(pc.powers[19] == 123 && cyborg_has_component(&actor, CY_HEART));
    fclose(fp);
    reference_cyborg_kit(&actor, &victim);
    newclasses_clear(&actor); actor.class = CLASS_NONE;
    assert(actor.armor == 0 && actor.hitroll == 0);
    assert(pc.cyborg.installed == 0 && pc.cyber.nanites == 0);
    assert(pc.cybernetics.face == 0 && pc.cybernetics.body == 0 && pc.cybernetics.bombs == 0);

    for (i = 0; i < 2; ++i) {
        actor.class = CLASS_NONE; pc.powers[0] = 0;
        actor.position = POS_STANDING; actor.fight_timer = 0;
        do_autoclass(&actor, i ? "demon" : "angel");
        assert(actor.class == (i ? CLASS_DEMON : CLASS_ANGEL));
        actor.exp = 49999;
        if (i) do_demon(&actor, "train"); else do_angel(&actor, "train");
        assert(pc.powers[0] == 0 && actor.exp == 49999);
        actor.exp = 50000;
        if (i) do_demon(&actor, "train"); else do_angel(&actor, "train");
        assert(pc.powers[0] == 1 && actor.exp == 0);
        pc.powers[0] = 8; recover(&actor); armor = actor.armor;
        if (i) do_demon(&actor, "shield"); else do_angel(&actor, "aura");
        assert(actor.armor == armor + 20);
        mana = actor.mana; recover(&actor);
        if (i) do_demon(&actor, "shield"); else do_angel(&actor, "aura");
        assert(actor.armor == armor + 20 && actor.mana == mana);
        recover(&actor);
        if (i) do_demon(&actor, "nightsight"); else do_angel(&actor, "nightsight");
        assert(newclasses_nightvision(&actor));
        recover(&actor);
        if (i) do_demon(&actor, "wings"); else do_angel(&actor, "wings");
        assert(IS_AFFECTED(&actor, AFF_FLYING));
        memset(&af, 0, sizeof(af)); af.type = skill_lookup("fly");
        af.duration = 3; af.bitvector = AFF_FLYING; affect_to_char(&actor, &af);
        affect_strip(&actor, gsn_parent_wings);
        assert(IS_AFFECTED(&actor, AFF_FLYING));
        affect_strip(&actor, af.type); assert(!IS_AFFECTED(&actor, AFF_FLYING));
        recover(&actor);
        if (i) do_demon(&actor, "read_aura Target"); else do_angel(&actor, "read_aura Target");
        assert(strstr(output, "alignment is -500 (evil)") != NULL);
        recover(&actor); SET_BIT(room.room_flags, ROOM_SAFE); mana = actor.mana;
        if (i) do_demon(&actor, "hellfire Target"); else do_angel(&actor, "halo Target");
        assert(actor.mana == mana);
        REMOVE_BIT(room.room_flags, ROOM_SAFE);
        hp = victim.hit;
        if (i) do_demon(&actor, "hellfire Target"); else do_angel(&actor, "halo Target");
        assert(victim.hit < hp && actor.mana < mana);
        stop_fighting(&actor, TRUE); actor.fight_timer = 0; actor.position = POS_STANDING;
        recover(&actor); mana = actor.mana;
        if (i) do_demon(&actor, "travel"); else do_angel(&actor, "travel");
        assert(actor.mana == mana && actor.in_room == &room);
        expanded_kit(&actor, &victim, &mob, i != 0);
        reference_shared_kit(&actor, &victim);
        newclasses_clear(&actor);
        assert(actor.armor == 0 && actor.hitroll == 0 && actor.damroll == 0);
    }
    actor.class = CLASS_CYBORG; actor.act = ACT_IS_NPC; nano = pc.cyber.nanites;
    do_cyborg(&actor, "train"); do_angel(&actor, "train"); do_demon(&actor, "train");
    assert(!cyborg_has_component(&actor, CY_HEART) && pc.cyber.nanites == nano);
    actor.act = 0; actor.pcdata = NULL;
    do_cyborg(&actor, "status"); do_angel(&actor, "train"); do_demon(&actor, "train");
    assert(!newclasses_nightvision(&actor));
    char_list = NULL;
    puts("Angel/Demon enrollment, progression, nonstacking buffs, flight ownership, NPC aura reading, protected/free failures, damage, cleanup and missing-state guards: passed");
    return 0;
}
