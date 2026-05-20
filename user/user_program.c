/*
 * user/user_program.c  —  miniOS
 *
 * A sample user-space program.  This is the kind of code students
 * will write to exercise the kernel they are building.
 *
 * It intentionally uses only the wrapper functions from
 * syscall_wrappers.h — never raw syscall() calls, never stdio.
 *
 * Run via:  make run-demo
 */

#include <string.h>
#include "syscall_wrappers.h"

/* Forward declarations for our "processes" */
static void process_greeting(void);
static void process_memory(void);
static void process_io(void);

int main(void)
{
    lib_puts("=== miniOS user-space demo ===\n\n");

    process_greeting();
    process_memory();
    process_io();

    lib_puts("\nAll done. Exiting.\n");
    lib_exit(0);
}

/* ------------------------------------------------------------------ *
 *  Demo 1: basic I/O and PID                                         *
 * ------------------------------------------------------------------ */
static void process_greeting(void)
{
    int pid = lib_getpid();

    char buf[64];
    /* Build message manually — no sprintf in "user space" */
    const char *msg1 = "Hello from user space! My PID is ";
    lib_write(1, msg1, strlen(msg1));

    /* tiny itoa for the demo */
    int n = pid;
    int i = 0;
    if (n == 0) { buf[i++] = '0'; }
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    buf[i] = '\0';
    /* reverse */
    for (int l = 0, r = i - 1; l < r; l++, r--) {
        char t = buf[l]; buf[l] = buf[r]; buf[r] = t;
    }
    lib_puts(buf);
    lib_puts("\n");
}

/* ------------------------------------------------------------------ *
 *  Demo 2: dynamic memory via kernel alloc/free                      *
 * ------------------------------------------------------------------ */
static void process_memory(void)
{
    lib_puts("Requesting 128 bytes from kernel... ");
    char *block = (char *)lib_alloc(128);
    if (!block) {
        lib_puts("FAILED (returned NULL)\n");
        return;
    }
    lib_puts("got it.\n");

    /* Write and read back */
    const char *payload = "data stored in kernel-allocated memory";
    size_t len = strlen(payload);
    for (size_t i = 0; i < len; i++) block[i] = payload[i];
    block[len] = '\0';

    lib_puts("Contents: ");
    lib_puts(block);
    lib_puts("\n");

    lib_puts("Freeing block... ");
    lib_free(block);
    lib_puts("done.\n");
}

/* ------------------------------------------------------------------ *
 *  Demo 3: reading from stdin via the kernel                         *
 * ------------------------------------------------------------------ */
static void process_io(void)
{
    char buf[128];
    lib_puts("Enter something (or press Enter to skip): ");
    syscall_result_t n = lib_read(0, buf, sizeof(buf));
    if (n > 0) {
        lib_puts("Kernel echoes: ");
        lib_write(1, buf, (size_t)n);
    }
}
