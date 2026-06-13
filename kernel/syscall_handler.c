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
atomic_flag      lock = ATOMIC_FLAG_INIT;
process_t       *current_process_ptr = NULL;
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

    // First we need to deal with some status globals, so we need to protect these with the process lock.
    pthread_mutex_lock(&process_lock);
    process_t *process_ptr = NULL;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == 0) {
            process_ptr = &process_table[i];
            break;
        }
    }
    if (!process_ptr) return MINIOS_EMAXPROCESSES;

    int pid = next_pid++;
    pthread_mutex_unlock(&process_lock);

    process_ptr->pid = pid;
    process_ptr->state = PROC_READY;
    process_ptr->run_flag = false;
    process_ptr->condition = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    pthread_create(&process_ptr->thread, NULL, (void *(*)(void *))thread_func_ptr, (void *)arg_ptr);
    return pid;
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

static syscall_result_t handle_lock(void)
{
    kprintf("[kernel] handle_lock\n");
    while (atomic_flag_test_and_set(&lock)) {
        ;
    }
    return MINIOS_OK;
}

static syscall_result_t handle_unlock(void)
{
    kprintf("[kernel] handle_unlock\n");
    atomic_flag_clear(&lock);
    return MINIOS_OK;
}

static syscall_result_t handle_yield(void)
{
    kprintf("[kernel] handle_yield from process ");
    pthread_mutex_lock(&process_lock);
    
    process_t *this_process_ptr = find_process_self();
    kprintf("%d: ", this_process_ptr->pid);

    if (!current_process_ptr) {
        // No process is currently running. This must be being called from a
        // newly-spawned process before it has been swapped in.
        swap_process_in(this_process_ptr);
        kprintf("new process - swapped in\n");
    } else if (current_process_ptr->pid != this_process_ptr->pid) {
        // This process is trying to yield, but it's not the currently running process.
        // This means that it's a newly-spawned process that hasn't been swapped in yet
        // We need to put it into a wait state for now.
        swap_process_out(this_process_ptr);
        kprintf("new process - swapped out\n");
    } else {
        // This is the currently running process, so we need to check whether its timeslice has expired,
        // and if so, swap it out and swap in another ready process.
        if (is_timeslice_expired(&current_process_ptr->slice_expire_time)) {
            kprintf("timeslice expired, ");
            process_t *outgoing_process_ptr = current_process_ptr;

            // If there's another ready process, swap it in
            process_t *swapped_process_ptr = swap_in_ready_process();
            if (swapped_process_ptr) {
                // If we swapped in another process, we need to swap this one out
                kprintf("swapped in process %d, ", swapped_process_ptr->pid);
                swap_process_out(outgoing_process_ptr);
                kprintf("swapped out current process\n");
            } else {
                // There were no ready processes, we just continue running this one for now
                kprintf("no ready process, continuing\n");
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

static syscall_result_t handle_done(void)
{
    kprintf("[kernel] handle_done\n");
    // This is called by a process when it's done, to allow the kernel to clean up and schedule another process.
    pthread_mutex_lock(&process_lock);
    current_process_ptr->pid = 0;
    if (!swap_in_ready_process()) {
        // No ready processes, so we reset the process pointer.
        current_process_ptr = NULL;
    }

    pthread_mutex_unlock(&process_lock);
    return MINIOS_OK;
}

static syscall_result_t handle_getpid(void)
{
    kprintf("[kernel] handle_getpid\n");
    return (syscall_result_t)current_process_ptr->pid;
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
