#include "../include/kernel.h"

extern int find_idle_core(void);
extern int find_core_for_process(process_t *process_ptr);
extern process_t *find_process_by_state(proc_state_t state);
extern void swap_process_in(process_t *process_ptr, int core_id);
extern void swap_process_out(process_t *process_ptr, proc_state_t new_state);
extern process_t *swap_in_ready_process(int core_id);
extern process_t *find_process_self(void);
extern bool is_timeslice_expired(struct timespec *expire_time);