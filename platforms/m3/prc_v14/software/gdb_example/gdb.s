/* 
 *
 * GDB Hooks for GDBoverMBUS support
 *
 *
 * Andrew Lukefahr
 * lukefahr@umich.edu
 *
 * Much of this is borrowed from:
 * https://github.com/adamheinrich/os.h/blob/master/src/os_pendsv_handler.s
 */

.syntax unified
.cpu cortex-m0
.fpu softvfp

.thumb 

.global handler_svcall
.type handler_svcall, %function
handler_svcall:

    push {lr}

    nop
   
    pop {pc}


#.global main 
#.type main, %function
#main:
#    push {lr}
#
#    nop
#   
#    pop {pc}

