/*
 * user/syscall_wrappers.c  —  miniOS user-space library
 *
 * Implements the library calls declared in syscall_wrappers.h.
 * 
 * In real Linux, this place would be held by the C Standard Library
 * (e.g.: malloc(), free(), etc). Here, they serve the same purpose
 * but each function is intentionally short — its only job is to set up
 * the arguments into the four uintptr_t slots and call syscall().
 * 
 * To add a new system call to our OS, you'll need to create a new wrapper
 * to expose the new syscall to the user level.
 * 
 * In Linux, the C library calls would have more code to validate inputs,
 * transform data, etc. But they would do the same basic job of calling
 * the kernel entry point with the right parameters.
 */

#include <string.h>
#include <stddef.h>
#include "syscall_wrappers.h"

/* ------------------------------------------------------------------ *
 *  I/O                                                                *
 * ------------------------------------------------------------------ */

syscall_result_t lib_write(int fd, const char *buf, size_t len)
{
    return syscall(SYS_WRITE,
                   (uintptr_t)fd,
                   (uintptr_t)buf,
                   (uintptr_t)len,
                   0);
}

syscall_result_t lib_puts(const char *s)
{
    return lib_write(1, s, strlen(s));
}

syscall_result_t lib_read(int fd, char *buf, size_t len)
{
    return syscall(SYS_READ,
                   (uintptr_t)fd,
                   (uintptr_t)buf,
                   (uintptr_t)len,
                   0);
}

/* ------------------------------------------------------------------ *
 *  Process control                                                    *
 * ------------------------------------------------------------------ */

int lib_spawn(void *(*thread_func_ptr)(void *), void *arg_ptr)
{
    return (int)syscall(SYS_SPAWN, (uintptr_t)thread_func_ptr, (uintptr_t)arg_ptr, 0, 0);
}

void lib_process()
{
    syscall(SYS_PROCESS, 0, 0, 0, 0);
}

void lib_exit(int status)
{
    syscall(SYS_EXIT, (uintptr_t)status, 0, 0, 0);
    /* unreachable */
}

void lib_lockinit()
{
    syscall(SYS_LOCKINIT, 0, 0, 0, 0);
}

void lib_lock()
{
    syscall(SYS_LOCK, 0, 0, 0, 0);
}

void lib_unlock()
{
    syscall(SYS_UNLOCK, 0, 0, 0, 0);
}

int lib_getpid(void)
{
    return (int)syscall(SYS_GETPID, 0, 0, 0, 0);
}

void lib_sleep(unsigned int ms)
{
    syscall(SYS_SLEEP, (uintptr_t)ms, 0, 0, 0);
}

/* ------------------------------------------------------------------ *
 *  Memory                                                             *
 * ------------------------------------------------------------------ */

void *lib_alloc(size_t size)
{
    syscall_result_t r = syscall(SYS_ALLOC, (uintptr_t)size, 0, 0, 0);
    return (r < 0) ? NULL : (void *)(uintptr_t)r;
}

void lib_free(void *ptr)
{
    syscall(SYS_FREE, (uintptr_t)ptr, 0, 0, 0);
}
