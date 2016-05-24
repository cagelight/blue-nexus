#ifndef BNEX_HTTP_H
#define BNEX_HTTP_H

#include "com.h"

#include <unistd.h>

//================================================================
//----------------------------------------------------------------
//================================================================


typedef struct bnex_http_request_s {
	
	char * head_copy; //all ptrs in this struct derive from here
	
	char const * head_delimiter;
	
	char const * mode;
	char const * path;
	
	char * path_decoded;
	
	struct bnex_http_request_field_s {
		char const * key;
		char const * value;
	} * fields;
	
	size_t fields_len;
	size_t buf_i;
	
} bnex_http_request_t;

void bnex_http_request_create(bnex_http_request_t * restrict);
void bnex_http_request_destroy(bnex_http_request_t * restrict);

typedef enum http_request_parse_status_e {
	HTTP_REQUEST_INCOMPLETE,
	HTTP_REQUEST_MALFORMED,
	HTTP_REQUEST_BAD_PATH,
	HTTP_REQUEST_COMPLETE
} http_request_parse_status_t;

http_request_parse_status_t bnex_http_request_parse(bnex_http_request_t * restrict, char * src_buf, size_t src_buf_size);
char const * bnex_http_request_get_field(bnex_http_request_t * restrict req, char const * field);

//================================================================
//----------------------------------------------------------------
//================================================================

typedef struct bnex_http_response_s {
	
	uint_fast16_t code;
	
	struct bnex_http_response_field_s {
		char * key;
		char * value;
	} * fields;
	size_t fields_len;
	
	enum bnex_http_response_writemode {
		BNEX_HTTP_RESPONSE_WRITEMODE_BUFFER,
		BNEX_HTTP_RESPONSE_WRITEMODE_FILE,
	} wmode;
	
	char * data;
	int data_fd;
	size_t data_len;
	
} bnex_http_response_t;

void bnex_http_response_create(bnex_http_response_t * restrict res);
void bnex_http_response_destroy(bnex_http_response_t * restrict res);

struct bnex_http_response_field_s * bnex_http_response_get_field(bnex_http_response_t * restrict res, char const * field);
void bnex_http_response_set_add_field(bnex_http_response_t * restrict res, char const * key, char const * value);
void bnex_http_response_set_data_buffer(bnex_http_response_t * restrict res, char const * MIME, char const * data, size_t data_size);
void bnex_http_response_set_data_file(bnex_http_response_t * restrict res, char const * MIME, int file_descriptor, size_t data_size);
void bnex_http_response_generate(bnex_http_response_t * restrict res, bnex_http_request_t const * restrict req);

//================================================================
//----------------------------------------------------------------
//================================================================

char const * http_text_for_code(uint_fast16_t code);
char const * http_path_decode_and_validate(char const *);
char const * http_path_encode(char const *);

#endif//BNEX_HTTP_H
