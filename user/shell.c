/*
 * user/shell.c  —  miniOS
 *
 * A minimal interactive shell running on top of the miniOS kernel.
 *
 * This gives students a starting point for the "shell" extension:
 *   - The read/parse/dispatch loop is already here.
 *   - Students add built-in commands by filling in cmd_table[].
 *   - As they add more syscalls (fork, exec, open …) they can make
 *     the shell progressively more powerful.
 *
 * Build:  make shell
 * Run:    ./build/shell
 */

#include <string.h>
#include "syscall_wrappers.h"

/* ------------------------------------------------------------------ *
 *  Command table — students add rows here                            *
 * ------------------------------------------------------------------ */
typedef void (*cmd_fn_t)(const char *args);

typedef struct {
    const char *name;
    const char *help;
    cmd_fn_t    fn;
} command_t;

/* Forward declarations */
static void cmd_help (const char *args);
static void cmd_echo (const char *args);
static void cmd_pid  (const char *args);
static void cmd_sleep(const char *args);
static void cmd_exit (const char *args);

static const command_t cmd_table[] = {
    { "help",  "show this help text",       cmd_help  },
    { "echo",  "echo arguments to stdout",  cmd_echo  },
    { "pid",   "print this process's PID",  cmd_pid   },
    { "sleep", "sleep N milliseconds",      cmd_sleep },
    { "exit",  "exit the shell",            cmd_exit  },

    /* ----------------------------------------------------------------
     * STUDENTS: add your built-in commands here.
     *
     * Example:
     *   { "ls",    "list files",        cmd_ls    },
     *   { "cat",   "print a file",      cmd_cat   },
     *   { "run",   "run a process",     cmd_run   },
     * ---------------------------------------------------------------- */

    { NULL, NULL, NULL }   /* sentinel */
};

/* ------------------------------------------------------------------ *
 *  Tiny string helpers (no libc in "user space")                     *
 * ------------------------------------------------------------------ */
static int str_eq(const char *a, const char *b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == '\0' && *b == '\0';
}

/* Simple atoi */
static int my_atoi(const char *s)
{
    int n = 0;
    while (*s >= '0' && *s <= '9') n = n * 10 + (*s++ - '0');
    return n;
}

/* ------------------------------------------------------------------ *
 *  Command implementations                                            *
 * ------------------------------------------------------------------ */

static void cmd_help(const char *args)
{
    (void)args;
    lib_puts("Available commands:\n");
    for (int i = 0; cmd_table[i].name; i++) {
        lib_puts("  ");
        lib_puts(cmd_table[i].name);
        lib_puts("\t— ");
        lib_puts(cmd_table[i].help);
        lib_puts("\n");
    }
}

static void cmd_echo(const char *args)
{
    if (args && *args) {
        lib_puts(args);
    }
    lib_puts("\n");
}

static void cmd_pid(const char *args)
{
    (void)args;
    int pid = lib_getpid();
    char buf[16];
    int i = 0;
    if (pid == 0) { buf[i++] = '0'; }
    while (pid > 0) { buf[i++] = '0' + (pid % 10); pid /= 10; }
    buf[i] = '\0';
    for (int l = 0, r = i - 1; l < r; l++, r--) {
        char t = buf[l]; buf[l] = buf[r]; buf[r] = t;
    }
    lib_puts("PID: ");
    lib_puts(buf);
    lib_puts("\n");
}

static void cmd_sleep(const char *args)
{
    if (!args || !*args) {
        lib_puts("usage: sleep <ms>\n");
        return;
    }
    unsigned int ms = (unsigned int)my_atoi(args);
    lib_puts("Sleeping...\n");
    lib_sleep(ms);
    lib_puts("Awake.\n");
}

static void cmd_exit(const char *args)
{
    (void)args;
    lib_puts("Goodbye.\n");
    lib_exit(0);
}

/* ------------------------------------------------------------------ *
 *  Read-eval-print loop                                              *
 * ------------------------------------------------------------------ */

static void dispatch(char *line)
{
    /* Strip trailing newline */
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
    if (len == 0) return;

    /* Split into command + args at first space */
    char *cmd  = line;
    char *args = NULL;
    for (size_t i = 0; i < len; i++) {
        if (line[i] == ' ') {
            line[i] = '\0';
            args = line + i + 1;
            break;
        }
    }

    /* Look up and dispatch */
    for (int i = 0; cmd_table[i].name; i++) {
        if (str_eq(cmd, cmd_table[i].name)) {
            cmd_table[i].fn(args ? args : "");
            return;
        }
    }

    lib_puts("Unknown command: ");
    lib_puts(cmd);
    lib_puts("  (try 'help')\n");
}

int main(void)
{
    lib_puts("miniOS shell — type 'help' for commands\n\n");

    char buf[256];
    while (1) {
        lib_puts("$ ");
        syscall_result_t n = lib_read(0, buf, sizeof(buf));
        if (n <= 0) break;   /* EOF */
        dispatch(buf);
    }

    lib_puts("\n");
    lib_exit(0);
}
