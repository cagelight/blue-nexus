#ifndef BNEX_SOCKET_H
#define BNEX_SOCKET_H

#include "com.h"

#include <netinet/in.h>

typedef struct bnex_socket_s {
	int fd;
	struct sockaddr_in addr;
} bnex_socket_t;

void bnex_socket_create(bnex_socket_t *);

bool bnex_socket_listener_open(bnex_socket_t * listener_out, uint16_t port);
void bnex_socket_listener_accept(bnex_socket_t * listener, bnex_socket_t * client_out);

ssize_t bnex_socket_read(bnex_socket_t *, void * buf, size_t buf_len);
ssize_t bnex_socket_write(bnex_socket_t *, void * buf, size_t buf_len);

char const * bnex_socket_ip2str(bnex_socket_t *);

void bnex_socket_close(bnex_socket_t * sock);

#endif//BNEX_SOCKET_H
