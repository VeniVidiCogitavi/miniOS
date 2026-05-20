/*
 * kernel/syscall_handler.c  —  miniOS
 *
 * This is where the kernel actually services each system call.
 * 
 * This code contains the single kernel entry point - kernel_handle_syscall().
 * That function checks which system call has been requested and calls a specific
 * handler for that particular system call.
 * 
 * === HOW TO ADD A NEW SYSTEM CALL ===
 *   1) Add a SYS_* constant in include/syscall.h.
 *   2) Add a case to the switch below.
 *   3) Implement a static helper function in this file (or
 *      add a new kernel .c file if it's more than ~20 lines).
 *   4) Add a user-space wrapper in user/syscall_wrappers.{h,c}.
 *
 * === HOW TO IMPLEMENT A NEW SYSTEM CALL ===
 *   1) For code that implements a feature that we're studying,
 *      we'll generally want to write our own routines and data
 *      structures to get the right behavior.
 *   2) However, we are able to call the regular C library calls
 *      here. we might do this for something that is too hard
 *      to do ourselves, or for side functionality that isn't
 *      part of our core project.
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>    /* usleep */
#include <time.h>      /* nanosleep fallback */
#include "../include/kernel.h"
#include "../include/syscall.h"

/* ------------------------------------------------------------------ *
 *  Forward declarations for internal helpers                          *
 * ------------------------------------------------------------------ */
static syscall_result_t handle_write (uintptr_t fd, uintptr_t buf,
                                      uintptr_t len);
static syscall_result_t handle_read  (uintptr_t fd, uintptr_t buf,
                                      uintptr_t len);
static syscall_result_t handle_exit  (uintptr_t status);
static syscall_result_t handle_getpid(void);
static syscall_result_t handle_sleep (uintptr_t ms);
static syscall_result_t handle_alloc (uintptr_t size);
static syscall_result_t handle_free  (uintptr_t ptr);

/* ------------------------------------------------------------------ *
 *  Simple kernel state                                                *
 * ------------------------------------------------------------------ */
static int next_pid = 1;   /* process-ID counter */

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

    switch (num) {
        case SYS_WRITE:  return handle_write (a0, a1, a2);
        case SYS_READ:   return handle_read  (a0, a1, a2);
        case SYS_EXIT:   return handle_exit  (a0);
        case SYS_GETPID: return handle_getpid();
        case SYS_SLEEP:  return handle_sleep (a0);
        case SYS_ALLOC:  return handle_alloc (a0);
        case SYS_FREE:   return handle_free  (a0);

        /* ----------------------------------------------------------------
         * STUDENTS: add your new cases here.
         *
         * Example:
         *   case SYS_OPEN:  return handle_open(a0, a1, a2);
         *   case SYS_FORK:  return handle_fork();
         * ---------------------------------------------------------------- */

        default:
            kprintf("[kernel] unknown syscall %d — returning ENOSYS\n", num);
            return MINIOS_ENOSYS;
    }
}

/* ------------------------------------------------------------------ *
 *  Handler implementations                                            *
 * ------------------------------------------------------------------ */

static syscall_result_t handle_write(uintptr_t fd,
                                     uintptr_t buf,
                                     uintptr_t len)
{
    const char *s = (const char *)buf;

    if (!s)                    return MINIOS_EINVAL;
    if (fd != 1 && fd != 2)   return MINIOS_EBADF;

    FILE *stream = (fd == 1) ? stdout : stderr;
    size_t written = fwrite(s, 1, (size_t)len, stream);
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

static syscall_result_t handle_exit(uintptr_t status)
{
    kprintf("[kernel] process exiting with status %lu\n", status);
    exit((int)status);
    return MINIOS_OK;   /* unreachable, but keeps the compiler happy */
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
