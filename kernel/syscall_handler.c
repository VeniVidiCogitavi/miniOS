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
#include <stdatomic.h>
#include <time.h>      /* nanosleep fallback */
#include "../include/kernel.h"
#include "../include/syscall.h"

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
static syscall_result_t handle_getpid  (void);
static syscall_result_t handle_sleep   (uintptr_t ms);
static syscall_result_t handle_alloc   (uintptr_t size);
static syscall_result_t handle_free    (uintptr_t ptr);

/* ------------------------------------------------------------------ *
 *  Simple kernel state                                               *
 * ------------------------------------------------------------------ */
#define MAX_PROCESSES 2
static int next_pid = 1;
static int current_processes = 0;
static process_t process_table[MAX_PROCESSES];
static atomic_flag lock = ATOMIC_FLAG_INIT;


/* ------------------------------------------------------------------ *
 *  Dispatcher — the heart of the kernel                              *
 * ------------------------------------------------------------------ */
syscall_result_t kernel_handle_syscall(syscall_num_t num,
                                       uintptr_t a0,
                                       uintptr_t a1,
                                       uintptr_t a2,
                                       uintptr_t a3)
{
    kprintf("[kernel] syscall %d  args=(%lu, %lu, %lu, %lu)\n",
            num, a0, a1, a2, a3);

    // When adding kernel functions, add a case here, and a new "handle_*()" function
    switch (num) {
        case SYS_WRITE:    return handle_write    (a0, a1, a2);
        case SYS_READ:     return handle_read     (a0, a1, a2);
        case SYS_SPAWN:    return handle_spawn    (a0, a1);
        case SYS_PROCESS:  return handle_process  ();
        case SYS_LOCKINIT: return handle_lockinit ();
        case SYS_LOCK:     return handle_lock     ();
        case SYS_UNLOCK:   return handle_unlock   ();
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
    char *s = (char *)buf;

    if (!s)    return MINIOS_EINVAL;
    if (fd != 0) return MINIOS_EBADF;

    if (!fgets(s, (int)len, stdin))
        return 0;
    return (syscall_result_t)strlen(s);
}

static syscall_result_t handle_spawn(uintptr_t thread_func_ptr, uintptr_t arg_ptr)
{
    if (current_processes >= MAX_PROCESSES) return MINIOS_EMAXPROCESSES;

    int i = current_processes++;
    int pid = next_pid++;

    process_table[i].pid = pid;
    process_table[i].state = PROC_READY;
    process_table[i].thread_func_ptr = (void * (*)(void *))thread_func_ptr; 
    process_table[i].thread_arg_ptr = (void *)arg_ptr;

    return pid;
}

static syscall_result_t handle_process()
{
    // Start the processes
    for (int i = 0; i < current_processes; i++) {
        pthread_create(
            &process_table[i].thread,
            NULL,
            process_table[i].thread_func_ptr,
            process_table[i].thread_arg_ptr
        );
        process_table[i].state = PROC_RUNNING;
    }

    // Wait for each process to finish
    int numProcesses = current_processes;
    for (int i = 0; i < numProcesses; i++) {
        pthread_join(process_table[i].thread, NULL);
        process_table[i].state = PROC_DONE;
        current_processes--;
    }

    return MINIOS_OK;
}

static syscall_result_t handle_exit(uintptr_t status)
{
    kprintf("[kernel] process exiting with status %lu\n", status);
    exit((int)status);
    return MINIOS_OK;   /* unreachable, but keeps the compiler happy */
}

static syscall_result_t handle_lockinit(void)
{
    return MINIOS_OK;
}

static syscall_result_t handle_lock(void)
{
    while (atomic_flag_test_and_set(&lock)) {
        ;
    }
    return MINIOS_OK;
}

static syscall_result_t handle_unlock(void)
{
    atomic_flag_clear(&lock);
    return MINIOS_OK;
}

static syscall_result_t handle_getpid(void)
{
    /* In a real kernel this would return the running process's PID.
     * Here we hand out incrementing IDs to illustrate the concept. */
    return (syscall_result_t)next_pid++;
}

static syscall_result_t handle_sleep(uintptr_t ms)
{
    struct timespec ts;
    ts.tv_sec  = (time_t)(ms / 1000);
    ts.tv_nsec = (long)((ms % 1000) * 1000000L);
    nanosleep(&ts, NULL);
    return MINIOS_OK;
}

static syscall_result_t handle_alloc(uintptr_t size)
{
    if (size == 0) return MINIOS_EINVAL;

    void *ptr = malloc((size_t)size);
    if (!ptr) return MINIOS_ENOMEM;

    kprintf("[kernel] alloc %lu bytes → %p\n", size, ptr);
    return (syscall_result_t)(uintptr_t)ptr;
}

static syscall_result_t handle_free(uintptr_t ptr)
{
    if (!ptr) return MINIOS_EINVAL;
    free((void *)ptr);
    kprintf("[kernel] free %p\n", (void *)ptr);
    return MINIOS_OK;
}
