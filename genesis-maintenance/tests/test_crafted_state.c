#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/crafted_state.h"

static void bad(const char *payload)
{
    CRAFTED_STATE state = {GEN_PARENT_MOOGLE,8,9999};
    char next[40];
    FILE *fp=tmpfile();
    assert(fp);
    fputs(payload,fp); fputs("\nTail 97\n",fp); rewind(fp);
    assert(!crafted_state_read(&state,fp));
    assert(!state.parent && !state.rank && !state.energy);
    assert(fgets(next,sizeof(next),fp) && !strcmp(next,"Tail 97\n"));
    fclose(fp);
}

int main(void)
{
    CRAFTED_STATE state, other;
    FILE *fp;
    char key[40];
    int i;
    const char *invalid[] = {"","1","1 1018","1 1018 8","2 1018 8 500",
        "1 1013 8 500","1 0 1 0","1 1018 9 500","1 1018 -1 500","1 1018 8 10001",
        "1 1018 8 -1","1 1018 8 +1","1 1018 8 1 extra","1 1018 8 10000000000000000000000000000",
        "1 1018 8 1.2","1 1018 8  1","1 1018 8 1 ",NULL};
    for (i=0;invalid[i];++i) bad(invalid[i]);
    memset(&state,0,sizeof(state));
    assert(crafted_state_valid(&state));
    assert(!crafted_state_valid(NULL));
    assert(!crafted_state_read(NULL,NULL));
    assert(!crafted_state_write(&state,NULL));
    for (i=0;i<2;++i) {
        state.parent=i ? GEN_PARENT_MOOGLE : GEN_PARENT_DEATHKNIGHT;
        state.rank=8; state.energy=CRAFTED_ENERGY_CAP;
        fp=tmpfile(); assert(fp);
        assert(crafted_state_write(&state,fp)); fputs("Tail 97\n",fp); rewind(fp);
        assert(fscanf(fp,"%39s",key)==1 && !strcmp(key,"CraftedClass"));
        assert(crafted_state_read(&other,fp));
        assert(!memcmp(&state,&other,sizeof(state)));
        assert(fgets(key,sizeof(key),fp) && !strcmp(key,"Tail 97\n"));
        fclose(fp);
    }
    fp=tmpfile(); assert(fp);
    fputs("1 1018 8 ",fp); for(i=0;i<500;++i) fputc('0',fp);
    fputs("\nTail 97\n",fp); rewind(fp);
    assert(!crafted_state_read(&other,fp));
    assert(fgets(key,sizeof(key),fp) && !strcmp(key,"Tail 97\n")); fclose(fp);
    fp=tmpfile(); assert(fp);
    fputs("1 1018 8 1",fp); fputc(0,fp); fputs("2\nTail 97\n",fp); rewind(fp);
    assert(!crafted_state_read(&other,fp));
    assert(fgets(key,sizeof(key),fp) && !strcmp(key,"Tail 97\n")); fclose(fp);
    fp=tmpfile(); assert(fp);
    fputs(" 1 1017 0 0\r\nTail 97\n",fp); rewind(fp);
    assert(crafted_state_read(&other,fp));
    assert(other.parent==GEN_PARENT_DEATHKNIGHT && !other.rank && !other.energy); fclose(fp);
    puts("CraftedClass codec: versions, identity, bounds, malformed/overflow/binary/oversized input and next-field preservation passed");
    return 0;
}
