#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../src/merc.h"
#include "../src/network.h"
extern void init_mm(void);
extern void new_descriptor(int fd);
extern char *help_greeting;

/* Exercise the IPv6 syscall path even on IPv4-only CI kernels. */
static int sim_mode,sim_v4,sim_v6,sim_reuse,sim_dual,sim_closed;
static struct sockaddr_in6 sim_peer;
static int mock_socket(int family,int type,int protocol)
{
    assert(type==SOCK_STREAM && !protocol);
    if(family==AF_INET6) {
        ++sim_v6;
        if(sim_mode==1) { errno=EAFNOSUPPORT; return -1; }
        if(sim_mode==4) { errno=EACCES; return -1; }
    } else { assert(family==AF_INET); ++sim_v4; }
    return 100+family;
}
static int mock_setsockopt(int fd,int level,int name,const void *value,socklen_t size)
{
    assert(fd==100+AF_INET || fd==100+AF_INET6);
    assert(size==sizeof(int));
    if(level==SOL_SOCKET && name==SO_REUSEADDR) {
        assert(*(const int *)value==1); ++sim_reuse;
    } else {
        assert(level==IPPROTO_IPV6 && name==IPV6_V6ONLY && !*(const int *)value);
        ++sim_dual;
        if(sim_mode==3) { errno=ENOPROTOOPT; return -1; }
    }
    return 0;
}
static int mock_bind(int fd,const struct sockaddr *address,socklen_t size)
{
    assert(fd==100+address->sa_family);
    if(address->sa_family==AF_INET6) {
        const struct sockaddr_in6 *v6=(const struct sockaddr_in6 *)address;
        assert(size==sizeof(*v6) && ntohs(v6->sin6_port)==45678);
        assert(IN6_IS_ADDR_UNSPECIFIED(&v6->sin6_addr));
        if(sim_mode==2) { errno=EADDRINUSE; return -1; }
    } else {
        const struct sockaddr_in *v4=(const struct sockaddr_in *)address;
        assert(size==sizeof(*v4) && ntohs(v4->sin_port)==45678);
        assert(v4->sin_addr.s_addr==htonl(INADDR_ANY));
    }
    return 0;
}
static int mock_listen(int fd,int backlog)
{
    assert((fd==100+AF_INET || fd==100+AF_INET6) && backlog==32); return 0;
}
static int mock_close(int fd) { assert(fd>=100); ++sim_closed; return 0; }
static int mock_getpeername(int fd,struct sockaddr *address,socklen_t *length)
{
    (void)fd; assert(*length>=sizeof(sim_peer));
    memcpy(address,&sim_peer,sizeof(sim_peer)); *length=sizeof(sim_peer); return 0;
}
static int mock_getnameinfo(const struct sockaddr *address,socklen_t length,
    char *host,socklen_t host_size,char *service,socklen_t service_size,int flags)
{
    if(flags==NI_NAMEREQD) return EAI_NONAME;
    return getnameinfo(address,length,host,host_size,service,service_size,flags);
}
#define socket mock_socket
#define setsockopt mock_setsockopt
#define bind mock_bind
#define listen mock_listen
#define close mock_close
#define getpeername mock_getpeername
#define getnameinfo mock_getnameinfo
#define genesis_listen_socket simulated_listener
#define genesis_peer_names simulated_peer_names
#include "../src/network.c"
#undef socket
#undef setsockopt
#undef bind
#undef listen
#undef close
#undef getpeername
#undef getnameinfo
#undef genesis_listen_socket
#undef genesis_peer_names

static void simulated_ipv6(void)
{
    char numeric[128],host[256];
    for(sim_mode=0;sim_mode<=4;++sim_mode) {
        int fd;
        sim_v4=sim_v6=sim_reuse=sim_dual=sim_closed=0;
        fd=simulated_listener(45678);
        assert(sim_v6==1);
        if(sim_mode==0) assert(fd==100+AF_INET6 && sim_dual==1 && sim_reuse==1 && !sim_v4);
        if(sim_mode==1) assert(fd==100+AF_INET && sim_v4==1 && sim_reuse==1);
        if(sim_mode==2) assert(fd==-1 && errno==EADDRINUSE && !sim_v4 && sim_closed==1);
        if(sim_mode==3) assert(fd==100+AF_INET && sim_v4==1 && sim_closed==1);
        if(sim_mode==4) assert(fd==-1 && errno==EACCES && !sim_v4 && !sim_closed);
    }
    memset(&sim_peer,0,sizeof(sim_peer)); sim_peer.sin6_family=AF_INET6;
    assert(inet_pton(AF_INET6,"2604:a880:400:d1:0:5:331d:4001",&sim_peer.sin6_addr)==1);
    assert(!simulated_peer_names(123,numeric,sizeof(numeric),host,sizeof(host)));
    assert(!strcmp(numeric,"2604:a880:400:d1:0:5:331d:4001") && !strcmp(host,numeric));
    assert(inet_pton(AF_INET6,"::ffff:127.0.0.1",&sim_peer.sin6_addr)==1);
    assert(!simulated_peer_names(123,numeric,sizeof(numeric),host,sizeof(host)));
    assert(!strcmp(numeric,"127.0.0.1") && !strcmp(host,numeric));
    puts("Simulated IPv6 syscalls: bind/options, unavailable-stack fallback, occupied/denied-port failures, IPv6 formatting and mapped-IPv4 normalization passed");
}

static int client(int family,int port)
{
    struct sockaddr_in v4;
    struct sockaddr_in6 v6;
    int fd=socket(family,SOCK_STREAM,0);
    assert(fd>=0);
    if(family==AF_INET) {
        memset(&v4,0,sizeof(v4)); v4.sin_family=AF_INET; v4.sin_port=htons(port);
        assert(inet_pton(AF_INET,"127.0.0.1",&v4.sin_addr)==1);
        assert(connect(fd,(struct sockaddr *)&v4,sizeof(v4))==0);
    } else {
        memset(&v6,0,sizeof(v6)); v6.sin6_family=AF_INET6; v6.sin6_port=htons(port);
        assert(inet_pton(AF_INET6,"::1",&v6.sin6_addr)==1);
        assert(connect(fd,(struct sockaddr *)&v6,sizeof(v6))==0);
    }
    return fd;
}
int main(void)
{
    struct sockaddr_storage address;
    socklen_t length=sizeof(address);
    char numeric[256],hostname[1025];
    int listener,port,c,a,i,family;
    BAN_DATA ban;
    init_mm(); help_greeting="Network regression greeting\n\r";
    simulated_ipv6();
    listener=genesis_listen_socket(0); assert(listener>=0);
    assert(!getsockname(listener,(struct sockaddr *)&address,&length));
    family=address.ss_family;
    port=family==AF_INET6 ? ntohs(((struct sockaddr_in6 *)&address)->sin6_port)
        : ntohs(((struct sockaddr_in *)&address)->sin_port);
    for(i=0;i<(family==AF_INET6 ? 2 : 1);++i) {
        c=client(i ? AF_INET6 : AF_INET,port);
        a=accept(listener,NULL,NULL); assert(a>=0);
        assert(!genesis_peer_names(a,numeric,sizeof(numeric),hostname,sizeof(hostname)));
        assert(!strcmp(numeric,i ? "::1" : "127.0.0.1") && hostname[0]);
        close(a); close(c);
        c=client(i ? AF_INET6 : AF_INET,port);
        new_descriptor(listener);
        assert(descriptor_list && descriptor_list->connected==CON_GET_NAME);
        assert(strstr(descriptor_list->outbuf,"Network regression greeting"));
        close_socket(descriptor_list); close(c); assert(!descriptor_list);
        memset(&ban,0,sizeof(ban)); ban.name=i ? "::1" : "127.0.0.1";
        ban_list=&ban;
        c=client(i ? AF_INET6 : AF_INET,port);
        new_descriptor(listener);
        assert(!descriptor_list);
        a=(int)read(c,numeric,sizeof(numeric)-1); assert(a>0); numeric[a]=0;
        assert(strstr(numeric,"banned"));
        close(c); ban_list=NULL;
        /* Keep reverse-hostname bans working alongside numeric bans. */
        ban.name=hostname; ban_list=&ban;
        c=client(i ? AF_INET6 : AF_INET,port);
        new_descriptor(listener); assert(!descriptor_list);
        a=(int)read(c,numeric,sizeof(numeric)-1); assert(a>0); numeric[a]=0;
        assert(strstr(numeric,"banned")); close(c); ban_list=NULL;
    }
    errno=0; assert(genesis_listen_socket(port)<0 && errno==EADDRINUSE);
    close(listener);
    assert(genesis_listen_socket(-1)<0 && errno==EINVAL);
    assert(genesis_listen_socket(65536)<0 && errno==EINVAL);
    assert(genesis_peer_names(-1,numeric,sizeof(numeric),hostname,sizeof(hostname))<0);
    assert(!strcmp(numeric,"(unknown)") && !strcmp(hostname,"(unknown)"));
    assert(genesis_peer_names(-1,NULL,0,hostname,sizeof(hostname))<0 && errno==EINVAL);
    printf("Network passed: %s; real descriptor greetings and numeric bans, canonical peers, occupied-port and error guards\n",
        family==AF_INET6 ? "IPv6 + IPv4 on one listener" : "IPv4 fallback (IPv6 unavailable here)");
    return 0;
}
