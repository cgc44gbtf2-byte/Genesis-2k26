#include <stdio.h>
#include <string.h>
#include "lineage_state.h"
#include "reference_state.h"

int lineage_track_count(long id)
{
    return id==GEN_PARENT_GARGOYLE ? 1 : id==GEN_PARENT_KUEIJIN ? 18 : id==GEN_PARENT_WRAITH ? 11 : 0;
}
int lineage_state_valid(const LINEAGE_STATE *s)
{
    int i,n;
    if(!s || s->energy<0 || s->energy>LINEAGE_ENERGY_CAP || s->dharma<0 || s->dharma>5) return 0;
    n=lineage_track_count(s->parent);
    if(!n && (s->parent || s->energy || s->dharma)) return 0;
    if(s->parent!=GEN_PARENT_KUEIJIN && s->dharma) return 0;
    for(i=0;i<LINEAGE_TRACKS;++i)
        if(s->tracks[i]<0 || s->tracks[i]>5 || (i>=n && s->tracks[i])
            || (s->parent==GEN_PARENT_KUEIJIN && s->tracks[i]>s->dharma+2)) return 0;
    return 1;
}
int lineage_state_write(const LINEAGE_STATE *s,FILE *fp)
{
    int i;
    if(!fp || !lineage_state_valid(s)) return 0;
    if(fprintf(fp,"LineageClass 1 %ld %ld %ld",s->parent,s->energy,s->dharma)<0) return 0;
    for(i=0;i<LINEAGE_TRACKS;++i) if(fprintf(fp," %ld",s->tracks[i])<0) return 0;
    return fputc('\n',fp)!=EOF;
}
int lineage_state_read(LINEAGE_STATE *s,FILE *fp)
{
    char line[384],*p;
    long v[LINEAGE_TRACKS+4];
    LINEAGE_STATE next;
    int i;
    if(!s || !fp) return 0;
    if(!reference_line(fp,line,sizeof(line))) goto bad;
    p=line;
    for(i=0;i<LINEAGE_TRACKS+4;++i) {
        v[i]=0;
        if(*p<'0' || *p>'9') goto bad;
        while(*p>='0' && *p<='9') {
            if(v[i]>LINEAGE_ENERGY_CAP/10) goto bad;
            v[i]=v[i]*10+(*p++-'0');
            if(v[i]>LINEAGE_ENERGY_CAP) goto bad;
        }
        if(i<LINEAGE_TRACKS+3 && *p++!=' ') goto bad;
    }
    if(*p || v[0]!=1) goto bad;
    memset(&next,0,sizeof(next));
    next.parent=v[1]; next.energy=v[2]; next.dharma=v[3];
    for(i=0;i<LINEAGE_TRACKS;++i) next.tracks[i]=v[i+4];
    if(!lineage_state_valid(&next)) goto bad;
    *s=next; return 1;
bad:
    memset(s,0,sizeof(*s)); return 0;
}
