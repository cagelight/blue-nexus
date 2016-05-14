#ifndef RWSLCK_H
#define RWSLCK_H

#include <stdatomic.h>

//================================================================
//----------------------------------------------------------------
//================================================================

// === READ/WRITE SPINLOCK ===

typedef struct rwslck_s {
	atomic_int_fast64_t readers;
	atomic_flag accessor;
	atomic_flag write_sem;
} rwslck_t;

inline void rwslck_init(rwslck_t * rws) {
	atomic_store(&rws->readers, 0);
	atomic_flag_clear(&rws->accessor);
	atomic_flag_clear(&rws->write_sem);
}

inline void rwslck_read_enter_b(rwslck_t * rws) {
	while (atomic_flag_test_and_set(&rws->accessor)) {
		__asm volatile ("pause" ::: "memory");
	}
	while (atomic_flag_test_and_set(&rws->write_sem)) {
		__asm volatile ("pause" ::: "memory");
	}
	atomic_flag_clear(&rws->write_sem);
	atomic_fetch_add(&rws->readers, 1);
	atomic_flag_clear(&rws->accessor);
}

inline bool rwslck_read_enter_nb(rwslck_t * rws) {
	if (atomic_flag_test_and_set(&rws->accessor)) {
		return false;
	}
	if (atomic_flag_test_and_set(&rws->write_sem)) {
		atomic_flag_clear(&rws->accessor);
		return false;
	}
	atomic_flag_clear(&rws->write_sem);
	atomic_fetch_add(&rws->readers, 1);
	atomic_flag_clear(&rws->accessor);
	return true;
}

inline void rwslck_read_leave(rwslck_t * rws) {
	atomic_fetch_sub(&rws->readers, 1);
}

inline void rwslck_write_lock_b(rwslck_t * rws) {
	while (atomic_flag_test_and_set(&rws->accessor)) {
		__asm volatile ("pause" ::: "memory");
	}
	while (atomic_flag_test_and_set(&rws->write_sem)) {
		__asm volatile ("pause" ::: "memory");
	}
	while (atomic_load(&rws->readers)) {
		__asm volatile ("pause" ::: "memory");
	}
	atomic_flag_clear(&rws->accessor);
}

inline bool rwslck_write_lock_nb(rwslck_t * rws) {
	while (atomic_flag_test_and_set(&rws->accessor)) {
		return false;
	}
	while (atomic_flag_test_and_set(&rws->write_sem)) {
		atomic_flag_clear(&rws->accessor);
		return false;
	}
	while (atomic_load(&rws->readers)) {
		atomic_flag_clear(&rws->write_sem);
		atomic_flag_clear(&rws->accessor);
		return false;
	}
	atomic_flag_clear(&rws->accessor);
	return true;
}

inline void rwslck_write_unlock(rwslck_t * rws) {
	atomic_flag_clear(&rws->write_sem);
}

inline void rwslck_degrade_write_to_read(rwslck_t * rws) {
	atomic_fetch_add(&rws->readers, 1);
	atomic_flag_clear(&rws->write_sem);
}

//================================================================
//----------------------------------------------------------------
//================================================================

#endif//RWSLCK_H

