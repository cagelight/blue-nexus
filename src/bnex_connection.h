#ifndef BNEX_CONNECTION_H
#define BNEX_CONNECTION_H

#include "bnex_socket.h"
#include "bnex_http.h"

typedef enum bnex_connection_status_e {
	BNEX_CONNECTION_INACTIVE,
	BNEX_CONNECTION_READING,
	BNEX_CONNECTION_WRITING,
} bnex_connection_status_t;

typedef struct bnex_connection_s {
	
	bnex_socket_t sock;
	bnex_connection_status_t status;
	char * buf;
	size_t buf_len;
	size_t buf_i;
	
	bnex_http_request_t req;
	bnex_http_response_t res;
	
} bnex_connection_t;

typedef enum bnex_connection_cycle_result_e {
	BNEX_CONNECTION_CYCLE_TERMINATE = 0,
	BNEX_CONNECTION_CYCLE_IDLE,
	BNEX_CONNECTION_CYCLE_READ_SOME,
	BNEX_CONNECTION_CYCLE_WROTE_SOME,
} bnex_connection_cycle_result_t;

void bnex_connection_create(bnex_connection_t *);
void bnex_connection_destroy(bnex_connection_t *);

bool bnex_connection_establish(bnex_connection_t *, bnex_socket_t * listener);
void bnex_connection_close(bnex_connection_t *);

bnex_connection_cycle_result_t bnex_connection_cycle(bnex_connection_t *);
void bnex_connection_compile_response(bnex_connection_t *);

#endif//BNEX_CONNECTION_H
