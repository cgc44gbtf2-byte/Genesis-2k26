#ifndef GENESIS_NETWORK_H
#define GENESIS_NETWORK_H
#include <stddef.h>

/* One dual-stack listener, with IPv4 fallback only when IPv6 is unavailable. */
int genesis_listen_socket(int port);
/* Numeric identity is normalized; IPv4-mapped peers become dotted IPv4. */
int genesis_peer_names(int fd, char *numeric, size_t numeric_size,
                       char *hostname, size_t hostname_size);
#endif
