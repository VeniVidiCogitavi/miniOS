#include "../include/kernel.h"

extern void swap_process_in(process_t *process_ptr);
extern void swap_process_out(process_t *process_ptr);
extern process_t *swap_in_ready_process(void);
extern process_t *find_process_self();
extern bool is_timeslice_expired(struct timespec *expire_time);