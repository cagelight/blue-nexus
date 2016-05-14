#ifndef BNEX_POOL_H
#define BNEX_POOL_H

#include "com.h"

void bnex_pool_init();
void bnex_pool_shutdown();
size_t bnex_pool_get_connection_count();

#endif//BNEX_POOL_H
