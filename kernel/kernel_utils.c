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
void swap_process_out(process_t *process_ptr) {
//    kprintf("[kernel] swapping process out: %d\n", process_ptr->pid);
    process_ptr->state = PROC_READY;
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
 * circular_index is used to keep track of where we left off in the process table, so that we can implement
 * a round-robin scheduling policy.
 * This should be called while the process_lock is held.
 */
process_t *swap_in_ready_process(int core_id) {
//    kprintf("[kernel] swapping in ready process");
    static int circular_index = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        const int checkIndex = (circular_index + i) % MAX_PROCESSES;
        if ((process_table[checkIndex].pid != 0) && (process_table[checkIndex].state == PROC_READY)) {
            swap_process_in(&process_table[checkIndex], core_id);
            return &process_table[checkIndex];
        }
    }
    return NULL;
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