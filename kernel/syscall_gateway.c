/*
 * kernel/syscall_gateway.c  —  miniOS
 *
 * This is the single crossing point from user space into the kernel.
 *
 ***** NOTE: There will be no need to modify this file as we add new system *****
 ***** calls to miniOS. This code works the same for all system calls.      *****
 *
 * In a real OS, the code in this function would use the special assembly
 * instruction "syscall" to trigger the hardware trap into kernel mode, sending
 * the syscall number and the arguments in registers.
 * 
 * Here it is a regular C function call, but the abstraction is is the same:
 *   - user code calls syscall() with a number and up to 4 arguments
 *   - the gateway validates the call number
 *   - the gateway forwards to kernel_handle_syscall()
 *   - the result (≥0 success, <0 error) comes back to the caller
 *
 */

#include "../include/syscall.h"
#include "../include/kernel.h"

syscall_result_t syscall(syscall_num_t num,
                         uintptr_t    a0,
                         uintptr_t    a1,
                         uintptr_t    a2,
                         uintptr_t    a3)
{
    /* --- Range-check the call number before touching the kernel --- */
    if (num <= 0 || num >= SYS_MAX) {
        return MINIOS_ENOSYS;   /* unknown syscall */
    }

    /* --- Cross the boundary --- */
    return kernel_handle_syscall(num, a0, a1, a2, a3);
}
