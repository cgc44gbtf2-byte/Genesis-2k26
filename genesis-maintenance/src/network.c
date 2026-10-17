/* Genesis Linux/POSIX networking; retain one listener for select/copyover. */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "network.h"

static int listener(int family, int port)
{
    struct sockaddr_in v4;
    struct sockaddr_in6 v6;
    int fd,one=1,zero=0,saved;
    fd=socket(family,SOCK_STREAM,0);
    if(fd<0) return -1;
    if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one))<0) goto failed;
    if(family==AF_INET6) {
        if(setsockopt(fd,IPPROTO_IPV6,IPV6_V6ONLY,&zero,sizeof(zero))<0) goto failed;
        memset(&v6,0,sizeof(v6));
        v6.sin6_family=AF_INET6; v6.sin6_port=htons((unsigned short)port);
        v6.sin6_addr=in6addr_any;
        if(bind(fd,(struct sockaddr *)&v6,sizeof(v6))<0) goto failed;
    } else {
        memset(&v4,0,sizeof(v4));
        v4.sin_family=AF_INET; v4.sin_port=htons((unsigned short)port);
        v4.sin_addr.s_addr=htonl(INADDR_ANY);
        if(bind(fd,(struct sockaddr *)&v4,sizeof(v4))<0) goto failed;
    }
    if(listen(fd,32)<0) goto failed;
    return fd;
failed:
    saved=errno; close(fd); errno=saved; return -1;
}

int genesis_listen_socket(int port)
{
    int fd,saved;
    if(port<0 || port>65535) { errno=EINVAL; return -1; }
    fd=listener(AF_INET6,port);
    if(fd>=0) return fd;
    saved=errno;
    /* Never hide permission errors, exhaustion or an occupied port. */
    if(saved!=EAFNOSUPPORT && saved!=EPROTONOSUPPORT && saved!=ENOPROTOOPT
        && saved!=EADDRNOTAVAIL) return -1;
    return listener(AF_INET,port);
}

int genesis_peer_names(int fd, char *numeric, size_t numeric_size,
                       char *hostname, size_t hostname_size)
{
    struct sockaddr_storage storage;
    struct sockaddr_in v4;
    const struct sockaddr_in6 *v6;
    const struct sockaddr *peer;
    socklen_t length=sizeof(storage);
    if(!numeric || !hostname || !numeric_size || !hostname_size) {
        errno=EINVAL; return -1;
    }
    snprintf(numeric,numeric_size,"(unknown)");
    snprintf(hostname,hostname_size,"(unknown)");
    memset(&storage,0,sizeof(storage));
    if(getpeername(fd,(struct sockaddr *)&storage,&length)<0) return -1;
    peer=(const struct sockaddr *)&storage;
    if(storage.ss_family==AF_INET6) {
        v6=(const struct sockaddr_in6 *)&storage;
        if(IN6_IS_ADDR_V4MAPPED(&v6->sin6_addr)) {
            memset(&v4,0,sizeof(v4));
            v4.sin_family=AF_INET; v4.sin_port=v6->sin6_port;
            memcpy(&v4.sin_addr,&v6->sin6_addr.s6_addr[12],sizeof(v4.sin_addr));
            peer=(const struct sockaddr *)&v4; length=sizeof(v4);
        }
    }
    if(getnameinfo(peer,length,numeric,(socklen_t)numeric_size,NULL,0,NI_NUMERICHOST)) {
        errno=EIO; return -1;
    }
    /* Preserve legacy hostname bans, and also check numeric bans in comm.c. */
    if(getnameinfo(peer,length,hostname,(socklen_t)hostname_size,NULL,0,NI_NAMEREQD))
        snprintf(hostname,hostname_size,"%s",numeric);
    return 0;
}
