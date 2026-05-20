/*
 * kernel/kernel_core.c  —  miniOS
 *
 * Kernel startup, shutdown, and the kprintf() logging utility.
 * 
 * Add code to kernel_init() if new functionality requires
 * initialization on startup.
 */

#include <stdio.h>
#include <stdarg.h>
#include "../include/kernel.h"

/* ------------------------------------------------------------------ *
 *  Kernel init / shutdown                                            *
 * ------------------------------------------------------------------ */
void kernel_init(void)
{
    kprintf("[kernel] miniOS kernel initialised\n");
    kprintf("[kernel] %d syscalls registered\n", (int)SYS_MAX - 1);

}

void kernel_shutdown(void)
{
    kprintf("[kernel] miniOS shutting down\n");
}

/* ------------------------------------------------------------------ *
 *  kprintf — kernel-side print for writing trace messages            *
 * ------------------------------------------------------------------ */
void kprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}
