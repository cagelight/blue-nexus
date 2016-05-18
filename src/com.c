#include "com.h"

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <unistd.h>

pthread_spinlock_t prnt_mut;

static char * cwd_internal;
char const * const * const cwd = (char const * const * const)&cwd_internal;

void com_init(void) {
	cwd_internal = malloc(512);
	getcwd(cwd_internal, 512);
	pthread_spin_init(&prnt_mut, PTHREAD_PROCESS_PRIVATE);
}

//============================== VAS =============================
//	Thread safe function for quick string formatting.
//	Allows nesting to an extent.

thread_local char vbufs [VAS_BUFNUM][VAS_BUFLEN];
thread_local int vbufn = 0;

char * vas(char const * fmt, ...) {
	va_list va;
	va_start(va, fmt);
	char * vbuf = vbufs[vbufn++];
	vsnprintf(vbuf, VAS_BUFLEN, fmt, va);
	va_end(va);
	if (vbufn == VAS_BUFNUM) vbufn = 0;
	return vbuf;
}

char * vas_next() {
	char * vbuf = vbufs[vbufn++];
	if (vbufn == VAS_BUFNUM) vbufn = 0;
	return vbuf;
}

//================================================================


void com_print(char const * str) {
	pthread_spin_lock(&prnt_mut);
	printf("%s", str);
	printf("\n");
	pthread_spin_unlock(&prnt_mut);
}
