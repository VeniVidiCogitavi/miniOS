/*
 * kernel/syscall_handler.c  —  miniOS
 *
 * This is where the kernel actually services each system call.
 * 
 * This code contains the single kernel entry point - kernel_handle_syscall().
 * That function checks which system call has been requested and calls a specific
 * handler for that particular system call.
 *
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>    /* usleep */
#include <time.h>      /* nanosleep fallback */
#include "../include/kernel.h"
#include "../include/syscall.h"
#include "../include/kernel_utils.h"

/* ------------------------------------------------------------------ *
 *  Forward declarations for internal helpers                         *
 * ------------------------------------------------------------------ */
static syscall_result_t handle_write   (uintptr_t fd, uintptr_t buf,
                                        uintptr_t len);
static syscall_result_t handle_read    (uintptr_t fd, uintptr_t buf,
                                        uintptr_t len);
static syscall_result_t handle_spawn   (uintptr_t thread_func_ptr, uintptr_t arg_ptr);
static syscall_result_t handle_process (void);
static syscall_result_t handle_exit    (uintptr_t status);
static syscall_result_t handle_lockinit(void);
static syscall_result_t handle_lock    (void);
static syscall_result_t handle_unlock  (void);
static syscall_result_t handle_yield   (void);
static syscall_result_t handle_done    (void);
static syscall_result_t handle_getpid  (void);
static syscall_result_t handle_sleep   (uintptr_t ms);
static syscall_result_t handle_alloc   (uintptr_t size);
static syscall_result_t handle_free    (uintptr_t ptr);

/* ------------------------------------------------------------------ *
 *  Simple kernel state                                               *
 * ------------------------------------------------------------------ */
int              next_pid = 1;
int              current_processes = 0;
process_t        process_table[MAX_PROCESSES];
bool             lock = false;
int              lock_owner_pid = -1;
process_t       *current_process_ptrs[MAX_CORES] = {NULL};
pthread_mutex_t  process_lock = PTHREAD_MUTEX_INITIALIZER;
bool             is_kernel_initialized = false;

/* ------------------------------------------------------------------ *
 *  Dispatcher — the heart of the kernel                              *
 * ------------------------------------------------------------------ */
syscall_result_t kernel_handle_syscall(syscall_num_t num,
                                       uintptr_t a0,
                                       uintptr_t a1,
                                       uintptr_t a2,
                                       uintptr_t a3)
{
//    kprintf("[kernel] syscall %d  args=(%lu, %lu, %lu, %lu)\n",
//            num, a0, a1, a2, a3);

    pthread_mutex_lock(&process_lock);
    if (!is_kernel_initialized) {
        kernel_init();
        is_kernel_initialized = true;
    }
    pthread_mutex_unlock(&process_lock);

    // When adding kernel functions, add a case here, and a new "handle_*()" function
    switch (num) {
        case SYS_WRITE:    return handle_write    (a0, a1, a2);
        case SYS_READ:     return handle_read     (a0, a1, a2);
        case SYS_SPAWN:    return handle_spawn    (a0, a1);
        case SYS_PROCESS:  return handle_process  ();
        case SYS_LOCKINIT: return handle_lockinit ();
        case SYS_LOCK:     return handle_lock     ();
        case SYS_UNLOCK:   return handle_unlock   ();
        case SYS_YIELD:    return handle_yield    ();
        case SYS_DONE:     return handle_done     ();
        case SYS_EXIT:     return handle_exit     (a0);
        case SYS_GETPID:   return handle_getpid   ();
        case SYS_SLEEP:    return handle_sleep    (a0);
        case SYS_ALLOC:    return handle_alloc    (a0);
        case SYS_FREE:     return handle_free     (a0);

        default:
            kprintf("[kernel] unknown syscall %d — returning ENOSYS\n", num);
            return MINIOS_ENOSYS;
    }
}

/* ------------------------------------------------------------------ *
 *  Handler implementations                                           *
 * ------------------------------------------------------------------ */

static syscall_result_t handle_write(uintptr_t fd,
                                     uintptr_t buf,
                                     uintptr_t len)
{
    static atomic_flag lock = ATOMIC_FLAG_INIT;

    while (atomic_flag_test_and_set(&lock)) ;
    const char *s = (const char *)buf;

    if (!s)                    return MINIOS_EINVAL;
    if (fd != 1 && fd != 2)   return MINIOS_EBADF;

    FILE *stream = (fd == 1) ? stdout : stderr;
    size_t written = fwrite(s, 1, (size_t)len, stream);
    atomic_flag_clear(&lock);
    return (syscall_result_t)written;
}

static syscall_result_t handle_read(uintptr_t fd,
                                    uintptr_t buf,
                                    uintptr_t len)
{
//    kprintf("[kernel] handle_read\n");
    char *s = (char *)buf;

    if (!s)    return MINIOS_EINVAL;
    if (fd != 0) return MINIOS_EBADF;

    if (!fgets(s, (int)len, stdin))
        return 0;
    return (syscall_result_t)strlen(s);
}

static syscall_result_t handle_spawn(uintptr_t thread_func_ptr, uintptr_t arg_ptr)
{
    kprintf("[kernel] handle_spawn\n");
    int returnValue;

    // We need to protect this with the process lock.
    pthread_mutex_lock(&process_lock);
    process_t *process_ptr = NULL;

    // Look for an available process slot in the process table
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == 0) {
            process_ptr = &process_table[i];
            break;
        }
    }

    if (process_ptr) {
        // Process slot found
        process_ptr->pid = next_pid++;
        kprintf("[kernel] spawning new process with pid %d\n", process_ptr->pid);
        process_ptr->state = PROC_READY;
        process_ptr->run_flag = false;
        process_ptr->condition = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
        pthread_create(&process_ptr->thread, NULL, (void *(*)(void *))thread_func_ptr, (void *)arg_ptr);
        returnValue = process_ptr->pid;
    } else {
        kprintf("[kernel] no available process slots\n");
        returnValue = MINIOS_EMAXPROCESSES;
    }

    pthread_mutex_unlock(&process_lock);
    return returnValue;
}

static syscall_result_t handle_process()
{
    pthread_exit(NULL);  // main exits, but threads stay alive
}

static syscall_result_t handle_exit(uintptr_t status)
{
    kprintf("[kernel] handle_exit() with status %lu\n", status);
    exit((int)status);
    return MINIOS_OK;   /* unreachable, but keeps the compiler happy */
}

static syscall_result_t handle_lockinit(void)
{
    kprintf("[kernel] handle_lockinit\n");
    return MINIOS_OK;
}


/**
 * Attempts to obtain the user-level lock. If the lock is already held, it will put the current process
 * to sleep until the lock is available.
 */
static syscall_result_t handle_lock(void)
{
    kprintf("[kernel] handle_lock\n");

    process_t *this_process_ptr = find_process_self();
    pthread_mutex_lock(&process_lock);

    while (lock) {
        // The lock is already held, so we need to yield the CPU to another process.
        // Theoretically, we will only come back when "lock" is cleared, and we will
        // be the next process to acquire it. However, it's possible that another
        // process could acquire the lock before we do, so we need to check again in the while loop.

        swap_in_ready_process(find_core_for_process(this_process_ptr));
        swap_process_out(this_process_ptr, PROC_WAIT_LOCK);
    }

    lock = true;
    lock_owner_pid = this_process_ptr->pid;

    pthread_mutex_unlock(&process_lock);

    return MINIOS_OK;
}


/**
 * Releases the user-level lock. If there are any processes waiting for the lock, it will wake one of them up.
 */
static syscall_result_t handle_unlock(void)
{
    kprintf("[kernel] handle_unlock\n");

    pthread_mutex_lock(&process_lock);

    // Let's check that we're the owner of the lock before we unlock it. This protects against
    // various errors, including double-unlocking.
    process_t *this_process_ptr = find_process_self();
    if (lock && (lock_owner_pid == this_process_ptr->pid)) {
        lock = false;
        lock_owner_pid = -1;

        // If there are any processes waiting for the lock, we need to wake one of them up.
        process_t *waiting_process_ptr = find_process_by_state(PROC_WAIT_LOCK);
        if (waiting_process_ptr) {
            // First see if there's an idle core to run the waiting process on (which we may have just
            // left when we went to sleep)
            const int idle_core_id = find_idle_core();
            if (idle_core_id >= 0) {
                kprintf("[kernel] swapping in process %d after waiting for lock, on core %d\n", waiting_process_ptr->pid, idle_core_id);
                swap_process_in(waiting_process_ptr, idle_core_id);
            } else {
                // No idle cores, so we just mark the process as ready and let it be swapped in later.
                kprintf("[kernel] marking process %d READY after waiting for lock\n", waiting_process_ptr->pid);
                waiting_process_ptr->state = PROC_READY;
            }
        }
    }
    pthread_mutex_unlock(&process_lock);
    return MINIOS_OK;
}


static syscall_result_t handle_yield(void)
{
    pthread_mutex_lock(&process_lock);
    
    process_t *this_process_ptr = find_process_self();
    kprintf("[kernel] handle_yield from process %d: ", this_process_ptr->pid);

    if (this_process_ptr->state == PROC_READY) {
        // This is a new process that has just been spawned, so we need to swap it in if there's an idle core.
        const int idle_core_id = find_idle_core();
        if (idle_core_id >= 0) {
            kprintf("swapping new process in, on core %d\n", idle_core_id);
            swap_process_in(this_process_ptr, idle_core_id);
        } else {
            kprintf("swapping new process out\n");
            swap_process_out(this_process_ptr, PROC_READY);
        }
    } else {
        // This is one of the currently running process, so we need to check whether its timeslice has expired,
        // and if so, swap it out and swap in another ready process.
        if (is_timeslice_expired(&this_process_ptr->slice_expire_time)) {
            // If there's another ready process, swap it in
            process_t *swapped_process_ptr = swap_in_ready_process(find_core_for_process(this_process_ptr));
            if (swapped_process_ptr) {
                // If we swapped in another process, we need to swap this one out
                kprintf("timeslice expired, swapped in process %d\n", swapped_process_ptr->pid);
                swap_process_out(this_process_ptr, PROC_READY);
            } else {
                // There were no ready processes, we just continue running this one for now
                kprintf("timeslice expired, no ready process, continuing\n");
            }
        } else {
            // Timeslice hasn't expired, so we just continue running this process
            kprintf("timeslice not expired\n");
        }
    }

    fflush(stderr);
    pthread_mutex_unlock(&process_lock);
    return MINIOS_OK;
}


/**
 * Called by a process when it's done, to allow the kernel to clean up and schedule another process.
 */
static syscall_result_t handle_done(void)
{
    kprintf("[kernel] handle_done\n");

    process_t *this_process_ptr = find_process_self();

    // This will release the user-level lock if it's held by this process.
    handle_unlock();

    // Make sure not to lock the mutex until after calling handle_unlock(), to avoid deadlock.
    pthread_mutex_lock(&process_lock);

    this_process_ptr->pid = 0;

    const int core_id = find_core_for_process(this_process_ptr);
    if (!swap_in_ready_process(core_id)) {
        // No ready processes, so we reset the process pointer.
        current_process_ptrs[core_id] = NULL;
    }

    pthread_mutex_unlock(&process_lock);
    return MINIOS_OK;
}

static syscall_result_t handle_getpid(void)
{
    kprintf("[kernel] handle_getpid\n");
    process_t *this_process_ptr = find_process_self();
    return (syscall_result_t)this_process_ptr->pid;
}

static syscall_result_t handle_sleep(uintptr_t ms)
{
    kprintf("[kernel] handle_sleep\n");
    struct timespec ts;
    ts.tv_sec  = (time_t)(ms / 1000);
    ts.tv_nsec = (long)((ms % 1000) * 1000000L);
    nanosleep(&ts, NULL);
    return MINIOS_OK;
}

static syscall_result_t handle_alloc(uintptr_t size)
{
    kprintf("[kernel] handle_alloc\n");
    if (size == 0) return MINIOS_EINVAL;

    void *ptr = malloc((size_t)size);
    if (!ptr) return MINIOS_ENOMEM;

    kprintf("[kernel] alloc %lu bytes → %p\n", size, ptr);
    return (syscall_result_t)(uintptr_t)ptr;
}

static syscall_result_t handle_free(uintptr_t ptr)
{
    kprintf("[kernel] handle_free\n");
    if (!ptr) return MINIOS_EINVAL;
    free((void *)ptr);
    kprintf("[kernel] free %p\n", (void *)ptr);
    return MINIOS_OK;
}
