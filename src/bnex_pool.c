#include "bnex_pool.h"
#include "bnex_connection.h"
#include "rwslck.h"

#include <time.h>
#include <pthread.h>
#include <string.h>
#include <sys/sysinfo.h>

typedef struct bnex_pool_node_s {
	struct bnex_pool_node_s * next;
	struct bnex_pool_node_s * prev;
	bnex_connection_t * con;
	atomic_flag ctrl;
} bnex_pool_node_t;

static bnex_socket_t listener;
static bnex_pool_node_t anchor;
static rwslck_t pool_rwslck;
static atomic_bool runsem;

static struct timespec thread_idle_waits [4] = {
	{
		.tv_sec = 0,
		.tv_nsec = 5000,
	},
	{
		.tv_sec = 0,
		.tv_nsec = 1000000,
	},
	{
		.tv_sec = 0,
		.tv_nsec = 25000000,
	},
	{
		.tv_sec = 0,
		.tv_nsec = 500000000,
	},
};

pthread_t * threads;
size_t threads_len;

//================================================================
//----------------------------------------------------------------
//================================================================

thread_local bnex_connection_t * stash;

static void * bnex_pool_thread_run(void * parm __attribute__((unused))) {
	
	stash = malloc(sizeof(bnex_connection_t));
	bnex_connection_create(stash);
	
	bnex_pool_node_t * node = &anchor;
	
	bnex_connection_cycle_result_t cycle_res;
	
	uint_fast32_t work_timer = 0;
	
	do {
		if (rwslck_read_enter_nb(&pool_rwslck)) {
			node = node->next;
			rwslck_read_leave(&pool_rwslck);
			
			continue_no_next:
			
			if (atomic_flag_test_and_set(&node->ctrl)) {
				continue;
			}
			
			if (!node->con) { //anchor node
				
				if (bnex_connection_establish(stash, &listener)) {
					bnex_pool_node_t * new_node = malloc(sizeof(bnex_pool_node_t));
					com_printf_info("new connection: %p", new_node);
					new_node->con = stash;
					atomic_flag_clear(&new_node->ctrl);
					rwslck_write_lock_b(&pool_rwslck);
					node->prev->next = new_node;
					new_node->prev = node->prev;
					node->prev = new_node;
					new_node->next = node;
					rwslck_write_unlock(&pool_rwslck);
					stash = malloc(sizeof(bnex_connection_t));
					bnex_connection_create(stash);
					work_timer = 0;
				}
				
				atomic_flag_clear(&node->ctrl);
				
				if (work_timer) {
					if (work_timer < 1000)
						nanosleep(&thread_idle_waits[0], NULL);
					else if (work_timer < 2000)
						nanosleep(&thread_idle_waits[1], NULL);
					else if (work_timer < 2100)
						nanosleep(&thread_idle_waits[2], NULL);
					else
						nanosleep(&thread_idle_waits[3], NULL);
				}
				
				work_timer++;
				
				continue;
			}
			
			cycle_res = bnex_connection_cycle(node->con);
			if (cycle_res == BNEX_CONNECTION_CYCLE_TERMINATE) {
				com_printf_info("closing connection: %p", node);
				bnex_connection_destroy(node->con);
				free(node->con);
				rwslck_write_lock_b(&pool_rwslck);
				node->prev->next = node->next;
				node->next->prev = node->prev;
				rwslck_degrade_write_to_read(&pool_rwslck);
				bnex_pool_node_t * willy = node;
				node = node->next;
				rwslck_read_leave(&pool_rwslck);
				free(willy);
				work_timer = 0;
				goto continue_no_next;
			} else if (cycle_res != BNEX_CONNECTION_CYCLE_IDLE) {
				work_timer = 0;
			}
			
			atomic_flag_clear(&node->ctrl);
		}
	} while (atomic_load(&runsem));
	
	bnex_connection_destroy(stash);
	free(stash);
	
	return NULL;
}

//================================================================
//----------------------------------------------------------------
//================================================================

void bnex_pool_init() {
	atomic_store(&runsem, true);
	anchor.next = &anchor;
	anchor.prev = &anchor;
	anchor.con = NULL;
	atomic_flag_clear(&anchor.ctrl);
	rwslck_init(&pool_rwslck);
	
	bnex_socket_create(&listener);
	
	uint16_t start_port = 2080;
	while (!bnex_socket_listener_open(&listener, start_port++)) {
		com_printf_error("could not bind to %hu", start_port-1);
	}
	com_printf_info("bound to %hu", start_port-1);
	
	threads_len = get_nprocs_conf();
	threads = malloc(threads_len * sizeof(pthread_t));
	
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
	for (size_t i = 0; i < threads_len; i++) {
		pthread_create(threads+i, &attr, bnex_pool_thread_run, NULL);
	}
	
}

void bnex_pool_shutdown() {
	atomic_store(&runsem, false);
	for (size_t i = 0; i < threads_len; i++) {
		pthread_join(threads[i], NULL);
	}
}

//================================================================
//----------------------------------------------------------------
//================================================================

size_t bnex_pool_get_connection_count() {
	size_t c = 0;
	rwslck_read_enter_b(&pool_rwslck);
	bnex_pool_node_t * node = anchor.next;
	while (node != &anchor) {
		c++;
		node = node->next;
	}
	rwslck_read_leave(&pool_rwslck);
	return c;
}
