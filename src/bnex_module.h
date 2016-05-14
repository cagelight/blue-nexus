#ifndef BNEX_MODULE_H
#define BNEX_MODULE_H

#include "com.h"

#include <stdbool.h>

typedef void (*bnexmodfunc_init) (void);
typedef void (*bnexmodfunc_term) (void);
typedef bool (*bnexmodfunc_shouldgen) (char const *);
typedef void (*bnexmodfunc_gen) (char const *, char const *, void *, void const *);

typedef struct bnex_module_s {
	void * handle;
	bnexmodfunc_init init;
	bnexmodfunc_term term;
	bnexmodfunc_shouldgen should_gen;
	bnexmodfunc_gen gen;
} bnex_module_t;

#endif //BNEX_MODULE_H
