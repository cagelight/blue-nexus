#ifndef PROVIDER_FILESYSTEM_H
#define PROVIDER_FILESYSTEM_H

#include "com.h"
#include "bnex_http.h"

bool provider_fs_handle(bnex_http_request_t const * restrict req, bnex_http_response_t * restrict res);

#endif//PROVIDER_FILESYSTEM_H
