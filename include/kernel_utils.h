#include "../include/kernel.h"

extern void swap_process_in(process_t *process_ptr);
extern void swap_process_out(process_t *process_ptr);
extern bool swap_in_ready_process(void);
extern process_t *find_process_by_thread_id(pthread_t threadId);
extern bool is_timeslice_expired(struct timespec *expire_time);