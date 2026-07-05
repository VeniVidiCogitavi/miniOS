/*
 * user/user_program.c  —  miniOS
 *
 * Our first program to create and run independent processes under miniOS.
 *
 * Run via:  make run-processes
 */

#include <string.h>
#include <time.h>
#include <stdio.h>
#include "syscall_wrappers.h"

void *processBody(void *pokemonName) {
    printf("Process %s started\n", (char *)pokemonName);

    for (int i = 0; i < 25; i++) {
        lib_yield();
        lib_lock();
        lib_puts("I choose you, ");
        lib_puts((char *)pokemonName);
        lib_puts("!\n");
        lib_unlock();
        lib_sleep(250);  // 1/4 second
    }
    lib_done();
    printf("Process %s done\n", (char *)pokemonName);
    return NULL;
}


int main(void)
{
    lib_puts("=== miniOS basic run processes ===\n\n");

    lib_lockinit();

    lib_spawn(processBody, "Pikachu");
    lib_spawn(processBody, "Charizard");
    lib_spawn(processBody, "Bulbasaur");
    lib_process();  // Will exit the main thread but leave other threads running
    
}

