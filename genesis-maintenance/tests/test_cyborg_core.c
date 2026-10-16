#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "../src/cyborg_core.h"

static void failed_install(CYBORG_CORE *s, int id, long *primal, int status)
{
    CYBORG_CORE before = *s;
    long old = *primal;
    assert(cyborg_core_install(s, id, primal) == status);
    assert(memcmp(s, &before, sizeof(before)) == 0 && *primal == old);
}

static void failed_decode(CYBORG_CORE *s, const char *text)
{
    CYBORG_CORE before = *s;
    assert(cyborg_core_decode(s, text) == CY_INVALID);
    assert(memcmp(s, &before, sizeof(before)) == 0);
}

int main(void)
{
    CYBORG_CORE s, before, decoded;
    const CYBORG_COMPONENT *p;
    long primal, total_primal = 0, total_humanity = 0;
    char record[CY_CORE_RECORD_SIZE], too_long[CY_CORE_RECORD_SIZE + 1];
    size_t i, j;
    int status;
    uint32_t mask;
    FILE *fp;
    char word[32], following[32];

    assert(cyborg_component_registry_valid());
    assert(CY_COMPONENT_COUNT == 24);
    assert(cyborg_component_bit(-1) == 0);
    assert(cyborg_component_bit(INT_MIN) == 0);
    assert(cyborg_component_bit(INT_MAX) == 0);
    assert(cyborg_component_bit(24) == 0);
    assert(cyborg_component_at((size_t)-1) == NULL);
    assert(cyborg_component_at(24) == NULL);
    assert(cyborg_component_named(NULL) == NULL);
    assert(cyborg_component_named("neuro") == NULL);
    assert(cyborg_component_named("NEUROPROCESSOR")->id == CY_NEUROPROCESSOR);
    assert(cyborg_component_named("neuroprocessor extra") == NULL);
    assert(cyborg_core_init(&s, CY_COMPONENT_MASK, LONG_MAX) == CY_OK);
    before = s;
    assert(cyborg_core_init(&s, UINT32_MAX, 100) == CY_INVALID);
    assert(cyborg_core_init(&s, cyborg_component_bit(CY_TASER), 100) == CY_INVALID);
    assert(cyborg_core_init(&s, 0, -1) == CY_INVALID);
    assert(memcmp(&s, &before, sizeof(s)) == 0);
    assert(cyborg_core_init(NULL, 0, 100) == CY_INVALID);
    assert(!cyborg_core_valid(NULL));
    assert(cyborg_core_install(NULL, 0, NULL) == CY_INVALID);
    assert(cyborg_core_uninstall(NULL, 0) == CY_INVALID);
    primal = LONG_MAX;
    failed_install(&s, -1, &primal, CY_INVALID);
    failed_install(&s, INT_MAX, &primal, CY_INVALID);
    assert(cyborg_core_install(&s, 0, NULL) == CY_INVALID);
    assert(cyborg_core_install(&s, 0, &s.humanity) == CY_INVALID);
    assert(cyborg_core_install(&s, 0, &s.humanity_limit) == CY_INVALID);
    assert(memcmp(&s, &before, sizeof(s)) == 0);
    puts("Cyborg registry, independent IDs, exact names and input/alias bounds passed");

    /* Every permitted component still requires actual installation of parents. */
    for (i = 0; i < CY_COMPONENT_COUNT; ++i) {
        p = cyborg_component_at(i);
        assert(cyborg_component_named(p->key) == p);
        assert(cyborg_core_init(&s, CY_COMPONENT_MASK, LONG_MAX) == CY_OK);
        primal = LONG_MAX;
        if (p->requires)
            failed_install(&s, (int)i, &primal, CY_MISSING_DEPENDENCY);
        /* Every missing ancestor separately blocks the selected descendant. */
        for (j = 0; j < i; ++j)
            assert(cyborg_core_install(&s, (int)j, &primal) == CY_OK);
        for (j = 0; j < i; ++j) {
            if ((p->requires & cyborg_component_bit((int)j)) == 0) continue;
            mask = s.installed & ~cyborg_component_bit((int)j);
            before = s;
            s.installed = mask;
            s.humanity += cyborg_component_at(j)->humanity;
            status = cyborg_core_valid(&s) ? CY_MISSING_DEPENDENCY : CY_INVALID;
            failed_install(&s, (int)i, &primal, status);
            s = before;
        }
        before = s;
        primal = p->primal - 1;
        failed_install(&s, (int)i, &primal, CY_NO_PRIMAL);
        primal = -1;
        failed_install(&s, (int)i, &primal, CY_INVALID);
        s.humanity_limit = (before.humanity_limit - before.humanity)
            + p->humanity - 1;
        s.humanity = p->humanity - 1;
        primal = p->primal;
        failed_install(&s, (int)i, &primal, CY_NO_HUMANITY);
        s = before;
        assert(cyborg_core_install(&s, (int)i, &primal) == CY_OK);
        assert(primal == 0 && s.humanity == before.humanity - p->humanity);
        assert(s.installed == (before.installed | cyborg_component_bit((int)i)));
        assert(cyborg_core_valid(&s));
        failed_install(&s, (int)i, &primal, CY_ALREADY_INSTALLED);
        total_primal += p->primal;
        total_humanity += p->humanity;
    }
    assert(cyborg_core_init(&s, 0, 100) == CY_OK);
    primal = 100;
    failed_install(&s, CY_HEART, &primal, CY_NOT_SELECTED);
    puts("All 24 component dependency chains, exact costs and atomic failure paths passed");

    assert(cyborg_core_init(&s, CY_COMPONENT_MASK, total_humanity) == CY_OK);
    primal = total_primal;
    for (i = 0; i < CY_COMPONENT_COUNT; ++i)
        assert(cyborg_core_install(&s, (int)i, &primal) == CY_OK);
    assert(s.installed == CY_COMPONENT_MASK && primal == 0 && s.humanity == 0);
    before = s;
    assert(cyborg_core_uninstall(&s, CY_LIVER) == CY_DEPENDENT_INSTALLED);
    assert(cyborg_core_uninstall(&s, CY_LEFT_ARM) == CY_DEPENDENT_INSTALLED);
    assert(memcmp(&s, &before, sizeof(s)) == 0);
    assert(cyborg_core_encode(&s, record, sizeof(record)) == CY_OK);
    assert(cyborg_core_decode(&decoded, record) == CY_OK);
    assert(memcmp(&s, &decoded, sizeof(s)) == 0);
    for (i = CY_COMPONENT_COUNT; i > 0; --i) {
        assert(cyborg_core_uninstall(&s, (int)i - 1) == CY_OK);
        assert(cyborg_core_valid(&s));
        before = s;
        assert(cyborg_core_uninstall(&s, (int)i - 1) == CY_NOT_INSTALLED);
        assert(memcmp(&s, &before, sizeof(s)) == 0);
    }
    assert(s.installed == 0 && s.humanity == total_humanity && primal == 0);
    puts("Dependency-safe uninstall, exact humanity restoration and no duplicate refunds passed");

    assert(cyborg_core_init(&s, CY_COMPONENT_MASK, LONG_MAX) == CY_OK);
    assert(cyborg_core_encode(&s, record, sizeof(record)) == CY_OK);
    assert(cyborg_core_decode(&decoded, record) == CY_OK);
    assert(memcmp(&s, &decoded, sizeof(s)) == 0);
    strcpy(record, "unchanged");
    assert(cyborg_core_encode(&s, record, 1) == CY_INVALID);
    assert(strcmp(record, "unchanged") == 0);
    assert(cyborg_core_encode(&s, NULL, 0) == CY_INVALID);
    assert(cyborg_core_decode(NULL, "") == CY_INVALID);
    failed_decode(&s, NULL);
    failed_decode(&s, "");
    failed_decode(&s, "CyborgCore 2 0 0 100 100");
    failed_decode(&s, "CyborgCore 1 -1 0 100 100");
    failed_decode(&s, "CyborgCore 1 +0 0 100 100");
    failed_decode(&s, "CyborgCore 1 4294967296 0 100 100");
    failed_decode(&s, "CyborgCore 1 0 0 999999999999999999999999999 0");
    failed_decode(&s, "CyborgCore 1 0 0 100");
    failed_decode(&s, "CyborgCore 1 0 0 100 100 tail");
    failed_decode(&s, "CyborgCore 1 0 1 100 96");
    failed_decode(&s, "CyborgCore 1 32 0 100 100");
    failed_decode(&s, "CyborgCore 1 16777215 32 100 96");
    failed_decode(&s, "CyborgCore 1 16777215 1 100 100");
    failed_decode(&s, "CyborgCore 1 16777215 1 3 0");
    failed_decode(&s, "CyborgCore 1 0 0 100 101");
    memset(too_long, '0', sizeof(too_long) - 1);
    too_long[sizeof(too_long) - 1] = 0;
    failed_decode(&s, too_long);
    before = s;
    s.humanity--;
    assert(!cyborg_core_valid(&s));
    primal = LONG_MAX;
    failed_install(&s, CY_HEART, &primal, CY_INVALID);
    assert(cyborg_core_uninstall(&s, CY_HEART) == CY_INVALID);
    assert(cyborg_core_encode(&s, record, sizeof(record)) == CY_INVALID);
    s = before;
    puts("Versioned codec, LONG_MAX bounds, malformed/orphan state and unchanged-on-failure passed");

    fp = tmpfile();
    assert(fp != NULL);
    assert(cyborg_core_write_player(&s, fp) == CY_OK);
    fputs("Exp 456\n", fp);
    rewind(fp);
    assert(fscanf(fp, "%31s", word) == 1 && strcmp(word, "CyborgCore") == 0);
    assert(cyborg_core_read_player(&decoded, fp) == CY_OK);
    assert(memcmp(&s, &decoded, sizeof(s)) == 0);
    assert(fgets(following, sizeof(following), fp) != NULL);
    assert(strcmp(following, "Exp 456\n") == 0);
    fclose(fp);
    fp = tmpfile();
    assert(fp != NULL);
    for (i = 0; i < 1000; ++i) fputc('1', fp);
    fputs("\nExp 789\n", fp);
    rewind(fp);
    decoded = s;
    assert(cyborg_core_read_player(&decoded, fp) == CY_INVALID);
    assert(decoded.permitted == 0 && decoded.installed == 0);
    assert(decoded.humanity == 0 && decoded.humanity_limit == 0);
    assert(fgets(following, sizeof(following), fp) != NULL);
    assert(strcmp(following, "Exp 789\n") == 0);
    fclose(fp);
    fp = tmpfile();
    assert(fp != NULL);
    fputs("1 0 0 100 100", fp);
    fputc(0, fp);
    fputs("ignored\nExp 123\n", fp);
    rewind(fp);
    assert(cyborg_core_read_player(&decoded, fp) == CY_INVALID);
    assert(fgets(following, sizeof(following), fp) != NULL);
    assert(strcmp(following, "Exp 123\n") == 0);
    fclose(fp);
    fp = tmpfile();
    assert(fp != NULL);
    fputs("  1 0 0 100 100\r\n", fp);
    rewind(fp);
    assert(cyborg_core_read_player(&decoded, fp) == CY_OK);
    assert(decoded.humanity == 100);
    fclose(fp);
    assert(cyborg_core_write_player(&s, NULL) == CY_INVALID);
    assert(cyborg_core_read_player(&s, NULL) == CY_INVALID);
    puts("Player-record I/O, CRLF, oversized/binary rejection and following-field preservation passed");
    {
        CYBORG_RUNTIME runtime;
        size_t j;
        const char *bad[] = {
            "2 100 1 0", "1 10001 1 0", "1 100 13 0", "1 100 1 2",
            "1 -1 1 0", "1 9999999999999999999999999999999999 1 0",
            "1 100 1 0 junk", "1 100 1", "1 100 1  0"
        };
        memset(&runtime, 0, sizeof(runtime));
        assert(cyborg_runtime_valid(&runtime));
        runtime.nanites = 10000; runtime.rifle = 12; runtime.factory = 1;
        fp = tmpfile(); assert(fp != NULL);
        assert(cyborg_runtime_write_player(&runtime, fp) == CY_OK);
        rewind(fp);
        assert(fscanf(fp, "%31s", word) == 1);
        memset(&runtime, 0, sizeof(runtime));
        assert(cyborg_runtime_read_player(&runtime, fp) == CY_OK);
        assert(runtime.nanites == 10000 && runtime.rifle == 12 && runtime.factory == 1);
        fclose(fp);
        for (j = 0; j < sizeof(bad) / sizeof(bad[0]); ++j) {
            fp = tmpfile(); assert(fp != NULL);
            fprintf(fp, "%s\nExp 789\n", bad[j]); rewind(fp);
            assert(cyborg_runtime_read_player(&runtime, fp) == CY_INVALID);
            assert(runtime.nanites == 0 && runtime.rifle == 0 && runtime.factory == 0);
            assert(fgets(following, sizeof(following), fp) != NULL);
            assert(strcmp(following, "Exp 789\n") == 0);
            fclose(fp);
        }
        puts("Cyborg resource codec bounds, overflow/version rejection, reset and following-field alignment: passed");
    }
    {
        CYBORG_SYSTEMS systems, decoded_systems, empty;
        const char *bad[] = {
            "2 5 5 6 12 20 1 10000000", "1 6 5 6 12 20 1 10000000",
            "1 5 6 6 12 20 1 10000000", "1 5 5 7 12 20 1 10000000",
            "1 5 5 6 13 20 1 10000000", "1 5 5 6 12 21 1 10000000",
            "1 5 5 6 12 20 2 10000000", "1 5 5 6 12 20 1 10000001",
            "1 5 5 6 12 0 1 0", "1 5 5 3 0 0 0 0", "1 0 0 1 0 0 0 0",
            "1 -1 5 6 12 20 1 0", "1 5 5 6 12 20 1 0 junk",
            "1 5 5 6 12 20 1", "1 5 5 6 12 20  1 0",
            "1 5 5 6 12 20 1 9999999999999999999999999999999"
        };
        memset(&systems, 0, sizeof(systems)); empty = systems;
        assert(!cyborg_systems_valid(NULL) && cyborg_systems_valid(&systems));
        systems.face = 5; systems.limbs = 5; systems.body = 6;
        systems.demolition = 12; systems.bombs = 20; systems.armed = 1;
        systems.bionic = 10000000;
        fp = tmpfile(); assert(fp != NULL);
        assert(cyborg_systems_write_player(&systems, fp) == CY_OK);
        fputs("Exp 789\n", fp); rewind(fp);
        assert(fscanf(fp, "%31s", word) == 1 && strcmp(word, "CyborgSystems") == 0);
        assert(cyborg_systems_read_player(&decoded_systems, fp) == CY_OK);
        assert(memcmp(&systems, &decoded_systems, sizeof(systems)) == 0);
        assert(fgets(following, sizeof(following), fp) && !strcmp(following, "Exp 789\n"));
        fclose(fp);
        for (j = 0; j < sizeof(bad)/sizeof(bad[0]); ++j) {
            decoded_systems = systems;
            fp = tmpfile(); assert(fp != NULL);
            fprintf(fp, "%s\nExp 789\n", bad[j]); rewind(fp);
            assert(cyborg_systems_read_player(&decoded_systems, fp) == CY_INVALID);
            assert(memcmp(&decoded_systems, &empty, sizeof(empty)) == 0);
            assert(fgets(following, sizeof(following), fp) && !strcmp(following, "Exp 789\n"));
            fclose(fp);
        }
        fp = tmpfile(); assert(fp != NULL);
        fputs(" \t1 5 5 6 12 20 1 10000000\r\n", fp); rewind(fp);
        assert(cyborg_systems_read_player(&decoded_systems, fp) == CY_OK);
        fclose(fp);
        fp = tmpfile(); assert(fp != NULL);
        for (j = 0; j < 1000; ++j) fputc('1', fp);
        fputs("\nExp 789\n", fp); rewind(fp);
        assert(cyborg_systems_read_player(&decoded_systems, fp) == CY_INVALID);
        assert(fgets(following, sizeof(following), fp) && !strcmp(following, "Exp 789\n"));
        fclose(fp);
        fp = tmpfile(); assert(fp != NULL);
        fputs("1 5 5 6 12 20 1 0", fp); fputc(0, fp);
        fputs("\nExp 789\n", fp); rewind(fp);
        assert(cyborg_systems_read_player(&decoded_systems, fp) == CY_INVALID);
        assert(fgets(following, sizeof(following), fp) && !strcmp(following, "Exp 789\n"));
        fclose(fp);
        assert(cyborg_systems_read_player(NULL, NULL) == CY_INVALID);
        assert(cyborg_systems_write_player(NULL, NULL) == CY_INVALID);
        puts("CyborgSystems version, dependency graph, currency/ammunition bounds, CRLF, binary/oversize and next-field preservation: passed");
    }
    return 0;
}
