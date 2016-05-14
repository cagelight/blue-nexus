#include "bnex_http.h"

#include <string.h>

#include <assert.h>

//================================================================
//----------------------------------------------------------------
//================================================================

void bnex_http_request_create(bnex_http_request_t * req) {
	memset(req, 0, sizeof(bnex_http_request_t));
}

void bnex_http_request_destroy(bnex_http_request_t * req) {
	if (req->head_copy) free(req->head_copy);
	if (req->fields) free(req->fields);
}

http_request_parse_status_t bnex_http_request_parse(bnex_http_request_t * req, char * src_buf, size_t src_buf_size) {
	
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

char const * bnex_http_request_get_field(bnex_http_request_t * req, char const * field) {
	for (size_t i = 0; i < req->fields_len; i++) {
		if (!strcasecmp(field, req->fields[i].key)) return req->fields[i].value;
	}
	return NULL;
}

//================================================================
//----------------------------------------------------------------
//================================================================

void bnex_http_response_create(bnex_http_response_t * res) {
	memset(res, 0, sizeof(bnex_http_response_t));
}

void bnex_http_response_destroy(bnex_http_response_t * res) {
	if (res->fields) {
		for (size_t i = 0; i < res->fields_len; i++) {
			free(res->fields[i].key);
			free(res->fields[i].value);
		}
		free(res->fields);
	}
	if (res->data) free(res->data);
}

struct bnex_http_response_field_s * bnex_http_response_get_field(bnex_http_response_t * res, char const * field) {
	for (size_t i = 0; i < res->fields_len; i++) {
		if (!strcasecmp(field, res->fields[i].key)) return res->fields+i;
	}
	return NULL;
}

void bnex_http_response_set_add_field(bnex_http_response_t * res, char const * key, char const * value) {
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

void bnex_http_response_set_data(bnex_http_response_t * res, char const * MIME, char const * data, size_t data_size) {
	res->data_len = data_size;
	res->data = realloc(res->data, res->data_len);
	memcpy(res->data, data, data_size);
	bnex_http_response_set_add_field(res, "Content-Type", MIME ? MIME : "application/octet-stream");
	bnex_http_response_set_add_field(res, "Content-Length", vas("%zu", data_size));
}

void bnex_http_response_generate(bnex_http_response_t * res, bnex_http_request_t const * req) {
	res->code = 200;
	bnex_http_response_set_add_field(res, "Server", "Blue Nexus");
	
	static char const * no_handle = "no handle for the requested resource";
	bnex_http_response_set_data(res, "text/plain", no_handle, strlen(no_handle));
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
	default:
		return "Unknown";
	}
}
