#ifndef BNEX_MODULE_EXPORT_H
#define BNEX_MODULE_EXPORT_H

#include <stddef.h>
#include <stdbool.h>

#define BNEX_CORE_IMPL extern
#define BNEX_MODULE_IMPL __attribute__((visibility("default")))

BNEX_CORE_IMPL char const * bnex_http_request_get_field(void const * request_handle, char const * field);
BNEX_CORE_IMPL void bnex_http_response_set_add_field(void * response_handle, char const * key, char const * value);
BNEX_CORE_IMPL void bnex_http_response_set_data(void * response_handle, char const * MIME, char const * data, size_t data_size);

//REQUIRED
BNEX_MODULE_IMPL void bnex_module_init();
BNEX_MODULE_IMPL void bnex_module_term();
BNEX_MODULE_IMPL bool bnex_module_should_gen(char const * path);
BNEX_MODULE_IMPL void bnex_module_gen(char const * mode, char const * path, void * response_handle, void const * request_handle);

#endif//BNEX_MODULE_EXPORT_H
