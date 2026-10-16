/*
 * Original Genesis adapters. Component costs/dependencies credited to
 * Kale/Jalerit's Vortex Cyborg. No reference effect bodies or numeric flags
 * are copied. Conservative rules and incomplete-port scope are documented
 * in NEW_CLASSES_IMPLEMENTATION.md. Full hybrid conversion remains disabled.
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include "merc.h"
#include "parents.h"

static int reference_command(CHAR_DATA *ch, char *argument);
static void reference_menu(CHAR_DATA *ch);
static void reference_tick(CHAR_DATA *ch);
static void reference_clear(CHAR_DATA *ch);
static void reference_loaded(CHAR_DATA *ch);
static void reference_restore(CHAR_DATA *ch);
static int reference_granted(CHAR_DATA *ch, const char *key);
static void legacy_sync(CHAR_DATA *ch);
static void legacy_tick(CHAR_DATA *ch);
static int legacy_command(CHAR_DATA *ch, const char *cmd, char *arg, char *extra);
static void legacy_offer(CHAR_DATA *ch, char *arg);
static void legacy_menu(CHAR_DATA *ch);
static int legacy_warp(CHAR_DATA *ch, int bit);
static void legacy_clear(CHAR_DATA *ch);

static int parent_actor(CHAR_DATA *ch)
{
    return ch != NULL && !IS_NPC(ch) && ch->pcdata != NULL
        && (IS_ANGEL(ch) || IS_DEMON(ch) || IS_CYBORG(ch));
}

static long parent_rank(CHAR_DATA *ch)
{
    if (!parent_actor(ch) || IS_CYBORG(ch)) return 0;
    return ch->pcdata->powers[0] >= 0 && ch->pcdata->powers[0] <= 8
        ? ch->pcdata->powers[0] : 0;
}

long cyborg_system_rank(CHAR_DATA *ch, int system)
{
    if (!parent_actor(ch) || !IS_CYBORG(ch)
        || !cyborg_systems_valid(&ch->pcdata->cybernetics)) return 0;
    switch (system) {
    case 0: return ch->pcdata->cybernetics.face;
    case 1: return ch->pcdata->cybernetics.limbs;
    case 2: return ch->pcdata->cybernetics.body;
    case 3: return ch->pcdata->cybernetics.demolition;
    }
    return 0;
}

long cyborg_melee_damage(CHAR_DATA *ch, long dam)
{
    long limbs = cyborg_system_rank(ch, 1);
    if (dam <= 0 || limbs == 0) return dam;
    dam = UMIN(dam, LONG_MAX / 4);
    dam += dam * 3 / 10;
    if (limbs >= 3) dam += dam / 2;
    return dam;
}

long cyborg_absorb_damage(CHAR_DATA *ch, long dam)
{
    if (dam <= 0 || cyborg_system_rank(ch, 2) == 0) return dam;
    return (dam / 7) * 4 + (dam % 7) * 4 / 7;
}

int cyborg_has_component(CHAR_DATA *ch, int id)
{
    return parent_actor(ch) && IS_CYBORG(ch)
        && cyborg_core_valid(&ch->pcdata->cyborg)
        && cyborg_runtime_valid(&ch->pcdata->cyber)
        && (ch->pcdata->cyborg.installed & cyborg_component_bit(id)) != 0;
}

static void parent_affect(CHAR_DATA *ch, long sn, long location,
                          long modifier, long duration, long bits)
{
    AFFECT_DATA af;
    memset(&af, 0, sizeof(af));
    af.type = sn; af.location = location; af.modifier = modifier;
    af.duration = duration; af.bitvector = bits;
    affect_to_char(ch, &af);
}

static void cyber_trait(CHAR_DATA *ch, int id, long location, long modifier)
{
    if (cyborg_has_component(ch, id))
        parent_affect(ch, gsn_cyber_traits, location, modifier, -1, 0);
}

static void cyborg_sync(CHAR_DATA *ch)
{
    if (ch == NULL) return;
    affect_strip(ch, gsn_cyber_traits);
    if (!parent_actor(ch) || !IS_CYBORG(ch)) return;
    cyber_trait(ch, CY_HEART, APPLY_CON, 1);
    cyber_trait(ch, CY_NEUROPROCESSOR, APPLY_INT, 1);
    cyber_trait(ch, CY_LEFT_ARM, APPLY_AC, 5);
    cyber_trait(ch, CY_RIGHT_ARM, APPLY_AC, 5);
    cyber_trait(ch, CY_LEFT_LEG, APPLY_DEX, 1);
    cyber_trait(ch, CY_RIGHT_LEG, APPLY_DEX, 1);
    cyber_trait(ch, CY_ADRENAL_BOOST, APPLY_DAMROLL, 5);
    cyber_trait(ch, CY_BOOSTERWARE, APPLY_HITROLL, 5);
    cyber_trait(ch, CY_BOOSTMASTER, APPLY_HITROLL, 10);
    cyber_trait(ch, CY_WOLVERS, APPLY_DAMROLL, 10);
    cyber_trait(ch, CY_MUSCLE, APPLY_STR, 1);
    cyber_trait(ch, CY_BONELACE, APPLY_AC, 20);
    cyber_trait(ch, CY_TARGETCOMP, APPLY_HITROLL, 10);
    cyber_trait(ch, CY_TELEOPTICS, APPLY_HITROLL, 5);
    if (cyborg_system_rank(ch, 2) > 0)
        parent_affect(ch, gsn_cyber_traits, APPLY_AC,
                      20 * cyborg_system_rank(ch, 2), -1, 0);
    if (cyborg_system_rank(ch, 1) > 0) {
        parent_affect(ch, gsn_cyber_traits, APPLY_HITROLL,
                      10 * cyborg_system_rank(ch, 1), -1, 0);
        parent_affect(ch, gsn_cyber_traits, APPLY_AC,
                      10 * cyborg_system_rank(ch, 1), -1, 0);
    }
    if (!cyborg_has_component(ch, CY_NANITE_FACTORY))
        ch->pcdata->cyber.factory = 0;
    if (!cyborg_has_component(ch, CY_BATTERY))
        ch->pcdata->cyber.nanites = 0;
    if (!cyborg_has_component(ch, CY_CHAMELEON) && cyborg_system_rank(ch, 2) < 4)
        affect_strip(ch, gsn_parent_cloak);
}

/* Mastery grants derived passives, never permanent trained-stat changes.
 * Serialized traits are discarded and rebuilt, including after training.
 */
static void parent_traits_sync(CHAR_DATA *ch)
{
    affect_strip(ch, gsn_parent_traits);
    if (!parent_actor(ch) || IS_CYBORG(ch)) return;
    if (parent_rank(ch) >= 4 || reference_granted(ch, "might"))
        parent_affect(ch, gsn_parent_traits, APPLY_STR, 1, -1, 0);
    if (parent_rank(ch) >= 5 || reference_granted(ch, "toughness"))
        parent_affect(ch, gsn_parent_traits, APPLY_CON, 1, -1, 0);
    if (parent_rank(ch) >= 6 || reference_granted(ch, "speed"))
        parent_affect(ch, gsn_parent_traits, APPLY_DEX, 1, -1, 0);
}

/* Called after effect purges and death resets. Only inherent mastery and
 * installed hardware return: never wings, wards, aspects or camouflage.
 */
void newclasses_restore_passives(CHAR_DATA *ch)
{
    crafted_restore(ch);
    lineage_restore(ch);
    reference_restore(ch);
    if (!parent_actor(ch)) return;
    if (IS_CYBORG(ch)) cyborg_sync(ch);
    else parent_traits_sync(ch);
    legacy_sync(ch);
}

static void parent_body_loaded(CHAR_DATA *ch)
{
    if (!parent_actor(ch) || IS_CYBORG(ch) || (parent_rank(ch) < 3
        && !reference_granted(ch, IS_ANGEL(ch) ? "feet" : "hooves")))
        affect_strip(ch, gsn_parent_feet);
    if (!IS_DEMON(ch) || (parent_rank(ch) < 4 && !reference_granted(ch,"horns")))
        affect_strip(ch, gsn_parent_horns);
    if (!IS_DEMON(ch) || (parent_rank(ch) < 5 && !reference_granted(ch,"tail")))
        affect_strip(ch, gsn_parent_tail);
}

void newclasses_clear(CHAR_DATA *ch)
{
    if (ch == NULL) return;
    reference_clear(ch);
    crafted_clear(ch);
    lineage_clear(ch);
    legacy_clear(ch);
    affect_strip(ch, gsn_parent_traits);
    affect_strip(ch, gsn_parent_feet);
    affect_strip(ch, gsn_parent_horns);
    affect_strip(ch, gsn_parent_tail);
    affect_strip(ch, gsn_cyber_traits);
    affect_strip(ch, gsn_parent_ward);
    affect_strip(ch, gsn_parent_aspect);
    affect_strip(ch, gsn_parent_wings);
    affect_strip(ch, gsn_parent_senses);
    affect_strip(ch, gsn_parent_cloak);
    affect_strip(ch, gsn_parent_recovery);
    if (ch->pcdata != NULL) {
        cyborg_core_init(&ch->pcdata->cyborg, 0, 0);
        memset(&ch->pcdata->cyber, 0, sizeof(ch->pcdata->cyber));
        memset(&ch->pcdata->cybernetics, 0, sizeof(ch->pcdata->cybernetics));
        memset(&ch->pcdata->legacy, 0, sizeof(ch->pcdata->legacy));
    }
}

void newclasses_loaded(CHAR_DATA *ch)
{
    if (ch == NULL || ch->pcdata == NULL || IS_NPC(ch)) return;
    reference_loaded(ch);
    crafted_loaded(ch);
    lineage_loaded(ch);
    legacy_clear(ch);
    if (!reference_legacy_valid(&ch->pcdata->legacy))
        memset(&ch->pcdata->legacy, 0, sizeof(ch->pcdata->legacy));
    if (!IS_DEMON(ch)) {
        ch->pcdata->legacy.souls = 0;
        ch->pcdata->legacy.gifts = ch->pcdata->legacy.warps = 0;
        ch->pcdata->legacy.patron[0] = 0;
        if (!IS_ANGEL(ch))
            ch->pcdata->legacy.grants[0] = ch->pcdata->legacy.grants[1] = 0;
    }
    if (ch->class != CLASS_NONE) ch->pcdata->legacy.willing = 0;
    /* Serialized derived traits are never authority; rebuild from components.
     * Do not remove hindrance on nonmembers: it is an enemy-applied debuff.
     */
    cyborg_sync(ch);
    if (!parent_actor(ch)) {
        affect_strip(ch, gsn_parent_ward);
        affect_strip(ch, gsn_parent_aspect);
        affect_strip(ch, gsn_parent_wings);
        affect_strip(ch, gsn_parent_senses);
        affect_strip(ch, gsn_parent_cloak);
        affect_strip(ch, gsn_parent_recovery);
    } else if (IS_CYBORG(ch)) {
        /* Cyborg cannot legitimately own Angel/Demon self-buffs. */
        affect_strip(ch, gsn_parent_ward);
        affect_strip(ch, gsn_parent_aspect);
        affect_strip(ch, gsn_parent_wings);
        affect_strip(ch, gsn_parent_senses);
    } else {
        if (ch->pcdata->powers[0] < 0 || ch->pcdata->powers[0] > 8)
            ch->pcdata->powers[0] = 0;
        if (parent_rank(ch) < (IS_ANGEL(ch) ? 1 : 3)
            && !reference_granted(ch,"aura") && !reference_granted(ch,"shield")
            && !reference_granted(ch,IS_ANGEL(ch) ? "angelarmour" : "demonarmour")
            && !reference_granted(ch,IS_ANGEL(ch) ? "windwall" : "curse"))
            affect_strip(ch, gsn_parent_ward);
        if (parent_rank(ch) < 2) {
            if (!reference_granted(ch,IS_ANGEL(ch) ? "angelform" : "demonform")
                && !reference_granted(ch,"rage")) affect_strip(ch, gsn_parent_aspect);
            if (!reference_granted(ch,"nightsight") && !reference_granted(ch,"shadowsight")
                && !reference_granted(ch,"heightened_senses") && !reference_granted(ch,"truesight"))
                affect_strip(ch, gsn_parent_senses);
        }
        if (parent_rank(ch) < 3 && !reference_granted(ch,"wings")) affect_strip(ch, gsn_parent_wings);
        affect_strip(ch, gsn_parent_cloak);
    }
    parent_traits_sync(ch);
    parent_body_loaded(ch);
    legacy_sync(ch);
}

int newclasses_nightvision(CHAR_DATA *ch)
{
    return lineage_nightvision(ch) || (parent_actor(ch) && IS_DEMON(ch)
            && is_affected(ch, skill_lookup("parent deathvision")))
        || cyborg_has_component(ch, CY_EYES)
        || (cyborg_system_rank(ch, 0) >= 1
            && is_affected(ch, skill_lookup("cyber sight")))
        || (parent_actor(ch) && !IS_CYBORG(ch) && (parent_rank(ch) >= 2
             || reference_granted(ch,"nightsight") || reference_granted(ch,"shadowsight")
             || reference_granted(ch,"heightened_senses") || reference_granted(ch,"truesight"))
            && is_affected(ch, gsn_parent_senses));
}

int newclasses_perception(CHAR_DATA *ch)
{
    if (lineage_perception(ch)) return 1;
    if (legacy_warp(ch, 10)) return 1;
    if (parent_actor(ch) && !IS_CYBORG(ch) && parent_rank(ch) >= 4
        && ch->pcdata->reference_familiar == 3 && ch->pcdata->familiar != NULL
        && is_affected(ch, skill_lookup("parent companion"))) return 1;
    AFFECT_DATA *af;
    if (cyborg_has_component(ch, CY_EARS)
        || (cyborg_system_rank(ch, 0) >= 2
            && is_affected(ch, skill_lookup("cyber sight")))) return 1;
    if (!parent_actor(ch) || IS_CYBORG(ch) || (parent_rank(ch) < 5
        && !reference_granted(ch,"heightened_senses") && !reference_granted(ch,"truesight"))) return 0;
    for (af = ch->affected; af != NULL; af = af->next)
        if (af->type == gsn_parent_senses && af->modifier == 2) return 1;
    return 0;
}

static int management_ready(CHAR_DATA *ch)
{
    if (!ch || IS_NPC(ch) || !ch->pcdata || !ch->in_room || ch->hit<=0
        || is_affected(ch,gsn_lycaon)
        || IS_EXTRA(ch,EXTRA_OSWITCH) || IS_MORE(ch,MORE_HEAD)) return 0;
    if (IS_BUI(ch)) {
        stc("Staff characters cannot train or modify class systems.\n\r"
            "Use a mortal test character.\n\r", ch);
        return 0;
    }
    if (ch->fighting != NULL || ch->fight_timer > 0
        || ch->position < POS_STANDING) {
        stc("Class training and modifications require peace and standing.\n\r", ch);
        return 0;
    }
    return 1;
}

static int power_ready(CHAR_DATA *ch, long mana, long nanites)
{
    if (ch->in_room == NULL || ch->hit <= 0
        || ch->position < POS_FIGHTING || is_affected(ch, gsn_lycaon)) {
        stc("You cannot use that class power now.\n\r", ch);
        return 0;
    }
    if (is_affected(ch, gsn_parent_recovery)) {
        stc("Your class power is still recovering.\n\r", ch);
        return 0;
    }
    if (ch->mana < mana || (nanites > 0
        && (!cyborg_runtime_valid(&ch->pcdata->cyber)
            || ch->pcdata->cyber.nanites < nanites))) {
        stc("You lack the mana or nanites for that power.\n\r", ch);
        return 0;
    }
    return 1;
}

static void power_commit(CHAR_DATA *ch, long mana, long nanites, long recovery)
{
    ch->mana -= mana;
    if (nanites > 0) ch->pcdata->cyber.nanites -= nanites;
    affect_strip(ch, gsn_parent_cloak);
    WAIT_STATE(ch, 12);
    parent_affect(ch, gsn_parent_recovery, APPLY_NONE, 0, recovery, 0);
}

static CHAR_DATA *parent_enemy(CHAR_DATA *ch, char *name)
{
    CHAR_DATA *v = name[0] ? get_char_room(ch, name) : ch->fighting;
    if (v == NULL || v == ch || v->in_room != ch->in_room
        || !can_see(ch, v) || IS_BUI(v) || is_same_group(ch, v)
        || IS_SET(ch->in_room->room_flags, ROOM_SAFE)
        || (!IS_NPC(v) && (!CAN_PK(ch) || !CAN_PK(v))) || is_safe(ch, v)) {
        stc("That is not a legal class-power combat target here.\n\r", ch);
        return NULL;
    }
    return v;
}

void cyborg_update(CHAR_DATA *ch)
{
    CYBORG_RUNTIME *r;
    long generated;
    reference_tick(ch);
    if (!parent_actor(ch) || !IS_CYBORG(ch)
        || !cyborg_core_valid(&ch->pcdata->cyborg)
        || !cyborg_runtime_valid(&ch->pcdata->cyber)
        || ch->hit <= 0 || ch->position < POS_RESTING
        || is_affected(ch, gsn_lycaon)) return;
    r = &ch->pcdata->cyber;
    if (!r->factory || !cyborg_has_component(ch, CY_NANITE_FACTORY)) return;
    generated = UMIN(100, 10000 - r->nanites);
    if (generated > 0 && ch->mana >= generated) {
        ch->mana -= generated; r->nanites += generated;
    }
    if (cyborg_has_component(ch, CY_LIVER) && r->nanites >= 25
        && is_affected(ch, gsn_poison)) {
        r->nanites -= 25;
        affect_strip(ch, gsn_poison);
        stc("Your cybernetic liver removes the poison.\n\r", ch);
    }
}

static long positive_number(const char *text)
{
    char *end;
    long value;
    if (*text < '1' || *text > '9') return 0;
    errno = 0; value = strtol(text, &end, 10);
    return errno == ERANGE || *end != 0 || value <= 0 ? 0 : value;
}

static void cyborg_menu(CHAR_DATA *ch, const char *section)
{
    CYBORG_CORE *core = &ch->pcdata->cyborg;
    CYBORG_RUNTIME *r = &ch->pcdata->cyber;
    CYBORG_SYSTEMS *s = &ch->pcdata->cybernetics;
    const CYBORG_COMPONENT *c;
    const char *state;
    char buf[256];
    size_t i;
    int all = !str_cmp(section, "all");
    int implants = all || !str_cmp(section, "implants");
    int powers = all || !str_cmp(section, "powers");
    if (all || (!implants && !powers)) {
        stc("CYBORG | Overview\n\r", ch);
        stc("Resource      Current / Limit     Resource      Current / Limit\n\r"
            "------------  ------------------  ------------  ------------------\n\r", ch);
        snprintf(buf, sizeof(buf), "Humanity      %7ld / %-8ld  Nanites       %7ld / 10000\n\r",
            core->humanity, core->humanity_limit, r->nanites); stc(buf, ch);
        snprintf(buf, sizeof(buf), "Bionics       %7ld / 10000000  Bombs         %7ld / 20 (%s)\n\r",
            s->bionic, s->bombs, s->armed ? "armed" : "safe"); stc(buf, ch);
        snprintf(buf, sizeof(buf), "Primal        %7ld             Factory       %s\n\r",
            ch->primal, r->factory ? "on" : "off"); stc(buf, ch);
        stc("\n\rTraining command                 Rank / Cap   Next cost\n\r"
            "-------------------------------  ----------   -----------------\n\r", ch);
        snprintf(buf, sizeof(buf), "train                            %4ld / 12    %ld XP\n\r",
            r->rifle, r->rifle < 12 ? 10000 * (r->rifle + 1) * (r->rifle + 1) : 0); stc(buf, ch);
        snprintf(buf, sizeof(buf), "face_implants improve            %4ld / 5     %ld bionics\n\r",
            s->face, s->face < 5 ? 12500L << s->face : 0); stc(buf, ch);
        snprintf(buf, sizeof(buf), "limb_implants improve             %4ld / 5     %ld bionics\n\r",
            s->limbs, s->limbs < 5 ? 12500L << s->limbs : 0); stc(buf, ch);
        snprintf(buf, sizeof(buf), "body_implants improve             %4ld / 6     %ld bionics\n\r",
            s->body, s->body < 6 ? 12500L << s->body : 0); stc(buf, ch);
        snprintf(buf, sizeof(buf), "demolitions improve               %4ld / 12    %ld primal\n\r",
            s->demolition, s->demolition < 12 ? 15 * (s->demolition + 1) : 0); stc(buf, ch);
        stc("Costs shown at cap are 0; capped tracks cannot be trained further.\n\r"
            "bionics <XP>: convert XP to bionics. Training requires peace/standing.\n\r"
            "\n\rBrowse with: cyborg implants | cyborg powers | cyborg all\n\r"
            "Use: cyborg install <component> | cyborg uninstall <component>\n\r"
            "All commands in these tables need the cyborg prefix. HELP CYBORG.\n\r", ch);
        if (IS_BUI(ch))
            stc("Staff rank blocks training/installation; use a mortal test character.\n\r", ch);
    }
    if (implants) {
        stc("\n\rCYBORG | Implants\n\r"
            "Component           Primal  Humanity  Installation\n\r"
            "------------------  ------  --------  ------------\n\r", ch);
        for (i = 0; i < CY_COMPONENT_COUNT; ++i) {
            c = cyborg_component_at(i);
            if (cyborg_has_component(ch, c->id)) state = "Installed";
            else if (!(core->permitted & cyborg_component_bit(c->id))) state = "Restricted";
            else if ((core->installed & c->requires) != c->requires) state = "Needs parts";
            else if (ch->primal < c->primal) state = "No primal";
            else if (core->humanity < c->humanity) state = "No humanity";
            else state = "Eligible";
            snprintf(buf, sizeof(buf), "%-18s  %6ld  %8ld  %s\n\r",
                c->key, c->primal, c->humanity, state); stc(buf, ch);
        }
        stc("Eligible = parts/resources met, not a bypass of rank/state restrictions.\n\r"
            "Install: cyborg install battery. Humanity refunds on removal; not primal.\n\r", ch);
    }
    if (powers) {
        stc("\n\rCYBORG | Component powers\n\r"
            "Command          Nanites  Requirement / operation\n\r"
            "---------------  -------  ---------------------------------------\n\r"
            "factory on/off      tick  Nanite factory; generates using mana\n\r"
            "convert <mana>         -  Factory; 1 mana -> 1 nanite\n\r"
            "repair               400  Nanite factory; restores missing HP\n\r"
            "taser <enemy>        200  Installed taser\n\r"
            "snipe <enemy>        300  Railgun and rifle rank 1; same room\n\r"
            "cloak               250  Chameleon; activation requires peace\n\r"
            "radio <message>        0  Installed radiosplice\n\r", ch);
        reference_menu(ch);
        stc("Rank/passive eligibility is not cast readiness; targets/costs still apply.\n\r"
            "Equipment: cyborgeq <piece> costs 100 primal. HELP CYBORG.\n\r", ch);
    }
}

void do_cyborg(CHAR_DATA *ch, char *argument)
{
    char cmd[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH], extra[MAX_INPUT_LENGTH];
    char buf[256];
    CYBORG_RUNTIME *r;
    const CYBORG_COMPONENT *c;
    CHAR_DATA *v, *receiver;
    long cost, amount, heal;
    int result;
    if (!parent_actor(ch) || !IS_CYBORG(ch)) {
        if (ch != NULL && !IS_NPC(ch)) stc("Only Cyborgs may use cybernetics.\n\r", ch);
        return;
    }
    if (argument == NULL || strlen(argument) >= MAX_INPUT_LENGTH) {
        stc("Invalid Cyborg input.\n\r", ch); return;
    }
    if (!ch->in_room || ch->hit <= 0 || ch->position < POS_RESTING
        || IS_MORE(ch, MORE_HEAD) || IS_EXTRA(ch, EXTRA_OSWITCH)
        || is_affected(ch, gsn_lycaon)) {
        stc("You cannot operate cybernetics in your present state.\n\r",ch); return;
    }
    if (reference_command(ch, argument)) return;
    argument = one_argument(argument, cmd);
    /* Radio accepts a bounded message, never a printf format string. */
    if (!str_cmp(cmd, "radio")) {
        if (!cyborg_has_component(ch, CY_RADIOSPLICE) || argument[0] == 0) {
            stc("Radio requires a radiosplice and a message.\n\r", ch); return;
        }
        snprintf(buf, sizeof(buf), "[Cybernet] %.30s: %.180s\n\r", ch->name, argument);
        for (receiver = char_list; receiver != NULL; receiver = receiver->next)
            if (receiver->desc != NULL && cyborg_has_component(receiver, CY_RADIOSPLICE))
                stc(buf, receiver);
        return;
    }
    argument = one_argument(argument, arg);
    one_argument(argument, extra);
    if (extra[0] != 0) { stc("Too many Cyborg arguments.\n\r", ch); return; }
    r = &ch->pcdata->cyber;
    if (!cyborg_core_valid(&ch->pcdata->cyborg) || !cyborg_runtime_valid(r)
        || !cyborg_systems_valid(&ch->pcdata->cybernetics)) {
        stc("Invalid cybernetic state; contact an administrator.\n\r", ch); return;
    }
    if (cmd[0] == 0 || !str_cmp(cmd, "status") || !str_cmp(cmd, "sys_status")
        || !str_cmp(cmd, "help") || !str_cmp(cmd, "systems")
        || !str_cmp(cmd, "implants") || !str_cmp(cmd, "powers")
        || !str_cmp(cmd, "all")) {
        if (arg[0]) { stc("Status takes no arguments.\n\r", ch); return; }
        cyborg_menu(ch, cmd);
        return;
    }
    if (!str_cmp(cmd, "install") || !str_cmp(cmd, "implant")
        || !str_cmp(cmd, "replace") || !str_cmp(cmd, "uninstall")) {
        if (!management_ready(ch)) return;
        c = cyborg_component_named(arg);
        if (c == NULL) { stc("Unknown component; use cyborg status.\n\r", ch); return; }
        result = !str_cmp(cmd, "uninstall")
            ? cyborg_core_uninstall(&ch->pcdata->cyborg, c->id)
            : cyborg_core_install(&ch->pcdata->cyborg, c->id, &ch->primal);
        if (result != CY_OK) {
            snprintf(buf, sizeof(buf), "Cybernetic modification rejected (%d): check prerequisites, primal, humanity and dependent components.\n\r", result);
            stc(buf, ch); return;
        }
        cyborg_sync(ch); WAIT_STATE(ch, 12);
        stc("Cybernetic modification complete.\n\r", ch); return;
    }
    if (!str_cmp(cmd, "train")) {
        if (arg[0] || !management_ready(ch)) return;
        if (r->rifle >= 12) { stc("Your rifle training is complete.\n\r", ch); return; }
        cost = 10000 * (r->rifle + 1) * (r->rifle + 1);
        if (ch->exp < cost) { stc("Not enough experience for rifle training.\n\r", ch); return; }
        ch->exp -= cost; ++r->rifle; WAIT_STATE(ch, 12);
        stc("Your rifle training advances.\n\r", ch); return;
    }
    if (!str_cmp(cmd, "factory")) {
        if (!management_ready(ch)) return;
        if (str_cmp(arg, "on") && str_cmp(arg, "off")) {
            stc("Use cyborg factory on/off.\n\r", ch); return;
        }
        if (!cyborg_has_component(ch, CY_NANITE_FACTORY)) {
            stc("Install a nanite_factory first.\n\r", ch); return;
        }
        r->factory = !str_cmp(arg, "on");
        stc(r->factory ? "Nanite factory enabled.\n\r" : "Nanite factory disabled.\n\r", ch);
        return;
    }
    if (!str_cmp(cmd, "convert")) {
        amount = positive_number(arg);
        if (!management_ready(ch) || !cyborg_has_component(ch, CY_NANITE_FACTORY)
            || amount == 0 || amount > 10000 - r->nanites || ch->mana < amount) {
            stc("Conversion requires an installed factory, available mana and nanite capacity.\n\r", ch); return;
        }
        ch->mana -= amount; r->nanites += amount; WAIT_STATE(ch, 12);
        stc("Mana converted to nanites.\n\r", ch); return;
    }
    if (!str_cmp(cmd, "repair")) {
        if (arg[0] || !cyborg_has_component(ch, CY_NANITE_FACTORY)
            || !power_ready(ch, 0, 400)) return;
        if (ch->max_hit <= 0 || ch->hit >= ch->max_hit) {
            stc("Nothing needs repair.\n\r", ch); return;
        }
        heal = UMIN(300 + 30 * r->rifle, ch->max_hit - ch->hit);
        power_commit(ch, 0, 400, 1); ch->hit += heal;
        update_pos(ch); stc("Nanites repair your wounds.\n\r", ch); return;
    }
    if (!str_cmp(cmd, "cloak")) {
        if (arg[0] || !cyborg_has_component(ch, CY_CHAMELEON)) {
            stc("Cloaking requires a chameleon implant.\n\r", ch); return;
        }
        if (is_affected(ch, gsn_parent_cloak)) {
            affect_strip(ch, gsn_parent_cloak); stc("Camouflage disabled.\n\r", ch); return;
        }
        if (ch->fighting != NULL || ch->fight_timer > 0 || !power_ready(ch, 0, 250)) return;
        power_commit(ch, 0, 250, 1);
        parent_affect(ch, gsn_parent_cloak, APPLY_NONE, 0, 3, 0);
        stc("Your chameleon camouflage activates.\n\r", ch); return;
    }
    if (!str_cmp(cmd, "taser") || !str_cmp(cmd, "snipe")) {
        int taser = !str_cmp(cmd, "taser");
        if (!cyborg_has_component(ch, taser ? CY_TASER : CY_RAILGUN)
            || (!taser && r->rifle < 1)) {
            stc("Install the weapon and complete its required training first.\n\r", ch); return;
        }
        if (!power_ready(ch, 0, taser ? 200 : 300)) return;
        v = parent_enemy(ch, arg);
        if (v == NULL || !hurt_fighting(ch, v)) return;
        amount = taser ? 900 + r->rifle * 25 : 800 + r->rifle * 50;
        if (taser && saves_spell(75, v)) amount /= 2;
        else if (taser && !is_affected(v, gsn_parent_hindrance))
            parent_affect(v, gsn_parent_hindrance, APPLY_HITROLL, -4, 1, 0);
        power_commit(ch, 0, taser ? 200 : 300, 1);
        act(taser ? "You discharge your taser at $N." : "You fire a railgun shot at $N.", ch, NULL, v, TO_CHAR);
        damage(ch, v, amount, taser ? TYPE_SPELL : TYPE_HIT, taser ? "taser discharge" : "railgun shot");
        return; /* Neither combatant is touched after damage/extraction. */
    }
    stc("That Cyborg power is unavailable; use cyborg status.\n\r", ch);
}

enum { POWER_WARD, POWER_ASPECT, POWER_SIGHT, POWER_WINGS, POWER_TRAVEL,
       POWER_AURA_READ, POWER_ATTACK, POWER_BLIND, POWER_UNNERVE, POWER_CURSE,
       POWER_CALM, POWER_FEET, POWER_HORNS, POWER_TAIL, POWER_PASSIVE,
       POWER_SHARE, POWER_STRENGTHEN, POWER_EMPOWER, POWER_PURIFY,
       POWER_FROST, POWER_SOULTWIST };
typedef struct parent_command {
    const char *key;
    int rank, kind;
    long cost, strength;
} PARENT_COMMAND;
static const PARENT_COMMAND angel_commands[] = {
    {"aura",1,POWER_WARD,750,20}, {"claws",2,POWER_ASPECT,1000,10},
    {"angelform",8,POWER_ASPECT,3000,20}, {"nightsight",2,POWER_SIGHT,250,1},
    {"wings",3,POWER_WINGS,1000,0}, {"travel",4,POWER_TRAVEL,1500,0},
    {"read_aura",1,POWER_AURA_READ,100,0},
    {"heightened_senses",5,POWER_SIGHT,500,2},
    {"windwall",6,POWER_WARD,1500,30}, {"halo",6,POWER_BLIND,1500,400},
    {"emergency_inferno",7,POWER_ATTACK,2000,600},
    {"feet",3,POWER_FEET,750,8},
    {"might",4,POWER_PASSIVE,0,0}, {"toughness",5,POWER_PASSIVE,0,0},
    {"speed",6,POWER_PASSIVE,0,0}, {"shadowsight",5,POWER_SIGHT,500,2},
    {"angelarmour",7,POWER_WARD,1800,35},
    {"spiritshare",3,POWER_SHARE,500,0},
    {"strengthen",4,POWER_STRENGTHEN,1200,0},
    {"guardianangel",6,POWER_WARD,2000,40},
    {"purifyspirit",5,POWER_PURIFY,1000,0},
    {"empower",8,POWER_EMPOWER,3000,0}, {NULL,0,0,0,0}
};
static const PARENT_COMMAND demon_commands[] = {
    {"fangs",1,POWER_ATTACK,500,300}, {"claws",2,POWER_ASPECT,1000,10},
    {"demonform",8,POWER_ASPECT,3000,20}, {"nightsight",2,POWER_SIGHT,250,1},
    {"wings",3,POWER_WINGS,1000,0}, {"travel",4,POWER_TRAVEL,1500,0},
    {"read_aura",1,POWER_AURA_READ,100,0},
    {"heightened_senses",5,POWER_SIGHT,500,2},
    {"shield",3,POWER_WARD,750,20}, {"demonarmour",6,POWER_WARD,1500,30},
    {"hellfire",7,POWER_ATTACK,2000,650},
    {"emergency_inferno",7,POWER_ATTACK,2000,600},
    {"unnerve",5,POWER_UNNERVE,1000,0}, {"curse",4,POWER_CURSE,1000,0},
    {"rage",6,POWER_ASPECT,1500,15}, {"calm",6,POWER_CALM,1000,0},
    {"hooves",3,POWER_FEET,750,8}, {"horns",4,POWER_HORNS,750,10},
    {"tail",5,POWER_TAIL,1000,10},
    {"might",4,POWER_PASSIVE,0,0}, {"toughness",5,POWER_PASSIVE,0,0},
    {"speed",6,POWER_PASSIVE,0,0},
    {"shadowsight",5,POWER_SIGHT,500,2}, {"truesight",5,POWER_SIGHT,500,2},
    {"soultwist",6,POWER_SOULTWIST,1800,0},
    {"frostbreath",4,POWER_FROST,1200,450},
    {"netherflames",8,POWER_ATTACK,2500,800},
    {NULL,0,0,0,0}
};

/* Positive support requires self or a living, visible group member.
 * No aid to unrelated combatants, hidden targets, evil NPCs or corpses.
 * Player alignment is deliberately not reconstructed.
 */
static CHAR_DATA *parent_ally(CHAR_DATA *ch, char *name)
{
    CHAR_DATA *v = !name[0] || !str_cmp(name, "self") ? ch : get_char_room(ch, name);
    if (v == NULL || v->in_room != ch->in_room || !can_see(ch, v)
        || v->hit <= 0 || v->position <= POS_DEAD || IS_BUI(v)
        || (v != ch && !is_same_group(ch, v))
        || (IS_NPC(v) && v->pIndexData != NULL
            && v->pIndexData->alignment < -350)) {
        stc("That support power requires yourself or a living visible ally here.\n\r", ch);
        return NULL;
    }
    return v;
}

static void parent_support(CHAR_DATA *ch, char *arg, const PARENT_COMMAND *p)
{
    CHAR_DATA *v;
    long heal, beast;
    if (!is_affected(ch, gsn_parent_wings)) {
        stc("Rejuvenation requires your own angelic wings.\n\r", ch); return;
    }
    v = parent_ally(ch, arg);
    if (v == NULL) return;
    if (p->kind == POWER_PURIFY) {
        if (IS_NPC(v) || v->pcdata == NULL || !IS_VAMPIRE(v)
            || v->beast <= 5 || v->beast >= 90 || v->beast == 50) {
            stc("Purify Spirit requires a willing allied Vampire with a troubled, changeable Beast.\n\r", ch);
            return;
        }
        /* Preserve the reference's two paths, but never overshoot 5..90. */
        beast = v->beast < 50 ? UMAX(5, v->beast - 5) : UMIN(90, v->beast + 10);
        power_commit(ch, p->cost, 0, 2); v->beast = beast;
        stc("Your angelic aura purifies the allied Vampire's spirit.\n\r", ch);
        return;
    }
    heal = v->max_hit > 0 && v->hit < v->max_hit ? v->max_hit - v->hit : 0;
    if (p->kind == POWER_SHARE) {
        if (v == ch || ch->hit <= 1) {
            stc("Share your spirit with another living ally.\n\r", ch); return;
        }
        heal = UMIN(heal, UMIN(1000, ch->hit / 10));
    } else if (p->kind == POWER_STRENGTHEN) {
        heal = UMIN(heal, 500);
    } else {
        if (ch->fighting != NULL || v->fighting != NULL
            || ch->fight_timer > 0 || v->fight_timer > 0) {
            stc("Empower requires peace for both you and your ally.\n\r", ch); return;
        }
        heal = UMIN(heal, 2000);
    }
    if (heal <= 0) {
        stc("That ally needs no restoration from this power.\n\r", ch); return;
    }
    power_commit(ch, p->cost, 0, p->kind == POWER_EMPOWER ? 3 : 2);
    if (p->kind == POWER_SHARE) ch->hit -= heal;
    v->hit += heal;
    update_pos(v);
    act("Your angelic power restores $N.", ch, NULL, v, TO_CHAR);
}

static void parent_command(CHAR_DATA *ch, char *argument, int angel)
{
    const PARENT_COMMAND *p, *table = angel ? angel_commands : demon_commands;
    char cmd[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH], extra[MAX_INPUT_LENGTH];
    char buf[256];
    CHAR_DATA *v;
    ROOM_INDEX_DATA *before, *home;
    long cost, sn = 0, amount;
    int saved;
    if (!parent_actor(ch) || (angel ? !IS_ANGEL(ch) : !IS_DEMON(ch))) {
        if (ch != NULL && !IS_NPC(ch)) stc("That power belongs to another class.\n\r", ch);
        return;
    }
    if (argument == NULL || strlen(argument) >= MAX_INPUT_LENGTH) {
        stc("Invalid class input.\n\r", ch); return;
    }
    if (IS_MORE(ch, MORE_HEAD)) {
        stc("You cannot use class powers in that state.\n\r", ch); return;
    }
    if (reference_command(ch, argument)) return;
    if (IS_EXTRA(ch, EXTRA_OSWITCH)) {
        stc("Only your object-form powers are available now.\n\r", ch); return;
    }
    argument = one_argument(argument, cmd);
    argument = one_argument(argument, arg); one_argument(argument, extra);
    if (extra[0]) { stc("Too many class-power arguments.\n\r", ch); return; }
    if (cmd[0] == 0) {
        snprintf(buf, sizeof(buf), "%s mastery: %ld/8. Use %s train.\n\r",
                 angel ? "Angel" : "Demon", parent_rank(ch), angel ? "angel" : "demon");
        stc(buf, ch);
        stc("Core power             Rank     Mana  Mastery\n\r"
            "--------------------  ----  -------  -------\n\r", ch);
        for (p = table; p->key != NULL; ++p) {
            snprintf(buf, sizeof(buf), "%-20s  %4d  %7ld  %s\n\r",
                p->key, p->rank, p->cost,
                parent_rank(ch) >= p->rank || reference_granted(ch, p->key) ? "Known" : "Locked");
            stc(buf, ch);
        }
        stc("Automatic mastery passives: Might +1 STR, Toughness +1 CON, Speed +1 DEX.\n\r"
            "Known = rank requirement met; targets, resources and state still apply.\n\r", ch);
        stc("Reference powers use Genesis rules. Full hybrids remain disabled.\n\r", ch);
        reference_menu(ch);
        return;
    }
    if (!str_cmp(cmd, "train")) {
        if (arg[0] || !management_ready(ch)) return;
        if (parent_rank(ch) >= 8) { stc("Your class mastery is complete.\n\r", ch); return; }
        if (ch->pcdata->powers[0] < 0 || ch->pcdata->powers[0] > 8) {
            stc("Invalid mastery state; contact an administrator.\n\r", ch); return;
        }
        cost = 50000 * (parent_rank(ch) + 1) * (parent_rank(ch) + 1);
        if (ch->exp < cost) { stc("Not enough experience for class training.\n\r", ch); return; }
        ch->exp -= cost; ++ch->pcdata->powers[0]; WAIT_STATE(ch, 12);
        parent_traits_sync(ch);
        stc("Your class mastery advances.\n\r", ch); return;
    }
    for (p = table; p->key != NULL && str_cmp(p->key, cmd); ++p) {}
    if (p->key == NULL) { stc("That reference ability is not yet available in this kit.\n\r", ch); return; }
    if (parent_rank(ch) < p->rank && !reference_granted(ch, p->key)) {
        stc("You have not mastered that power.\n\r", ch); return;
    }
    if (p->kind == POWER_PASSIVE) {
        if (arg[0]) { stc("Passive powers take no target.\n\r", ch); return; }
        parent_traits_sync(ch);
        stc("That passive is active automatically at your mastery rank.\n\r", ch); return;
    }
    if (!power_ready(ch, p->cost, 0)) return;
    if (p->kind == POWER_SHARE || p->kind == POWER_STRENGTHEN
        || p->kind == POWER_EMPOWER || p->kind == POWER_PURIFY) {
        parent_support(ch, arg, p); return;
    }
    if (p->kind == POWER_WARD) sn = gsn_parent_ward;
    if (p->kind == POWER_ASPECT) sn = gsn_parent_aspect;
    if (p->kind == POWER_SIGHT) sn = gsn_parent_senses;
    if (p->kind == POWER_WINGS) sn = gsn_parent_wings;
    if (p->kind == POWER_FEET) sn = gsn_parent_feet;
    if (p->kind == POWER_HORNS) sn = gsn_parent_horns;
    if (p->kind == POWER_TAIL) sn = gsn_parent_tail;
    if (sn != 0) {
        if (arg[0]) { stc("That power takes no target.\n\r", ch); return; }
        if (is_affected(ch, sn) || (p->kind == POWER_WINGS && IS_AFFECTED(ch, AFF_FLYING))) {
            stc("That effect is already active.\n\r", ch); return;
        }
        power_commit(ch, p->cost, 0, 1);
        if (p->kind == POWER_WARD) parent_affect(ch, sn, APPLY_AC, p->strength, 3, 0);
        if (p->kind == POWER_ASPECT) {
            parent_affect(ch, sn, APPLY_HITROLL, p->strength, 3, 0);
            parent_affect(ch, sn, APPLY_DAMROLL, p->strength, 3, 0);
        }
        if (p->kind == POWER_SIGHT) parent_affect(ch, sn, APPLY_NONE, p->strength, 3, 0);
        if (p->kind == POWER_WINGS) parent_affect(ch, sn, APPLY_NONE, 0, 3, AFF_FLYING);
        if (p->kind == POWER_FEET) parent_affect(ch, sn, APPLY_DAMROLL, p->strength, 3, 0);
        if (p->kind == POWER_HORNS) parent_affect(ch, sn, APPLY_HITROLL, p->strength, 3, 0);
        if (p->kind == POWER_TAIL) parent_affect(ch, sn, APPLY_AC, p->strength, 3, 0);
        stc("Your class power takes effect.\n\r", ch); return;
    }
    if (p->kind == POWER_CALM) {
        if (arg[0] || !is_affected(ch, gsn_parent_aspect)) {
            stc("You have no empowered aspect to calm.\n\r", ch); return;
        }
        power_commit(ch, p->cost, 0, 1);
        affect_strip(ch, gsn_parent_aspect); stc("You calm your infernal aspect.\n\r", ch); return;
    }
    if (p->kind == POWER_AURA_READ) {
        v = get_char_room(ch, arg);
        if (v == NULL || !IS_NPC(v) || v->pIndexData == NULL || !can_see(ch, v)) {
            stc("Read Aura requires a visible NPC here.\n\r", ch); return;
        }
        snprintf(buf, sizeof(buf), "Their alignment is %ld (%s).\n\r", v->pIndexData->alignment,
                 v->pIndexData->alignment > 350 ? "good" : v->pIndexData->alignment < -350 ? "evil" : "neutral");
        power_commit(ch, p->cost, 0, 1); stc(buf, ch); return;
    }
    if (p->kind == POWER_TRAVEL) {
        before = ch->in_room; home = ch->home > 0 ? get_room_index(ch->home) : NULL;
        if (arg[0] || home == NULL || home == before || ch->fighting != NULL
            || ch->fight_timer > 0 || IS_AFFECTED(ch, AFF_CURSE | AFF_WEBBED)
            || IS_SET(before->room_flags, ROOM_NO_RECALL | ROOM_NO_TELEPORT)
            || IS_SET(home->room_flags, ROOM_NO_TELEPORT | ROOM_PRIVATE | ROOM_SOLITARY)) {
            stc("Class travel cannot reach your home from here.\n\r", ch); return;
        }
        do_recall(ch, "home");
        if (ch->in_room == home) {
            power_commit(ch, p->cost, 0, 2); stc("Your class travel carries you home.\n\r", ch);
        }
        return;
    }
    v = parent_enemy(ch, arg);
    if (v == NULL) return;
    if (p->kind == POWER_UNNERVE && is_affected(v, gsn_parent_hindrance)) {
        stc("They are already unnerved.\n\r", ch); return;
    }
    if (p->kind == POWER_CURSE && IS_AFFECTED(v, AFF_CURSE)) {
        stc("They are already cursed.\n\r", ch); return;
    }
    if (p->kind == POWER_SOULTWIST
        && (!IS_NPC(v) || v->pIndexData == NULL || v->pIndexData->alignment <= 350)) {
        stc("Soul Twist requires a good-aligned NPC; player alignment is unavailable.\n\r", ch);
        return;
    }
    if (!hurt_fighting(ch, v)) return;
    power_commit(ch, p->cost, 0, 1);
    saved = saves_spell(75, v);
    if (p->kind == POWER_UNNERVE || p->kind == POWER_CURSE) {
        if (!saved)
            parent_affect(v, p->kind == POWER_CURSE ? gsn_curse : gsn_parent_hindrance,
                          APPLY_HITROLL, -4, 2, p->kind == POWER_CURSE ? AFF_CURSE : 0);
        stc(saved ? "They resist your infernal power.\n\r" : "Your infernal power hinders them.\n\r", ch);
        return;
    }
    amount = p->strength + parent_rank(ch) * 60;
    if (p->kind == POWER_SOULTWIST)
        amount = URANGE(351, v->pIndexData->alignment, 1000);
    if (saved) amount /= 2;
    else if (p->kind == POWER_BLIND && !IS_AFFECTED(v, AFF_BLIND))
        parent_affect(v, gsn_blindness, APPLY_HITROLL, -4, 1, AFF_BLIND);
    if (p->kind == POWER_FROST && !saved && !is_affected(v, gsn_parent_chill))
        parent_affect(v, gsn_parent_chill, APPLY_STR, -1, 2, 0);
    act("You unleash your class power against $N.", ch, NULL, v, TO_CHAR);
    damage(ch, v, amount, !str_cmp(cmd, "fangs") ? TYPE_HIT : TYPE_SPELL, (char *)p->key);
    /* No combatant dereferences after damage. */
}

void do_angel(CHAR_DATA *ch, char *argument) { parent_command(ch, argument, 1); }
void do_demon(CHAR_DATA *ch, char *argument)
{
    char cmd[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH], extra[MAX_INPUT_LENGTH];
    if (ch && !IS_NPC(ch) && ch->pcdata && ch->class == CLASS_NONE
        && argument && strlen(argument) < MAX_INPUT_LENGTH) {
        argument = one_argument(argument, cmd);
        argument = one_argument(argument, arg); one_argument(argument, extra);
        if (!str_cmp(cmd, "offersoul") && !extra[0]) legacy_offer(ch, arg);
        else stc("Use demon offersoul on/off to consent to a demonic pact.\n\r", ch);
        return;
    }
    parent_command(ch, argument, 0);
}

#include "reference_abilities.inc"
#include "reference_legacy.inc"

/* Only this narrow bridge exposes reference transients to the three new
 * lineages. The helpers own native mana/recovery charges; the caller debits
 * lineage energy only after a successful native commit.
 */
int newclasses_lineage_reference(CHAR_DATA *ch,int mode,char *arg,long mana)
{
    REFERENCE_POWER p;
    CHAR_DATA *before;
    long oldmana;
    int recovering;
    if (!lineage_valid(ch) || mode<0 || mode>4 || !arg) return 0;
    memset(&p,0,sizeof(p)); p.mana=mana;
    p.kind=mode==0 ? REF_HUMANFORM : mode==1 ? REF_GUARDIAN
        : mode==2 ? REF_EYE : mode==3 ? REF_TRANSFORM : REF_OBJECTLEAP;
    before=ch->pcdata->familiar; oldmana=ch->mana;
    recovering=is_affected(ch,gsn_parent_recovery);
    if (mode==1 || mode==2) reference_familiar_power(ch,&p,arg);
    else reference_object_power(ch,&p,arg);
    if (ch->pcdata->familiar && ch->pcdata->familiar!=before
        && ch->pcdata->familiar->wizard==ch) {
        CHAR_DATA *pet=ch->pcdata->familiar;
        free_string(pet->name);
        pet->name=str_dup(mode==2 ? "detached spirit scout" : "ancestral phantasm guardian");
        free_string(pet->short_descr); pet->short_descr=str_dup(pet->name);
        free_string(pet->long_descr); pet->long_descr=str_dup("An ancestral spirit waits here.\n\r");
    }
    return ch->mana<oldmana || (!recovering && is_affected(ch,gsn_parent_recovery));
}
