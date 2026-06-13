/*
 * kernel.h  —  miniOS kernel internals
 *
 * This will be included for kernel side code, but not user-level code.
 */

#ifndef MINIOS_KERNEL_H
#define MINIOS_KERNEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <stdatomic.h>
#include "syscall.h"


/* ------------------------------------------------------------------ *
 *  File-descriptor table (tiny, fixed-size for simplicity)           *
 * ------------------------------------------------------------------ */
#define KERNEL_MAX_FDS 16

typedef enum {
    FD_STDIN  = 0,
    FD_STDOUT = 1,
    FD_STDERR = 2,
} std_fd_t;


/* ------------------------------------------------------------------ *
 *  Process States                                                    *
 * ------------------------------------------------------------------ */
typedef enum {
    PROC_READY,
    PROC_RUNNING,
    PROC_DONE
} proc_state_t;

/* ------------------------------------------------------------------ *
 *  Process descriptor                                                 *
 *  Extend this struct as you add scheduling, memory management, etc. *
 * ------------------------------------------------------------------ */
typedef struct {
    int             pid;
    proc_state_t    state;
    pthread_t       thread;
    bool            run_flag;
    pthread_cond_t  condition;
    struct timespec slice_expire_time;
    int             exit_code;
    char            name[32];
} process_t;

/* ------------------------------------------------------------------ *
 *  Kernel subsystem init / shutdown                                   *
 * ------------------------------------------------------------------ */
void kernel_init(void);
void kernel_shutdown(void);

/* ------------------------------------------------------------------ *
 *  Internal handler — do not call from user space                    *
 *  Implemented in kernel/syscall_handler.c                           *
 * ------------------------------------------------------------------ */
syscall_result_t kernel_handle_syscall(syscall_num_t num,
                                       uintptr_t a0,
                                       uintptr_t a1,
                                       uintptr_t a2,
                                       uintptr_t a3);

/* ------------------------------------------------------------------ *
 *  Kernel logging (kernel/kprintf.c)                                  *
 * ------------------------------------------------------------------ */
void kprintf(const char *fmt, ...);


/* ------------------------------------------------------------------ *
 *  Kernel state globals                                              *
 * ------------------------------------------------------------------ */
#define MAX_PROCESSES 2
extern int              next_pid;
extern int              current_processes;
extern process_t        process_table[];
extern atomic_flag      lock;
extern process_t       *current_process_ptr;
extern pthread_mutex_t  process_lock;
extern bool             is_kernel_initialized;

#endif /* MINIOS_KERNEL_H */
