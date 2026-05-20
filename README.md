# miniOS — OS Simulator in C

This C program is designed to take the place of PintOS for our Operating Systems preview


## Project layout

miniOS/
├── include/
│   ├── syscall.h          # syscall numbers & gateway declaration (shared)
│   └── kernel.h           # kernel-internal types & declarations
│
├── kernel/                ### OUR KERNEL PROJECTS WILL BE HERE ###
│   ├── kernel_core.c      # THIS CONTAINS SYSCALLS THAT WE CREATE
│   ├── syscall_gateway.c  # the single kernel entry point (should be no need to edit this)
│   └── syscall_handler.c  # dispatcher + per-syscall handlers
│
├── user/                  ### USER-LEVEL CODE GOES HERE ###
│   ├── syscall_wrappers.h # library header
│   ├── syscall_wrappers.c # thin wrappers around syscall() (corollary of C std library)
│   ├── user_program.c     # example user program (demo)
│   └── shell.c            # minimal interactive shell to extend
│
└── Makefile



## Quick start

make              # compile everything
make run-demo     # run the user_program demo
make run-shell    # launch the interactive shell
make clean        # remove build artefacts


## How to add a system call

### Step 1 — Add a new enum to `syscall_num_t` in `include/syscall.h`

### Step 2 — `kernel/syscall_handler.c`:
 - Add a case to `kernel_handle_syscall()`, with a new handler function name
 - Implement the new handler (in this file, or a new `kernel/fs.c`)

### Step 3 — Create a new wrapper for user space in `user/syscall_wrappers.h` and `.c`

### Step 4 — Use it in a user program


## Kernel logging

The kernel writes diagnostic output to **stderr** via `kprintf()`.
User output goes to **stdout** via `lib_write()`.
This means you can redirect them independently:

$ ./build/demo 2>/dev/null   # suppress kernel noise
$ ./build/demo 2>kernel.log  # capture kernel log separately
