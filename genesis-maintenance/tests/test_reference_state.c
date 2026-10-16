#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/reference_state.h"

static void rejected(const char *text)
{
    REFERENCE_LEGACY s;
    FILE *fp=tmpfile();
    assert(fp); memset(&s,0,sizeof(s)); s.souls=42;
    fputs(text,fp); fputs("\nNEXT\n",fp); rewind(fp);
    assert(!reference_legacy_read(&s,fp));
    assert(s.souls==0 && s.gifts==0 && s.warps==0 && s.grants[0]==0);
    assert(fgetc(fp)=='N'); fclose(fp);
}
int main(void)
{
    REFERENCE_LEGACY s,t;
    FILE *fp;
    char large[1024];
    long flags,ticks;
    unsigned i;
    memset(&s,0,sizeof(s)); s.souls=REF_SOUL_CAP; s.gifts=31;
    s.warps=1023; s.willing=1; strcpy(s.patron,"Twelveletter");
    fp=tmpfile(); assert(fp && reference_legacy_write(&s,fp)); rewind(fp);
    assert(fscanf(fp,"%*s ")==0);
    memset(&t,0,sizeof(t)); assert(reference_legacy_read(&t,fp));
    assert(!memcmp(&s,&t,sizeof(s))); fclose(fp);
    for(i=0;i<30;++i) { s.warps=1UL<<i; assert(reference_legacy_valid(&s)); }
    s.warps=2047; assert(!reference_legacy_valid(&s));
    rejected("2 0 0 0 0 -"); rejected("1 1000001 0 0 0 -");
    rejected("1 -1 0 0 0 -"); rejected("1 0 32 0 0 -");
    rejected("1 0 0 1073741824 0 -"); rejected("1 0 0 2047 0 -");
    rejected("1 0 0 0 2 -"); rejected("1 0 0 0 0 Bad1");
    rejected("1 0 0 0 0 Thirteenletters"); rejected("1 0 0 0 0 - extra");
    rejected("1 99999999999999999999999999999999999999 0 0 0 -");
    memset(large,'9',sizeof(large)-1); large[sizeof(large)-1]=0; rejected(large);
    fp=tmpfile(); assert(fp);
    fwrite("1 0 0\0 0 0 -\nNEXT\n",1,18,fp); rewind(fp);
    assert(!reference_legacy_read(&t,fp) && fgetc(fp)=='N'); fclose(fp);
    fp=tmpfile(); assert(fp); fputs(" 1 1 0 0 0 -\r\nNEXT\n",fp); rewind(fp);
    assert(reference_legacy_read(&t,fp) && t.souls==1 && fgetc(fp)=='N'); fclose(fp);
    for(i=1;i<=15;++i) {
        fp=tmpfile(); assert(fp); fprintf(fp,"1 %u 12\nNEXT\n",i); rewind(fp);
        assert(reference_weapon_read(&flags,&ticks,fp) && flags==i && ticks==12);
        assert(fgetc(fp)=='N'); fclose(fp);
    }
    {
        const char *bad[]={"1 0 1","1 16 1","1 1 0","1 1 13","2 1 1",
            "1 -1 1","1 1 1 extra","1 99999999999999999999 1",NULL};
        for(i=0;bad[i];++i) {
            fp=tmpfile(); assert(fp); fprintf(fp,"%s\nNEXT\n",bad[i]); rewind(fp);
            flags=ticks=5; assert(!reference_weapon_read(&flags,&ticks,fp));
            assert(!flags && !ticks && fgetc(fp)=='N'); fclose(fp);
        }
    }
    puts("ParentLegacy/RefWeapon codecs: masks, warp count, patron, resources, versions, binary/oversize/overflow, CRLF and next-field preservation passed");
    return 0;
}
