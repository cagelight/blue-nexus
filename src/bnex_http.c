#include "bnex_http.h"

#include <string.h>
#include <assert.h>

#include "provider_filesystem.h"

//================================================================
//----------------------------------------------------------------
//================================================================

void bnex_http_request_create(bnex_http_request_t * restrict req) {
	memset(req, 0, sizeof(bnex_http_request_t));
}

void bnex_http_request_destroy(bnex_http_request_t * restrict req) {
	if (req->path_decoded) free(req->path_decoded);
	if (req->head_copy) free(req->head_copy);
	if (req->fields) free(req->fields);
}

http_request_parse_status_t bnex_http_request_parse(bnex_http_request_t * restrict req, char * src_buf, size_t src_buf_size) {
	
	if (!req->head_delimiter) {
		req->head_delimiter = memmem(src_buf, src_buf_size, "\r\n\r\n", 4);
		if (!req->head_delimiter) return HTTP_REQUEST_INCOMPLETE;
		else {
			
			req->head_copy = malloc((req->head_delimiter - src_buf) + 5);
			req->head_copy[(req->head_delimiter - src_buf) + 4] = '\0';
			memcpy(req->head_copy, src_buf, (req->head_delimiter - src_buf) + 4);
			
			size_t head_copy_size = strlen(req->head_copy);
			
			req->mode = req->head_copy;
			while (req->head_copy[req->buf_i] != ' ' && req->buf_i < head_copy_size) req->buf_i++;
			if (req->buf_i == head_copy_size) return HTTP_REQUEST_MALFORMED;
			req->head_copy[req->buf_i++] = '\0';
			
			req->path = req->head_copy+req->buf_i;
			while (req->head_copy[req->buf_i] != ' ' && req->buf_i < head_copy_size) req->buf_i++;
			if (req->buf_i == head_copy_size) return HTTP_REQUEST_MALFORMED;
			req->head_copy[req->buf_i++] = '\0';
			
			char const * temp_path = http_path_decode_and_validate(req->path);
			if (!temp_path) return HTTP_REQUEST_BAD_PATH;
			req->path_decoded = malloc(strlen(temp_path) + 1);
			strcpy(req->path_decoded, temp_path);
			req->path = req->path_decoded;
				
			do {
				
				while (req->head_copy[req->buf_i] != '\r' && req->buf_i < head_copy_size) req->buf_i++;
				if (req->buf_i == head_copy_size) return HTTP_REQUEST_MALFORMED;
				if (src_buf + req->buf_i == req->head_delimiter) {
					req->head_copy[req->buf_i] = '\0';
					req->buf_i += 4;
					break;
				}
				req->head_copy[req->buf_i++] = '\0';
				req->head_copy[req->buf_i++] = '\0';
				
				req->fields_len++;
				req->fields = realloc(req->fields, sizeof(struct bnex_http_request_field_s) * req->fields_len);
				req->fields[req->fields_len-1].key = req->head_copy + req->buf_i;
				while (req->head_copy[req->buf_i] != ':' && req->buf_i < head_copy_size) req->buf_i++;
				if (req->buf_i == head_copy_size) return HTTP_REQUEST_MALFORMED;
				req->head_copy[req->buf_i++] = '\0';
				req->head_copy[req->buf_i++] = '\0';
				req->fields[req->fields_len-1].value = req->head_copy + req->buf_i;
			
			} while(true);
		}
	}
	
	assert(bnex_http_request_get_field(req, "Content-Length") == NULL); // TODO -- Handle Request Body
	
	return HTTP_REQUEST_COMPLETE;
}

char const * bnex_http_request_get_field(bnex_http_request_t * restrict req, char const * field) {
	for (size_t i = 0; i < req->fields_len; i++) {
		if (!strcasecmp(field, req->fields[i].key)) return req->fields[i].value;
	}
	return NULL;
}

//================================================================
//----------------------------------------------------------------
//================================================================

void bnex_http_response_create(bnex_http_response_t * restrict res) {
	memset(res, 0, sizeof(bnex_http_response_t));
	res->data_fd = -1;
}

void bnex_http_response_destroy(bnex_http_response_t * restrict res) {
	if (res->fields) {
		for (size_t i = 0; i < res->fields_len; i++) {
			free(res->fields[i].key);
			free(res->fields[i].value);
		}
		free(res->fields);
	}
	if (res->data) free(res->data);
	if (res->data_fd) close(res->data_fd);
}

struct bnex_http_response_field_s * bnex_http_response_get_field(bnex_http_response_t * restrict res, char const * field) {
	for (size_t i = 0; i < res->fields_len; i++) {
		if (!strcasecmp(field, res->fields[i].key)) return res->fields+i;
	}
	return NULL;
}

void bnex_http_response_set_add_field(bnex_http_response_t * restrict res, char const * key, char const * value) {
	struct bnex_http_response_field_s * resfield = bnex_http_response_get_field(res, key);
	if (resfield) {
		resfield->value = realloc(resfield->value, strlen(value));
		strcpy(resfield->value, value);
	} else {
		res->fields_len++;
		res->fields = realloc(res->fields, res->fields_len * sizeof(struct bnex_http_response_field_s));
		res->fields[res->fields_len-1].key = malloc(strlen(key) + 1);
		strcpy(res->fields[res->fields_len-1].key, key);
		res->fields[res->fields_len-1].value = malloc(strlen(value) + 1);
		strcpy(res->fields[res->fields_len-1].value, value);
	}
}

void bnex_http_response_set_data_buffer(bnex_http_response_t * restrict res, char const * MIME, char const * data, size_t data_size) {
	res->wmode = BNEX_HTTP_RESPONSE_WRITEMODE_BUFFER;
	res->data_len = data_size;
	res->data = realloc(res->data, res->data_len);
	memcpy(res->data, data, data_size);
	bnex_http_response_set_add_field(res, "Content-Type", MIME ? MIME : "application/octet-stream");
}

void bnex_http_response_set_data_file(bnex_http_response_t * restrict res, char const * MIME, int fd, size_t data_size) {
	res->wmode = BNEX_HTTP_RESPONSE_WRITEMODE_FILE;
	res->data_len = data_size;
	res->data_fd = fd;
	bnex_http_response_set_add_field(res, "Content-Type", MIME ? MIME : "application/octet-stream");
}

static char const * no_handle = "no handle for the requested resource";

void bnex_http_response_generate(bnex_http_response_t * restrict res, bnex_http_request_t const * restrict req) {
	
	res->code = 500;
	
	if (provider_fs_handle(req, res)) goto end;
	
	res->code = 404;
	bnex_http_response_set_data_buffer(res, "text/plain", no_handle, strlen(no_handle));
	
	end:
	
	bnex_http_response_set_add_field(res, "Server", "Blue Nexus");
	bnex_http_response_set_add_field(res, "Content-Length", vas("%zu", res->data_len));
}

//================================================================
//----------------------------------------------------------------
//================================================================

char const * http_text_for_code(uint_fast16_t code) {
	switch(code) {
	case 100:
		return "Continue";	
	case 200:
		return "OK";
	case 404:
		return "Not Found";
	case 500:
		return "Internal Server Error";
	default:
		return "Unknown";
	}
}

static inline char from_meta(char meta [2]) {
	switch (meta[0]) {
	case '2':
		switch(meta[1]) {
		case '0':
			return ' ';
		}
	}
	return '\0';
}

char const * http_path_decode_and_validate(char const * enc) {
	
	uint_fast16_t i_f = 0, i_t = 0;
	uint_fast16_t e_f = strlen(enc), e_t = VAS_BUFLEN;
	
	char * buf = vas_next();
	
	uint_fast8_t meta = 0;
	char meta_c [2];
	
	while(i_f != e_f && i_t != e_t) {
		char c = enc[i_f++];
		if (meta) {
			if (c < '0' || c > '9') return NULL;
			meta_c[meta++ - 1] = c;
			if (meta == 3) {
				meta = 0;
				c = from_meta(meta_c);
				if (c) goto metaparse; else return NULL;
			} else {
				continue;
			}
		} else if (c == '%') {
			meta = 1;
			continue;
		} else metaparse: if (c == '/' || c == '\\') {
			if (buf[i_t-1] == '/') continue;
			else buf[i_t++] = '/';
		} else {
			buf[i_t++] = c;
			continue;
		}
	}
	
	if (buf[i_t-1] == '/') 
		buf[i_t-1] = '\0';
	else
		buf[i_t] = '\0';
	
	return buf;
}

char const * http_path_encode(char const * txt) {
	return txt;
}
