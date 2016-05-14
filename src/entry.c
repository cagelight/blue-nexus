#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "bnex_pool.h"
#include "bnex_connection.h"

static bool interrupt_called = false;

static void catch_signal (int sig) {
	switch (sig) {
	case SIGINT:
		if (interrupt_called) abort();
		interrupt_called = true;
		break;
	case SIGPIPE:
		com_print_warning("a SIGPIPE was ignored");
		break;
	default:
		break;
	};
}

int main() {
	signal(SIGINT, catch_signal);
	signal(SIGPIPE, catch_signal);
	com_init();
	
	struct timespec slptim = {
		.tv_sec = 0,
		.tv_nsec = 25000000,
	};
	
	bnex_pool_init();

	while (!interrupt_called) {
		nanosleep(&slptim, NULL);
	}
	
	bnex_pool_shutdown();
	
}
