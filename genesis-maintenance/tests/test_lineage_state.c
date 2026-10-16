#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/lineage_state.h"
static void invalid(long parent,long energy,long dharma,int slot,long value)
{
    LINEAGE_STATE out;
    char tail[40];
    FILE *fp=tmpfile();
    int i;
    assert(fp);
    fprintf(fp,"1 %ld %ld %ld",parent,energy,dharma);
    for(i=0;i<LINEAGE_TRACKS;++i) fprintf(fp," %ld",i==slot ? value : 0L);
    fputs("\nTail 99\n",fp); rewind(fp);
    memset(&out,1,sizeof(out)); assert(!lineage_state_read(&out,fp));
    assert(!out.parent && !out.energy && !out.dharma);
    for(i=0;i<LINEAGE_TRACKS;++i) assert(!out.tracks[i]);
    assert(fgets(tail,sizeof(tail),fp) && !strcmp(tail,"Tail 99\n")); fclose(fp);
}
int main(void)
{
    LINEAGE_STATE s,out;
    int ids[]={GEN_PARENT_GARGOYLE,GEN_PARENT_KUEIJIN,GEN_PARENT_WRAITH};
    int i,j,k;
    FILE *fp;
    char key[40];
    for(i=0;i<3;++i) {
        memset(&s,0,sizeof(s)); s.parent=ids[i]; s.energy=10000;
        s.dharma=i==1 ? 5 : 0;
        for(j=0;j<lineage_track_count(s.parent);++j) s.tracks[j]=5;
        fp=tmpfile(); assert(fp);
        assert(lineage_state_write(&s,fp)); fputs("Tail 99\n",fp); rewind(fp);
        assert(fscanf(fp,"%39s",key)==1 && !strcmp(key,"LineageClass"));
        assert(lineage_state_read(&out,fp) && !memcmp(&s,&out,sizeof(s)));
        assert(fgets(key,sizeof(key),fp) && !strcmp(key,"Tail 99\n")); fclose(fp);
        for(j=0;j<LINEAGE_TRACKS;++j) {
            invalid(ids[i],0,0,j,-1);
            invalid(ids[i],0,0,j,j>=lineage_track_count(ids[i]) ? 1 : 6);
        }
    }
    invalid(0,1,0,-1,0); invalid(GEN_PARENT_MOOGLE,0,0,-1,0);
    invalid(GEN_PARENT_GARGOYLE,10001,0,-1,0); invalid(GEN_PARENT_WRAITH,0,1,-1,0);
    invalid(GEN_PARENT_KUEIJIN,0,6,-1,0); invalid(GEN_PARENT_KUEIJIN,0,-1,-1,0);
    invalid(GEN_PARENT_KUEIJIN,0,0,0,3);
    for(k=0;k<4;++k) {
        fp=tmpfile(); assert(fp);
        if(k==0) fputs("2 1015 0 0",fp);
        if(k==1) { fputs("1 1015 0 0 ",fp); for(j=0;j<500;++j) fputc('9',fp); }
        if(k==2) { fputs("1 1015 0 0 ",fp); fputc(0,fp); }
        if(k==3) fputs("1 1015 0 0 +1",fp);
        fputs("\nTail 99\n",fp); rewind(fp); assert(!lineage_state_read(&out,fp));
        assert(fgets(key,sizeof(key),fp) && !strcmp(key,"Tail 99\n")); fclose(fp);
    }
    puts("LineageClass codec: all three independent schemas, every track bound/reserved slot, wrong owner, overflow/version/binary/oversize and following fields passed");
    return 0;
}
