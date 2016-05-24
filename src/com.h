#ifndef BNEX_COM_H
#define BNEX_COM_H

#define _GNU_SOURCE

#include <jemalloc/jemalloc.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <string.h>

#define thread_local _Thread_local
#define atomic _Atomic
#define ipause __asm volatile ("pause" ::: "memory")
#define forever for (;;)
#define lastchr(ptr) ptr[strlen(ptr)-1]

void com_init(void);

#define VAS_BUFLEN 1024
#define VAS_BUFNUM 8
char * vas(char const * fmt, ...);
char * vas_next();

extern char const * const * const cwd;

void com_print(char const *);
#define com_printf(str, ...) com_print(vas(str, ##__VA_ARGS__))

#define com_print_level(str, lev) com_print(vas("%s (%s, line %u): %s", #lev, __func__, __LINE__, str))
#define com_printf_level(str, lev, ...) com_print(vas("%s (%s, line %u): %s", #lev, __func__, __LINE__, vas(str, ##__VA_ARGS__)))

#define com_print_info(str) com_print_level(str, INFO)
#define com_print_warning(str) com_print_level(str, WARNING)
#define com_print_error(str) com_print_level(str, ERROR)
#define com_print_fatal(str) com_print_level(str, FATAL)

#define com_printf_info(str, ...) com_printf_level(str, INFO, ##__VA_ARGS__)
#define com_printf_warning(str, ...) com_printf_level(str, WARNING, ##__VA_ARGS__)
#define com_printf_error(str, ...) com_printf_level(str, ERROR, ##__VA_ARGS__)
#define com_printf_fatal(str, ...) com_printf_level(str, FATAL, ##__VA_ARGS__)

#endif//BNEX_COM_H
