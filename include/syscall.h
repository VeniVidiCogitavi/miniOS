/*
 * syscall.h  —  miniOS
 *
 * Shared between kernel and user space.
 * Every system call number lives here, along with the types both
 * sides agree on.
 *
 * To add a new system call, do THREE things:
 *   1) Add a SYS_* constant below (give it the next available number).
 *   2) Implement the handler in kernel/syscall_handler.c.
 *   3) Add a user-space wrapper in user/syscall_wrappers.c (and .h).
 */

#ifndef MINIOS_SYSCALL_H
#define MINIOS_SYSCALL_H

#include <stdint.h>

/* ------------------------------------------------------------------ *
 *  Return-value convention                                            *
 *  Non-negative  → success (value is result)                         *
 *  Negative      → error   (value is -errno-style code)             *
 * ------------------------------------------------------------------ */
typedef int64_t syscall_result_t;

/* ------------------------------------------------------------------ *
 *  Error codes                                                        *
 * ------------------------------------------------------------------ */
#define MINIOS_OK          0
#define MINIOS_EBADF      -1   /* bad file descriptor                 */
#define MINIOS_ENOMEM     -2   /* out of memory                       */
#define MINIOS_EINVAL     -3   /* invalid argument                    */
#define MINIOS_ENOSYS     -4   /* syscall not implemented             */
#define MINIOS_EPERM      -5   /* permission denied                   */

/* ------------------------------------------------------------------ *
 *  System call numbers                                                *
 * ------------------------------------------------------------------ */
typedef enum {
    /* --- I/O --- */
    SYS_WRITE  = 1,   /* write(fd, buf, len)              */
    SYS_READ   = 2,   /* read(fd, buf, len)               */

    /* --- Process control --- */
    SYS_EXIT   = 3,   /* exit(status)                     */
    SYS_GETPID = 4,   /* getpid()                         */
    SYS_SLEEP  = 5,   /* sleep(milliseconds)              */

    /* --- Memory --- */
    SYS_ALLOC  = 6,   /* alloc(size)  → ptr as uintptr_t  */
    SYS_FREE   = 7,   /* free(ptr)                        */

    /* ----------------------------------------------------------------
     * STUDENTS: add your new syscall numbers here.
     *
     * Example:
     *   SYS_OPEN   = 8,
     *   SYS_CLOSE  = 9,
     *   SYS_FORK   = 10,
     * ---------------------------------------------------------------- */

    SYS_MAX    /* sentinel — must stay last */
} syscall_num_t;

/* ------------------------------------------------------------------ *
 *  The syscall gateway                                                *
 *                                                                     *
 *  This is the ONE way user code crosses into the kernel.            *
 *  num   — which call (SYS_* above)                                  *
 *  a0-a3 — up to four arguments, cast to uintptr_t                   *
 *                                                                     *
 *  The kernel interprets the arguments according to num.             *
 * ------------------------------------------------------------------ */
syscall_result_t syscall(syscall_num_t num,
                         uintptr_t a0,
                         uintptr_t a1,
                         uintptr_t a2,
                         uintptr_t a3);

#endif /* MINIOS_SYSCALL_H */
