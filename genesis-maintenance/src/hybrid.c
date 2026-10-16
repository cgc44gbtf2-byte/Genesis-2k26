/*
 * Read-only development preview. Does not convert characters, debit resources,
 * grant abilities, or change the old class/save format.
 */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "parents.h"

static void hybrid_roster_row(CHAR_DATA *ch,const GENESIS_HYBRID *hybrid)
{
    char buf[256];
    const GENESIS_PARENT *first=genesis_parent_find(hybrid->first);
    const GENESIS_PARENT *second=genesis_parent_find(hybrid->second);
    if(!first || !second) return;
    snprintf(buf,sizeof(buf),"  %d  %-26s %s + %s\n\r",
        hybrid->id,hybrid->name,first->name,second->name);
    stc(buf,ch);
}

void do_hybrid(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], extra[MAX_INPUT_LENGTH];
    char second[MAX_INPUT_LENGTH],tail[MAX_INPUT_LENGTH];
    char *rest;
    char buf[MAX_STRING_LENGTH];
    const GENESIS_PARENT *parent,*other;
    const GENESIS_PARENT_POWER *power;
    const GENESIS_HYBRID *hybrid;
    size_t i, count;

    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL) return;
    /* Legacy one_argument has no destination-size parameter. */
    if (argument == NULL || strlen(argument) >= sizeof(arg)) {
        stc("Preview only. Syntax: hybrid [parent]\n\r", ch);
        return;
    }
    argument = one_argument(argument, arg);
    rest=one_argument(argument, extra);
    if (!genesis_parent_registry_valid()) {
        stc("Hybrid preview unavailable: invalid parent registry.\n\r", ch);
        return;
    }
    if(!str_cmp(arg,"pair")) {
        rest=one_argument(rest,second);
        one_argument(rest,tail);
        if(!extra[0] || !second[0] || tail[0]) {
            stc("Preview only. Syntax: hybrid pair <first parent> <second parent>\n\r",ch);
            return;
        }
        parent=genesis_parent_named(extra); other=genesis_parent_named(second);
        hybrid=parent && other ? genesis_hybrid_for_pair(parent->id,other->id) : NULL;
        if(!hybrid) {
            stc("Unknown or identical parents. Use two exact names from hybrid.\n\r",ch);
            return;
        }
        hybrid_roster_row(ch,hybrid);
        stc("Registry preview only: conversion and inherited powers remain disabled.\n\r",ch);
        stc("Class Leader policy: hybrids may not challenge, hold, inherit or retain that position.\n\r",ch);
        stc("Existing class IDs, characters, resources and saves are unchanged.\n\r",ch);
        return;
    }
    if(!str_cmp(arg,"roster")) {
        if(*rest || (extra[0] && !(parent=genesis_parent_named(extra)))) {
            stc("Preview only. Syntax: hybrid roster [exact parent name]\n\r",ch);
            return;
        }
        if(!extra[0]) parent=NULL;
        stc("Named hybrid roster - preview only; conversion is not enabled.\n\r",ch);
        count=0;
        for(i=0;i<genesis_hybrid_count();++i) {
            hybrid=genesis_hybrid_at(i);
            if(parent && hybrid->first!=parent->id && hybrid->second!=parent->id) continue;
            hybrid_roster_row(ch,hybrid); ++count;
        }
        snprintf(buf,sizeof(buf),"%lu named combinations shown. No character state was changed.\n\r",
            (unsigned long)count);
        stc(buf,ch);
        return;
    }
    if (extra[0] != '\0') {
        stc("Preview only. Syntax: hybrid [parent]\n\r", ch);
        return;
    }
    if (arg[0] == '\0') {
        stc("Hybrid development preview - conversion is not enabled.\n\r", ch);
        sprintf(buf, "%lu planned parents; %lu distinct two-parent pairings.\n\r",
            (unsigned long)genesis_parent_count(),
            (unsigned long)genesis_parent_pair_count());
        stc(buf, ch);
        for (i = 0; i < genesis_parent_count(); ++i) {
            parent = genesis_parent_at(i);
            sprintf(buf, "  %-12s %s\n\r", parent->key,
                parent->legacy_class != 0 ? "existing standalone class"
                                         : "new class - not yet playable");
            stc(buf, ch);
        }
        stc("Use hybrid <parent> to inspect its inventory.\n\r", ch);
        stc("Use hybrid roster [parent] for named combinations, or hybrid pair <parent> <parent>.\n\r",ch);
        stc("Cyborg is both a standalone class and a hybrid parent; conversion remains disabled.\n\r", ch);
        stc("Future hybrids require mastered class tracks, stances and base stats.\n\r", ch);
        stc("Planned hybrids inherit all powers from two parents, not a 75% selection.\n\r", ch);
        stc("Pure-class ascension is a separate path; expensive respecs will allow switching.\n\r", ch);
        stc("No class, powers, resources or character files are changed by this command.\n\r", ch);
        return;
    }
    parent = genesis_parent_named(arg);
    if (parent == NULL) {
        stc("Unknown parent. Use hybrid to see exact parent names.\n\r", ch);
        return;
    }
    sprintf(buf, "%s inventory - incomplete; NOT a playable hybrid kit.\n\r",
        parent->name);
    stc(buf, ch);
    count = genesis_parent_power_count(parent->id);
    if (count == 0) {
        stc("Ability inventory has not yet been catalogued here.\n\r", ch);
        stc("Existing standalone class commands, if any, remain unchanged.\n\r", ch);
        return;
    }
    for (i = 0; i < count; ++i) {
        power = genesis_parent_power_at(parent->id, i);
        sprintf(buf, "  %-24s %-7s [%s; %s]\n\r",
            power->key, power->kind == GEN_POWER_SYSTEM ? "system"
                      : power->kind == GEN_POWER_PASSIVE ? "passive" : "active",
            power->sources, power->implemented ? "standalone effect; hybrid blocked"
                                              : "effect not ported");
        stc(buf, ch);
    }
    stc("Sources: D=Deluxe E=Ethereal L=GWLow5 U=UMGW V=Vortex M=Mindcloud A=ADG G=Genesis new design.\n\r", ch);
    stc("These are inventory entries, not final counts or selection quotas.\n\r", ch);
}
