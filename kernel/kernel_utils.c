#include "../include/kernel_utils.h"


/**
 * Returns the ID of an idle core, or -1 if there are no idle cores.
 * This should be called while the process_lock is held.
 */
int find_idle_core(void) {
    for (int i = 0; i < MAX_CORES; i++) {
        if (current_process_ptrs[i] == NULL) {
            return i;
        }
    }
    return -1;
}


/**
 * Finds the core ID of the current process. Returns -1 if the process is not running on any core.
 * This should be called while the process_lock is held.
 */
int find_core_for_process(process_t *process_ptr) {
    for (int i = 0; i < MAX_CORES; i++) {
        if (current_process_ptrs[i] == process_ptr) {
            return i;
        }
    }
    return -1;
}


/**
 * Finds a process in the given state and returns a pointer to it, or NULL if there are no processes in that state.
 * This should be called while the process_lock is held.
 */
process_t *find_process_by_state(proc_state_t state) {
    static int circular_index = 0;  // Used to implement a round-robin search.

    for (int i = 0; i < MAX_PROCESSES; i++) {
        circular_index = (++circular_index) % MAX_PROCESSES;
        if ((process_table[circular_index].pid != 0) && (process_table[circular_index].state == state)) {
            return &process_table[circular_index];
        }
    }
    return NULL;
}

/**
 * Swap the given process in, making it the currently running process.
 * This should be called while the process_lock is held.
 */
void swap_process_in(process_t *process_ptr, int core_id) {
    process_ptr->state = PROC_RUNNING;
    process_ptr->run_flag = true;

    // Store the time when this process's timeslice will expire, so we can check it when it yields or makes a syscall
    clock_gettime(CLOCK_MONOTONIC, &process_ptr->slice_expire_time);
    process_ptr->slice_expire_time.tv_sec++;
    current_process_ptrs[core_id] = process_ptr;

    // Signal the process to run
    pthread_cond_signal(&process_ptr->condition);
}


/**
 * Swap the given process out, putting it back into the ready state.
 * We've already swapped another process in, so we shouldn't touch the global state values.
 * This should be called while the process_lock is held.
 */
void swap_process_out(process_t *process_ptr, proc_state_t new_state) {
//    kprintf("[kernel] swapping process out: %d\n", process_ptr->pid);
    process_ptr->state = new_state;
    process_ptr->run_flag = false;

    // Wait for the process to be signaled to run again. This will cause this thread
    // to wait, releasing the process lock.
    //
    // Note: This looks like a busy wait, but it's not. In most cases, the thread will be inactive until
    // it's signaled, but the pthread spec allows for spurious wakeups, so this loop is part
    // of the correct usage pattern for condition variables.
    while (!process_ptr->run_flag) {
        pthread_cond_wait(&process_ptr->condition, &process_lock);
    }
}


/**
 * Find a process in the ready state and swap it in. Returns the pointer to the process that was swapped in,
 * or NULL if there were no ready processes.
 * This should be called while the process_lock is held.
 */
process_t *swap_in_ready_process(int core_id) {
//    kprintf("[kernel] swapping in ready process");

    const process_t *ready_process_ptr = find_process_by_state(PROC_READY);
    if (ready_process_ptr) {
        swap_process_in((process_t *)ready_process_ptr, core_id);
    }
    return ready_process_ptr;
}


/**
 * Find a process by matching its thread ID. The thread is available to us anywhere.
 */
process_t *find_process_self(void) {
    pthread_t threadId = pthread_self();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].thread == threadId) {
            return &process_table[i];
        }
    }
    return NULL;
}


/**
 * Checks a timeslice expiration value and checks whether it's expired.
 */
bool is_timeslice_expired(struct timespec *expire_time) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec > expire_time->tv_sec) || (now.tv_sec == expire_time->tv_sec && now.tv_nsec >= expire_time->tv_nsec);
}