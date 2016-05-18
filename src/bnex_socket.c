#include "bnex_socket.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>

#include <errno.h>

void bnex_socket_create(bnex_socket_t * sock) {
	memset(sock, 0, sizeof(bnex_socket_t));
	sock->fd = -1;
}

bool bnex_socket_listener_open(bnex_socket_t * bsck, uint16_t port) {
	
	bool stat = false;
	
	bsck->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (bsck->fd < 0) {
		com_print_error("failed to acquire socket");
		goto res;
	}
	
	bsck->addr.sin_family = AF_INET;
	bsck->addr.sin_addr.s_addr = INADDR_ANY;
	bsck->addr.sin_port = htons(port);
	if (bind(bsck->fd, (struct sockaddr *) &bsck->addr, sizeof(bsck->addr)) < 0) {
		com_print_error("failed to bind socket");
		goto res;
	}
	
	if (listen(bsck->fd, SOMAXCONN < 0)) {
		com_print_error("failed to listen");
		goto res;
	}
	
	stat = true;
	res:
	return stat;
}

void bnex_socket_listener_accept(bnex_socket_t * listener, bnex_socket_t * client_out) {
	socklen_t slen = sizeof(client_out->addr);
	client_out->fd = accept4(listener->fd, (struct sockaddr *) &client_out->addr, &slen, SOCK_NONBLOCK);
}

ssize_t bnex_socket_read(bnex_socket_t * sock, void * buf, size_t buf_len) {
	ssize_t e = recv(sock->fd, buf, buf_len, 0);
	if (e < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
		else return -1;
	} else if (e == 0) {
		return -1;
	} else {
		return e;
	}
}

ssize_t bnex_socket_write(bnex_socket_t * sock, void * buf, size_t buf_len) {
	ssize_t e = send(sock->fd, buf, buf_len, 0);
	if (e < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
		else return -1;
	} else {
		return e;
	}
}

ssize_t bnex_socket_sendfile(bnex_socket_t * sock, int fd, off_t * offset, size_t count) {
	ssize_t e = sendfile(sock->fd, fd, offset, count);
	if (e < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
		else return -1;
	} else {
		return e;
	}
}

char const * bnex_socket_ip2str(bnex_socket_t * sock) {
	return inet_ntop(AF_INET, &sock->addr.sin_addr, vas_next(), VAS_BUFLEN);
}

void bnex_socket_close(bnex_socket_t * sock) {
	shutdown(sock->fd, SHUT_RDWR);
	close(sock->fd);
}
