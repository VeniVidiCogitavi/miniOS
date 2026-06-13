/*
 * user/syscall_wrappers.h  —  miniOS user-space library
 *
 * Thin, readable wrappers around the raw syscall() gateway.
 * User programs #include this instead of calling syscall() directly,
 * just as real programs use libc rather than raw int 0x80.
 *
 * When you add a new SYS_* constant to syscall.h, add a matching
 * wrapper function here (declaration) and in syscall_wrappers.c
 * (definition).
 */

#ifndef MINIOS_SYSCALL_WRAPPERS_H
#define MINIOS_SYSCALL_WRAPPERS_H

#include <stddef.h>
#include <stdint.h>
#include "../include/syscall.h"

/* ------------------------------------------------------------------ *
 *  I/O                                                                *
 * ------------------------------------------------------------------ */

/* Write len bytes from buf to file descriptor fd.
 * Returns number of bytes written, or negative on error. */
syscall_result_t lib_write(int fd, const char *buf, size_t len);

/* Convenience: write a NUL-terminated string to stdout. */
syscall_result_t lib_puts(const char *s);

/* Read up to len-1 bytes from fd into buf (NUL-terminated).
 * Returns number of bytes read, or negative on error. */
syscall_result_t lib_read(int fd, char *buf, size_t len);

/* ------------------------------------------------------------------ *
 *  Process control                                                    *
 * ------------------------------------------------------------------ */

 /* Create a process, giving the function it will run and the argument it will take. Do not run it yet. */
 /* Returns the PID of the new process (positive) or an error (negative) */
 int lib_spawn(void *(*thread_func_ptr)(void *), void *arg_ptr);

 /* Run all spawned processes. Blocks until all processes have exited. */
 void lib_process();

/* Terminate the current process with the given exit status. */
void lib_exit(int status);

/* Initialize a concurrency lock */
void lib_lockinit(void);

/* Ask for a process lock */
void lib_lock(void);

/* Release a process lock */
void lib_unlock(void);

/* Yield to the OS for timeslice check */
void lib_yield(void);

/* Notify OS that process is exiting */
void lib_done(void);

/* Return this process's PID. */
int lib_getpid(void);

/* Sleep for ms milliseconds. */
void lib_sleep(unsigned int ms);

/* ------------------------------------------------------------------ *
 *  Memory                                                             *
 * ------------------------------------------------------------------ */

/* Allocate size bytes; returns pointer or NULL on failure. */
void *lib_alloc(size_t size);

/* Free memory previously obtained from lib_alloc. */
void  lib_free(void *ptr);

#endif /* MINIOS_SYSCALL_WRAPPERS_H */
