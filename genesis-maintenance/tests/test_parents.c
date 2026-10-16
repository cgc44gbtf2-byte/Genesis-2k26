#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/merc.h"
#include "../src/parents.h"
extern void fread_char(CHAR_DATA *ch, FILE *fp);

static void preview(CHAR_DATA *ch, char *argument, const char *expected)
{
    DESCRIPTOR_DATA desc;
    CHAR_DATA before;
    PC_DATA pc_before;
    char output[32768];
    memset(&desc, 0, sizeof(desc));
    memset(output, 0, sizeof(output));
    desc.outbuf = output;
    desc.outsize = sizeof(output);
    desc.fcommand = TRUE;
    desc.character = ch;
    ch->desc = &desc;
    memcpy(&before, ch, sizeof(before));
    if (ch->pcdata != NULL) memcpy(&pc_before, ch->pcdata, sizeof(pc_before));
    do_hybrid(ch, argument);
    if (expected != NULL) assert(strstr(output, expected) != NULL);
    else assert(output[0] == '\0');
    /* Even failed inputs and unavailable new parents must not mutate players. */
    assert(memcmp(&before, ch, sizeof(before)) == 0);
    if (ch->pcdata != NULL)
        assert(memcmp(&pc_before, ch->pcdata, sizeof(pc_before)) == 0);
    ch->desc = NULL;
}

int main(void)
{
    size_t i, j, k, pairs = 0, angel_count, demon_count;
    const GENESIS_PARENT *a, *b;
    const GENESIS_PARENT_POWER *p, *q;
    int low = -1, high = -1;
    CHAR_DATA actor;
    PC_DATA pc;
    char oversized[MAX_INPUT_LENGTH + 16];
    long classes[] = {CLASS_VAMPIRE, CLASS_WEREWOLF, CLASS_SORCERER,
        CLASS_DROW, CLASS_HIGHLANDER, CLASS_MAGE, CLASS_DRAGON, CLASS_NINJA,
        CLASS_MONK, CLASS_ELEM, CLASS_PHOENIX, CLASS_CYBORG, CLASS_ANGEL, CLASS_DEMON};
    assert(genesis_parent_registry_valid());
    assert(genesis_parent_count() == 19);
    assert(genesis_parent_pair_count() == 171);
    assert(genesis_hybrid_registry_valid());
    assert(genesis_hybrid_count()==171);
    assert(!genesis_hybrid_at(171) && !genesis_hybrid_at((size_t)-1));
    assert(!genesis_hybrid_find(0) && !genesis_hybrid_find(GEN_PARENT_VAMPIRE));
    assert(!genesis_hybrid_named(NULL) && !genesis_hybrid_named(""));
    assert(!genesis_hybrid_named("Blood") && !genesis_hybrid_named("Abomination"));
    assert(genesis_hybrid_named("BLOODFANG")->id==200001);
    assert(genesis_hybrid_find(200171)==genesis_hybrid_named("Phantom Pom"));
    assert(!genesis_hybrid_for_pair(GEN_PARENT_VAMPIRE,GEN_PARENT_VAMPIRE));
    assert(!genesis_hybrid_for_pair(-1,GEN_PARENT_VAMPIRE));
    for(i=0;i<genesis_parent_count();++i) {
        size_t appearances=0;
        for(j=0;j<genesis_hybrid_count();++j) {
            const GENESIS_HYBRID *h=genesis_hybrid_at(j);
            if(h->first==genesis_parent_at(i)->id || h->second==genesis_parent_at(i)->id)
                ++appearances;
        }
        assert(appearances==18);
        for(j=i+1;j<genesis_parent_count();++j) {
            const GENESIS_HYBRID *h=genesis_hybrid_for_pair(
                genesis_parent_at(i)->id,genesis_parent_at(j)->id);
            assert(h && h==genesis_hybrid_for_pair(h->second,h->first));
            assert(h==genesis_hybrid_find(h->id) && h==genesis_hybrid_named(h->name));
            assert(genesis_parent_for_class(h->id)==NULL);
            assert(!genesis_parent_pair_ready(h->first,h->second));
        }
    }
    puts("Named hybrid registry: all171 unique IDs/names/pairs, both orders,18 appearances per parent, exact lookup and blocked readiness passed");
    assert(genesis_parent_at(19) == NULL);
    assert(genesis_parent_at((size_t)-1) == NULL);
    assert(genesis_parent_named(NULL) == NULL);
    assert(genesis_parent_named("") == NULL);
    assert(genesis_parent_named("dem") == NULL);
    assert(genesis_parent_named("demon_low5") == NULL);
    assert(genesis_parent_named("DEMON")->id == GEN_PARENT_DEMON);
    assert(genesis_parent_for_class(CLASS_NONE) == NULL);
    assert(genesis_parent_for_class(99999) == NULL);
    assert(genesis_parent_for_class(GEN_PARENT_DEMON) == NULL);
    for (i = 0; i < sizeof(classes) / sizeof(classes[0]); ++i)
        assert(genesis_parent_for_class(classes[i])->legacy_class == classes[i]);
    assert(genesis_parent_for_class(CLASS_DROW)->id == GEN_PARENT_DROW);
    /* New reference identities must not become empty selectable classes. */
    assert(genesis_parent_named("KUEI-JIN")->id == GEN_PARENT_KUEIJIN);
    for (i = 14; i < 19; ++i) {
        a = genesis_parent_at(i);
        assert(!genesis_parent_ready(a->id));
        if (a->id == GEN_PARENT_DEATHKNIGHT) assert(a->legacy_class == CLASS_DEATHKNIGHT);
        else if (a->id == GEN_PARENT_MOOGLE) assert(a->legacy_class == CLASS_MOOGLE);
        else if (a->id == GEN_PARENT_GARGOYLE) assert(a->legacy_class == CLASS_GARGOYLE);
        else if (a->id == GEN_PARENT_KUEIJIN) assert(a->legacy_class == CLASS_KUEIJIN);
        else assert(a->legacy_class == CLASS_WRAITH);
        for (j = 0; j < genesis_parent_power_count(a->id); ++j)
            assert(genesis_parent_power_at(a->id, j)->implemented==0
                || genesis_parent_power_at(a->id, j)->implemented==1);
    }
    assert(genesis_parent_power_named(GEN_PARENT_KUEIJIN, "bloodlash") != NULL);
    assert(genesis_parent_power_named(GEN_PARENT_WRAITH, "arcanoi") != NULL);
    assert(genesis_parent_power_named(GEN_PARENT_GARGOYLE, "stone_resilience") != NULL);
    assert(genesis_parent_power_count(GEN_PARENT_MOOGLE) == 14);
    assert(genesis_parent_power_count(GEN_PARENT_DEATHKNIGHT) == 13);
    for (i = 0; i < genesis_parent_count(); ++i) {
        a = genesis_parent_at(i);
        assert(genesis_parent_find(a->id) == a);
        assert(genesis_parent_named(a->key) == a);
        assert(!genesis_parent_ready(a->id));
        assert(!genesis_parent_pair_valid(a->id, a->id));
        for (j = i + 1; j < genesis_parent_count(); ++j) {
            b = genesis_parent_at(j);
            ++pairs;
            assert(genesis_parent_pair_valid(a->id, b->id));
            assert(genesis_parent_pair_valid(b->id, a->id));
            assert(genesis_parent_pair_canonical(b->id, a->id, &low, &high));
            assert(low == a->id && high == b->id);
            assert(!genesis_parent_pair_ready(a->id, b->id));
        }
    }
    assert(pairs == 171);
    low = 7; high = 9;
    assert(!genesis_parent_pair_canonical(-1, GEN_PARENT_DEMON, &low, &high));
    assert(low == 7 && high == 9);
    assert(!genesis_parent_pair_canonical(GEN_PARENT_ANGEL, GEN_PARENT_DEMON, NULL, &high));
    assert(!genesis_parent_pair_canonical(GEN_PARENT_ANGEL, GEN_PARENT_DEMON, &low, &low));
    assert(!genesis_parent_pair_ready(-1, GEN_PARENT_ANGEL));
    angel_count = genesis_parent_power_count(GEN_PARENT_ANGEL);
    demon_count = genesis_parent_power_count(GEN_PARENT_DEMON);
    assert(angel_count >= 27 && demon_count > 30);
    assert(genesis_parent_power_count(-1) == 0);
    for (i = 0; i < genesis_parent_count(); ++i) {
        a = genesis_parent_at(i);
        k = genesis_parent_power_count(a->id);
        for (j = 0; j < k; ++j) {
            p = genesis_parent_power_at(a->id, j);
            assert(p != NULL && p->parent == a->id);
            assert(p->implemented == 0 || p->implemented == 1);
            if (a->id == GEN_PARENT_MOOGLE || a->id == GEN_PARENT_DEATHKNIGHT)
                assert(p->implemented);
            if (a->id == GEN_PARENT_GARGOYLE || a->id == GEN_PARENT_KUEIJIN || a->id == GEN_PARENT_WRAITH)
                assert(p->implemented);
            assert(genesis_parent_power_named(a->id, p->key) == p);
        }
        assert(genesis_parent_power_at(a->id, k) == NULL);
    }
    p = genesis_parent_power_named(GEN_PARENT_DEMON, "leech");
    assert(p != NULL && strcmp(p->sources, "L U G") == 0);
    assert(p->implemented);
    assert(genesis_parent_power_named(GEN_PARENT_ANGEL, "spiritshare")->implemented);
    assert(genesis_parent_power_named(GEN_PARENT_DEMON, "frostbreath")->implemented);
    assert(genesis_parent_power_named(GEN_PARENT_CYBORG, "repair")->implemented);
    assert(genesis_parent_power_named(GEN_PARENT_CYBORG, "launch")->implemented);
    {
        size_t implemented[4] = {0, 0, 0, 0};
        int ids[4] = {GEN_PARENT_ANGEL, GEN_PARENT_DEMON,
                      GEN_PARENT_CYBORG, GEN_PARENT_PHOENIX};
        for (i = 0; i < 4; ++i)
            for (j = 0; j < genesis_parent_power_count(ids[i]); ++j)
                implemented[i] += genesis_parent_power_at(ids[i], j)->implemented;
        assert(implemented[0] == 33 && implemented[1] == 71);
        assert(implemented[2] == 60 && implemented[3] == 8);
    }
    assert(genesis_parent_power_named(GEN_PARENT_ANGEL, "leech") == NULL);
    p = genesis_parent_power_named(GEN_PARENT_ANGEL, "speed");
    q = genesis_parent_power_named(GEN_PARENT_DEMON, "speed");
    assert(p != NULL && q != NULL && p != q && p->parent != q->parent);
    assert(genesis_parent_power_named(GEN_PARENT_DEMON, "inferno") == NULL);
    assert(genesis_parent_power_named(GEN_PARENT_DEMON, "emergency_inferno") != NULL);
    assert(genesis_parent_power_named(GEN_PARENT_DEMON, NULL) == NULL);
    printf("19 parent identities, 171 unique pairs, %lu Angel/%lu Demon inventory entries: passed\n",
        (unsigned long)angel_count, (unsigned long)demon_count);

    memset(&actor, 0, sizeof(actor));
    memset(&pc, 0, sizeof(pc));
    actor.pcdata = &pc;
    actor.class = CLASS_VAMPIRE;
    actor.mana = 54321;
    actor.exp = 12345;
    pc.powers[0] = 5;
    pc.powers[19] = 4;
    actor.stance[0] = 100;
    preview(&actor, "", "19 planned parents; 171");
    preview(&actor, "gargoyle", "Gargoyle inventory");
    preview(&actor, "kuei-jin", "Kuei-jin inventory");
    preview(&actor, "wraith", "Wraith inventory");
    preview(&actor, "moogle", "Moogle inventory");
    preview(&actor, "deathknight", "Deathknight inventory");
    preview(&actor, "", "Cyborg is both a standalone class and a hybrid parent");
    preview(&actor, "demon", "emergency_inferno");
    preview(&actor, "ANGEL", "guardianangel");
    preview(&actor, "vampire", "standalone class commands");
    preview(&actor, "cyborg", "nanite_factory");
    assert(genesis_parent_power_count(GEN_PARENT_CYBORG) == 60);
    preview(&actor, "cyborg", "stuntubes");
    assert(genesis_parent_power_named(GEN_PARENT_CYBORG, "oilspit") == NULL);
    assert(genesis_parent_power_named(GEN_PARENT_CYBORG, "terminator") == NULL);
    assert(genesis_parent_power_named(GEN_PARENT_CYBORG, "upload") == NULL);
    assert(!genesis_parent_ready(GEN_PARENT_CYBORG));
    preview(&actor, "phoenix", "ashwalk");
    preview(&actor, "unknown", "Unknown parent");
    preview(&actor, "roster", "171 named combinations shown");
    preview(&actor, "ROSTER KUEI-JIN", "18 named combinations shown");
    preview(&actor, "roster wraith", "Phantom Pom");
    preview(&actor, "pair vampire werewolf", "200001  Bloodfang");
    preview(&actor, "PAIR WEREWOLF VAMPIRE", "200001  Bloodfang");
    preview(&actor, "pair angel demon", "Nephilim");
    preview(&actor, "pair vampire vampire", "identical parents");
    preview(&actor, "pair vampire dem", "Unknown");
    preview(&actor, "pair vampire", "Preview only");
    preview(&actor, "pair vampire werewolf confirm", "Preview only");
    preview(&actor, "roster vamp", "Preview only");
    preview(&actor, "roster vampire confirm", "Preview only");
    preview(&actor, "dem", "Unknown parent");
    preview(&actor, "demon confirm", "Preview only");
    preview(&actor, "'demon'", "leech");
    preview(&actor, NULL, "Preview only");
    memset(oversized, 'd', sizeof(oversized) - 1);
    oversized[sizeof(oversized) - 1] = '\0';
    preview(&actor, oversized, "Preview only");
    actor.act = ACT_IS_NPC;
    preview(&actor, "", NULL);
    actor.act = 0;
    actor.pcdata = NULL;
    preview(&actor, "", NULL);
    puts("Real hybrid handler/parser: read-only state, invalid inputs, NPCs and missing player data passed");
    {
        CHAR_DATA loaded;
        PC_DATA loaded_pc;
        CYBORG_CORE saved;
        FILE *fp = tmpfile();
        long primal = 100;
        assert(fp != NULL);
        memset(&loaded, 0, sizeof(loaded));
        memset(&loaded_pc, 0, sizeof(loaded_pc));
        loaded.pcdata = &loaded_pc;
        loaded_pc.powers[0] = 4;
        loaded_pc.powers[19] = 9;
        assert(cyborg_core_init(&saved, cyborg_component_bit(CY_HEART), 100) == CY_OK);
        assert(cyborg_core_install(&saved, CY_HEART, &primal) == CY_OK);
        fputs("Class 2\n", fp);
        assert(cyborg_core_write_player(&saved, fp) == CY_OK);
        fputs("Exp 456\nEnd\n", fp);
        rewind(fp);
        fread_char(&loaded, fp);
        assert(memcmp(&saved, &loaded_pc.cyborg, sizeof(saved)) == 0);
        assert(loaded.class == CLASS_WEREWOLF && loaded.exp == 456);
        assert(loaded_pc.powers[0] == 4 && loaded_pc.powers[19] == 9);
        fclose(fp);
        fp = tmpfile();
        assert(fp != NULL);
        fputs("CyborgCore 2 1 1 100 96\nExp 789\nEnd\n", fp);
        rewind(fp);
        fread_char(&loaded, fp);
        assert(loaded_pc.cyborg.permitted == 0 && loaded_pc.cyborg.installed == 0);
        assert(loaded.exp == 789 && loaded.class == CLASS_WEREWOLF);
        assert(loaded_pc.powers[0] == 4 && loaded_pc.powers[19] == 9);
        fclose(fp);
    }
    puts("Real player parser: independent Cyborg round trip, malformed reset and legacy field preservation passed");
    return 0;
}
