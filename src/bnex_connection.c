#include "bnex_connection.h"

#include <string.h>
#include <unistd.h>
#include <assert.h>

static inline void bnex_connection_reset_for_read(bnex_connection_t * con) {
	con->status = BNEX_CONNECTION_READING;
	bnex_http_request_destroy(&con->req);
	bnex_http_request_create(&con->req);
	bnex_http_response_destroy(&con->res);
	bnex_http_response_create(&con->res);
	con->buf_i = 0;
	con->buf_off = 0;
	con->buf = realloc(con->buf, 128);
	con->buf_len = 128;
}

static inline bnex_connection_cycle_result_t bnex_connection_cycle_handle_writing(bnex_connection_t * con) {
	
	ssize_t rv;
	if (con->buf_i < con->buf_len) {
		rv = bnex_socket_write(&con->sock, con->buf + con->buf_i, con->buf_len - con->buf_i);
		if (rv > 0) {
			con->buf_i += rv;
			return BNEX_CONNECTION_CYCLE_WROTE_SOME;
		} else if (rv == 0) {
			return BNEX_CONNECTION_CYCLE_IDLE;
		} else {
			return BNEX_CONNECTION_CYCLE_TERMINATE;
		}
	} else if (con->buf_i + con->buf_off < con->buf_len + con->res.data_len) {
		switch (con->res.wmode) {
		case BNEX_HTTP_RESPONSE_WRITEMODE_BUFFER:
			rv = bnex_socket_write(&con->sock, con->res.data + (con->buf_i - con->buf_len), con->res.data_len - (con->buf_i - con->buf_len));
			if (rv > 0) {
				con->buf_i += rv;
				return BNEX_CONNECTION_CYCLE_WROTE_SOME;
			} else if (rv == 0) {
				return BNEX_CONNECTION_CYCLE_IDLE;
			} else {
				return BNEX_CONNECTION_CYCLE_TERMINATE;
			}
		case BNEX_HTTP_RESPONSE_WRITEMODE_FILE:
			rv = bnex_socket_sendfile(&con->sock, con->res.data_fd, &con->buf_off, con->res.data_len);
			if (rv > 0) {
				if (con->buf_off == (off_t)con->res.data_len) {
					bnex_connection_reset_for_read(con);
				} 
				return BNEX_CONNECTION_CYCLE_WROTE_SOME;
			} else if (rv < 0) {
				return BNEX_CONNECTION_CYCLE_TERMINATE;
			} else {
				return BNEX_CONNECTION_CYCLE_IDLE;
			}
		default:
			return BNEX_CONNECTION_CYCLE_TERMINATE;
		}
	} else {
		bnex_connection_reset_for_read(con);
		return BNEX_CONNECTION_CYCLE_IDLE;
	}
}

static inline bnex_connection_cycle_result_t bnex_connection_cycle_handle_reading(bnex_connection_t * con) {
	ssize_t rv = bnex_socket_read(&con->sock, con->buf + con->buf_i, con->buf_len - con->buf_i);
	if (rv < 0) {
		return BNEX_CONNECTION_CYCLE_TERMINATE;
	} else if (rv == 0) {
		return BNEX_CONNECTION_CYCLE_IDLE;
	} else {
		con->buf_i += rv;
		switch (bnex_http_request_parse(&con->req, con->buf, con->buf_i)) {
		case HTTP_REQUEST_INCOMPLETE:
			if (con->buf_i == con->buf_len) {
				con->buf_len *= 1.5;
				con->buf = realloc(con->buf, con->buf_len);
			}
			return BNEX_CONNECTION_CYCLE_READ_SOME;
		case HTTP_REQUEST_MALFORMED:
			com_printf_error("Connection from \"%s\" sent malformed request", bnex_socket_ip2str(&con->sock));
			return BNEX_CONNECTION_CYCLE_TERMINATE;
		case HTTP_REQUEST_BAD_PATH:
			com_printf_error("Connection from \"%s\" requested invalid path", bnex_socket_ip2str(&con->sock));
			return BNEX_CONNECTION_CYCLE_TERMINATE;
		case HTTP_REQUEST_COMPLETE: 
			bnex_connection_compile_response(con);
			con->buf_i = 0;
			con->status = BNEX_CONNECTION_WRITING;
			return BNEX_CONNECTION_CYCLE_READ_SOME;
		default:
			return BNEX_CONNECTION_CYCLE_TERMINATE;
		}
	}
}

//================================================================
//----------------------------------------------------------------
//================================================================

void bnex_connection_create(bnex_connection_t * con) {
	memset(con, 0, sizeof(bnex_connection_t));
	bnex_socket_create(&con->sock);
	con->status = BNEX_CONNECTION_INACTIVE;
}

void bnex_connection_destroy(bnex_connection_t * con) {
	bnex_connection_close(con);
	if (con->buf) free(con->buf);
}

bool bnex_connection_establish(bnex_connection_t * con, bnex_socket_t * listener) {
	bnex_socket_listener_accept(listener, &con->sock);
	if (con->sock.fd >= 0) {
		bnex_connection_reset_for_read(con);
		return true;
	}
	return false;
}

void bnex_connection_close(bnex_connection_t * con) {
	bnex_http_response_destroy(&con->res);
	bnex_http_request_destroy(&con->req);
	bnex_socket_close(&con->sock);
	con->status = BNEX_CONNECTION_INACTIVE;
}

bnex_connection_cycle_result_t bnex_connection_cycle(bnex_connection_t * con) {
	bnex_connection_cycle_result_t res;
	switch (con->status) {
	case BNEX_CONNECTION_INACTIVE:
		return BNEX_CONNECTION_CYCLE_TERMINATE;
	case BNEX_CONNECTION_READING:
		res = bnex_connection_cycle_handle_reading(con);
		return res;
	case BNEX_CONNECTION_WRITING:
		res = bnex_connection_cycle_handle_writing(con);
		return res;
	default:
		return BNEX_CONNECTION_CYCLE_TERMINATE;
	}
}

void bnex_connection_compile_response(bnex_connection_t * con) {

	bnex_http_response_generate(&con->res, &con->req);
	
	size_t size = 14; // Constants: "HTTP/1.1" (8), two spaces (2), two CRLF's (4)
	
	char * code_str = vas("%hu", con->res.code);
	char const * code_text = http_text_for_code(con->res.code);
	
	size_t code_str_len = strlen(code_str);
	size_t code_text_len = strlen(code_text);
	
	size += strlen(code_str);
	size += strlen(code_text);
	
	for (size_t i = 0; i < con->res.fields_len; i++) {
		size += strlen(con->res.fields[i].key);
		size += strlen(con->res.fields[i].value);
		size += 4; //Constants: ": " (2), CRLF (2)
	}
	
	con->buf = realloc(con->buf, size);
	con->buf_len = size;
	
	memcpy(con->buf, "HTTP/1.1", 8);
	con->buf[8] = ' ';
	size_t i = 9;
	
	memcpy(con->buf + i, code_str, code_str_len);
	i += code_str_len;
	
	con->buf[i++] = ' ';
	
	memcpy(con->buf + i, code_text, code_text_len);
	i += code_text_len;
	
	con->buf[i++] = '\r';
	con->buf[i++] = '\n';
	
	for (size_t j = 0; j < con->res.fields_len; j++) {
		size_t key_len = strlen(con->res.fields[j].key);
		size_t value_len = strlen(con->res.fields[j].value);
		memcpy(con->buf + i, con->res.fields[j].key, key_len);
		i += key_len;
		con->buf[i++] = ':';
		con->buf[i++] = ' ';
		memcpy(con->buf + i, con->res.fields[j].value, value_len);
		i += value_len;
		con->buf[i++] = '\r';
		con->buf[i++] = '\n';
	}
	
	con->buf[i++] = '\r';
	con->buf[i++] = '\n';
	
}
