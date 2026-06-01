/*
 * user/user_program.c  —  miniOS
 *
 * Our first program to create and run independent processes under miniOS.
 *
 * Run via:  make run-processes
 */

#include <string.h>
#include "syscall_wrappers.h"

void *processBody(void *pokemonName) {
    lib_lockinit();
    for (int i = 0; i < 1000; i++) {
        lib_lock();
        lib_puts("I choose you, ");
        lib_puts((char *)pokemonName);
        lib_puts("!\n");
        lib_unlock();
    }
    return NULL;
}


int main(void)
{
    lib_puts("=== miniOS basic run processes ===\n\n");

    lib_spawn(processBody, "Pikachu");
    lib_spawn(processBody, "Charizard");
    lib_process();

    lib_puts("\nAll done. Exiting.\n");
    lib_exit(0);
}

